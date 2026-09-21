#!/usr/bin/env python3
"""生成 ISF 键程模型的定点查表常量（analog_isf_table.inc）。

模型:  absv = k/(d-sw)^2   =>   sw = D - K*V
       D = M*t/(t-1),  t = sqrt(bottom/top),  M = ANALOG_MAX_TRAVEL
       K = sqrt(top)*d                (逐键, 校准时重算；d 为浮点 D)
       V[i] = round(2^F * INV_SQRT[i])  (uint16_t, 编译期常量)

INV_SQRT[i] 是"归一化形状"，只含几何量:
       INV_SQRT[i] = mean(1/sqrt(absv))  over  absv in [i*2^N, (i+1)*2^N)

类型与位宽的选取 —— 全部由本脚本按极端点定
--------------------------------------------
D 与 K 都随校准端点单调：top 越大、bottom 越靠近 top，D/K 越大，故最大值落在
极端点 (top=TOPREADING_MAX, bottom=BOTTOMREADING_MIN)。

  isf_scalar_t: 极端点的 D、K 均可装入 int16_t 则用 int16_t，否则 int32_t。
  F: 热路径恒用 int32 中间量，末端词 V 的定点位宽按 K_max 缩：
       F = min(15, floor(log2(INT32_MAX / K_max))) 修正到还容纳四舍五入的
       +2^(F-1)。K 越大 F 越小 => V 越小、不溢出 int32；K 小到 int16 时 F=15
       （V 顶满 uint16 的分数位，精度最高）。

用法:
    analog_isf_gen.py --top-min 200 --top-max 350 \
                      --bottom-min 700 --bottom-max 850 \
                      --max-travel 255 --ignore-bits 1 \
                      --output analog_isf_table.inc
"""

import argparse
import math

INT32_MAX = 0x7fffffff
INT16_MAX = 0x7fff


def spread(lo, hi, interior):
    """端点 + [lo,hi] 之间 interior 个等分内点，去重排序（退化为单点时只留该点）。

    精度评估（死区/最大曲线误差）在极端点未必出现在校准端点上，所以在校准端点
    区间内按等分内点加密采样；点数随参数区间自适应，而不是写死某块板的取值。
    """
    if lo == hi:
        return (lo,)
    step = (hi - lo) / (interior + 1)
    pts = {lo, hi}
    for i in range(1, interior + 1):
        pts.add(int(round(lo + step * i)))
    return tuple(sorted(pts))


def build_table(reading_max, ignore_bits):
    """每个表项 i 上求 mean(1/sqrt(absv))，即 INV_SQRT[i]；INV_SQRT[0] = 0。"""
    step = 1 << ignore_bits
    n_cells = (reading_max >> ignore_bits) + 1  # 表项数（含 i=0 那一项）
    # inv_sqrt 是 LUT 的浮点态；生成的 .inc 里 V[] 是它的定点态
    # (V[i] = round(2^F * INV_SQRT[i])，见 v_table)。
    inv_sqrt = []
    for i in range(n_cells):
        lo = i * step
        if lo == 0:
            inv_sqrt.append(0.0)
            continue
        hi = lo + step
        total = 0.0
        for a in range(lo, hi):
            a = min(a, reading_max)
            total += 1.0 / math.sqrt(a)
        inv_sqrt.append(total / step)
    return inv_sqrt


def compute_dk(top, bottom, max_travel):
    """D/K 的浮点与取整值。D = M*t/(t-1)，K = sqrt(top)*d（不再除 SCALE）。"""
    t = math.sqrt(bottom / top)
    d = max_travel * t / (t - 1.0)
    return d, int(d + 0.5), int(math.sqrt(top) * d + 0.5)


def choose_f(k_max):
    """选 V 的定点位宽 F：热路径用 int32 中间量，V 按 K_max 缩到不溢出。

    V[i] <= 2^F（INV_SQRT[i] <= 1），故 int32 不溢出的充要条件是
        K_max * 2^F + 2^(F-1) <= INT32_MAX
    另受 V 是 uint16_t 约束 F <= 15。取满足两者的最大 F（K 小时 F=15 顶满）。
    """
    for f in range(15, 0, -1):
        if k_max * (1 << f) + (1 << (f - 1)) <= INT32_MAX:
            return f
    raise SystemExit(
        "K_max=%d 过大：连 F=1 都不满足 int32 中间量（请检查校准端点/最大键程值）" % k_max
    )


def v_table(inv_sqrt, f):
    """V[i] = round(2^F * INV_SQRT[i])，直接产出 uint16_t 整数表。"""
    return [int((1 << f) * v + 0.5) for v in inv_sqrt]


def evaluate(top, bottom, max_travel, ignore_bits, f, vtab, d, d_int, k_int):
    """评估单个评估采样点下的死区与最大曲线误差（信息用，不再参与搜索）。"""
    n_max = len(vtab) - 1
    half = 1 << (f - 1)

    def sw(x):
        idx = min(x >> ignore_bits, n_max)
        return d_int - ((k_int * vtab[idx] + half) >> f)

    def ideal(x):
        return d - d * math.sqrt(top / x)

    w_top = 0.0
    w_bot = 0.0
    max_curve_error = 0.0
    for x in range(int(top) + 1, int(bottom)):
        s = sw(x)
        if s <= 0:
            w_top = max(w_top, -s)
        if s >= max_travel:
            w_bot = max(w_bot, s - max_travel)
        max_curve_error = max(max_curve_error, abs(s - ideal(x)))
    return max(w_top, w_bot), max_curve_error


def fmt_v(vtab, per_line=8):
    """把 uint16_t 整数表展开成 ISF_INIT_V 的行（每行 per_line 个）。

    这里直接写整数，不再用"ISF_SCALE * 浮点字面量"的折叠式——值本身已是
    定点整数，写进 .inc 就是 C 编译器实际读到的常量，不存在折叠不一致。
    """
    parts = [str(v) for v in vtab]
    lines = []
    for i in range(0, len(parts), per_line):
        lines.append("    " + ", ".join(parts[i : i + per_line]) + ",")
    # 除最后一行外每行都要有行续接符——少一个就会让 ISF_INIT_V 在那一行
    # 提前结束，其余取值变成顶层的游离 token（报 "expected identifier before int"）。
    return " \\\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--top-min", type=int, required=True)
    ap.add_argument("--top-max", type=int, required=True)
    ap.add_argument("--bottom-min", type=int, required=True)
    ap.add_argument("--bottom-max", type=int, required=True)
    ap.add_argument("--max-travel", type=int, required=True)
    ap.add_argument("--ignore-bits", type=int, default=1)
    ap.add_argument("--top-interior", type=int, default=4,
                    help="top 校准端点区间内的等分内点数（精度评估采样密度）")
    ap.add_argument("--bottom-interior", type=int, default=3,
                    help="bottom 校准端点区间内的等分内点数")
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    # 区间允许退化(top_min == top_max，即"只知道默认校准值、无校准端点区间")——语义上
    # 与 analog_core.h §6.5 的缺省回退一致：单点区间就是纯默认校准值推导。
    # 必需的条件只有两条：区间有序，且 top 侧整体浅于 bottom 侧(否则 t<=1、D 无定义)。
    if not (args.top_min <= args.top_max):
        raise SystemExit("区间非法: 要求 top_min <= top_max")
    if not (args.bottom_min <= args.bottom_max):
        raise SystemExit("区间非法: 要求 bottom_min <= bottom_max")
    if not (args.top_max < args.bottom_min):
        raise SystemExit("区间非法: 要求 top_max < bottom_min（否则 t<=1，D = M*t/(t-1) 无定义）")

    # 评估采样点按参数区间等分加密采样（端点必取），不再写死某块板的取值。
    eval_top_values = spread(args.top_min, args.top_max, args.top_interior)
    eval_bottom_values = spread(args.bottom_min, args.bottom_max, args.bottom_interior)
    eval_points = [(t, b) for t in eval_top_values for b in eval_bottom_values if t < b]

    inv_sqrt = build_table(args.bottom_max, args.ignore_bits)

    # 每个评估采样点的 D/K；极端点是 (top_max, bottom_min)。
    entries = []
    for top, bottom in eval_points:
        d, d_int, k_int = compute_dk(top, bottom, args.max_travel)
        entries.append((top, bottom, d, d_int, k_int))
    factory = next(e for e in entries
                   if e[0] == args.top_max and e[1] == args.bottom_min)
    d_max = max(e[3] for e in entries)
    k_max = max(e[4] for e in entries)

    # D/K 类型：极端点能装进 16 位才用 int16_t。
    scalar_type = "int16_t" if (d_max <= INT16_MAX and k_max <= INT16_MAX) else "int32_t"

    # F 按 K_max 缩：热路径恒用 int32 中间量，V 缩到 K_max*2^F + 2^(F-1) 不溢出。
    f = choose_f(k_max)
    vtab = v_table(inv_sqrt, f)

    dz = 0.0
    max_curve_error = 0.0
    for top, bottom, d, d_int, k_int in entries:
        q_dz, q_sh = evaluate(top, bottom, args.max_travel, args.ignore_bits,
                              f, vtab, d, d_int, k_int)
        dz = max(dz, q_dz)
        max_curve_error = max(max_curve_error, q_sh)

    body = fmt_v(vtab)
    text = """/* 由 util/analog_isf_gen.py 生成，请勿手改。
 *
 *   absv 区间   [%d, %d]      (top/bottom 的校准端点区间)
 *   MAX_TRAVEL  %d
 *   IGNORE_BITS %d            (每个表项 %d 个读数)
 *   V 表项数    %d            (uint16_t)
 *
 *   定点映射: sw = D - ((K*V + 2^(F-1)) >> F)
 *     D = round(M*t/(t-1)),  t = sqrt(bottom/top)
 *     K = round(sqrt(top)*d)          (逐键, 校准时重算)
 *     V[i] = round(2^F * INV_SQRT[i])
 *
 *   极端点 top=TOPREADING_MAX, bottom=BOTTOMREADING_MIN:
 *     D_max = %d, K_max = %d  =>  isf_scalar_t = %s
 *   F = %d   死区上界 %.1f   最大曲线误差上界 %.2f
 *   （在 [%d,%d]x[%d,%d] 的 %d 个评估采样点上取最坏值）
 */
/* D/K 派生标量类型：由本脚本按极端点 D/K 是否可装入 16 位决定。 */
typedef %s isf_scalar_t;

/* V 的定点位宽：V = round(2^F / sqrt(absv))，热路径 (K*V + 2^(F-1)) >> F。
 * F 由 K_max 决定，保证 int32 中间量不溢出（K 大则 F 小、V 随之缩小）。 */
#define ISF_F %d

/* 出厂默认 D/K（极端点 top=TOPREADING_MAX、bottom=BOTTOMREADING_MIN 推导）。 */
#define ISF_DEFAULT_D %d
#define ISF_DEFAULT_K %d

#define ISF_INIT_V \\
%s
""" % (
        args.top_min,
        args.bottom_max,
        args.max_travel,
        args.ignore_bits,
        1 << args.ignore_bits,
        len(inv_sqrt),
        d_max,
        k_max,
        scalar_type,
        f,
        dz,
        max_curve_error,
        args.top_min,
        args.top_max,
        args.bottom_min,
        args.bottom_max,
        len(eval_points),
        scalar_type,
        f,
        factory[3],
        factory[4],
        body,
    )

    with open(args.output, "w", newline="\n", encoding="utf-8") as fh:
        fh.write(text)
    print("已生成 %s: isf_scalar_t=%s F=%d 死区=%.1f 最大曲线误差=%.2f 表项数=%d"
          % (args.output, scalar_type, f, dz, max_curve_error, len(inv_sqrt)))


if __name__ == "__main__":
    main()
