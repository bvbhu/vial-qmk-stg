#!/usr/bin/env python3
"""生成 ISF 键程模型的查表常量（analog_isf_table.inc）。

模型:  absv = k/(d-sw)^2   =>   sw = D - K*V
       D = M*t/(t-1),  t = sqrt(bottom/top),  M = ANALOG_MAX_TRAVEL
       K = sqrt(top)*D/SCALE        (逐键, 校准时重算)
       V[i] = SCALE * INV_SQRT[i]        (编译期常量, 由本脚本生成)

INV_SQRT[i] 是"归一化形状"，只含几何量、不含 SCALE:
       INV_SQRT[i] = mean(1/sqrt(absv))  over  absv in [i*2^N, (i+1)*2^N)
格内取平均而非点采样，与原始手写表的做法一致（原表按 4 点平均）。

SCALE 的选取 —— 硬约束 + 最小化形状误差
----------------------------------------
约束不是"sw(top)<=0 且 sw(bottom)>=M"。analog_model_sw() 的两条早退
    absv <= top            -> 0
    absv >= bottom         -> M
已经结构性地保证了两端能取到 0 和 M，所以那两个不等式恒成立、不构成约束。
真正会被用户感知的是**死区**：早退之内、但 sw 仍被截断成 0 或 M 的那段行程。

    W_top = max{ -sw(x) : top < x < bottom, sw(x) <= 0 }
    W_bot = max{  sw(x)-M : top < x < bottom, sw(x) >= M }

约束 W = max(W_top, W_bot) <= DEADZONE_MAX，在此约束下最小化

    shape = max{ |sw(x) - ideal(x)| : top < x < bottom }

在所有角点 (top,bottom) 上取最大，见 CORNERS。注意 shape 在
DEADZONE_MAX >= ~20 之后不再下降（下界由 K 的整数取整决定），所以
把 DEADZONE_MAX 调得更大没有收益。

用法:
    analog_isf_gen.py --top-min 200 --top-max 350 \
                      --bottom-min 700 --bottom-max 850 \
                      --max-travel 255 --ignore-bits 1 \
                      --deadzone-max 20 --output analog_isf_table.inc
"""

import argparse
import math
import sys

# 在区间内取样做约束/目标评估的角点。端点必取，内部按需加密。
CORNERS_TOP = (200, 230, 260, 290, 320, 350)
CORNERS_BOTTOM = (700, 760, 820, 850)


def build_table(reading_max, ignore_bits):
    """INV_SQRT[i] = mean(1/sqrt(absv)) over the cell; INV_SQRT[0] = 0."""
    step = 1 << ignore_bits
    n_cells = (reading_max >> ignore_bits) + 1
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


def make_eval(max_travel, reading_max, ignore_bits, inv_sqrt):
    """返回 evaluate(top, bottom, scale) -> (deadzone, shape)。"""
    n_max = len(inv_sqrt) - 1

    def evaluate(top, bottom, scale):
        t = math.sqrt(bottom / top)
        d = max_travel * t / (t - 1.0)
        k = max(1, int(math.sqrt(top) * d / scale + 0.5))
        d_int = int(d + 0.5)

        def sw(x):
            idx = min(x >> ignore_bits, n_max)
            return d_int - k * int(scale * inv_sqrt[idx] + 0.5)

        def ideal(x):
            return d - d * math.sqrt(top / x)

        w_top = 0.0
        w_bot = 0.0
        shape = 0.0
        for x in range(int(top) + 1, int(bottom)):
            s = sw(x)
            if s <= 0:
                w_top = max(w_top, -s)
            if s >= max_travel:
                w_bot = max(w_bot, s - max_travel)
            shape = max(shape, abs(s - ideal(x)))
        return max(w_top, w_bot), shape

    return evaluate


def solve(evaluate, corners, deadzone_max, scale_lo, scale_hi):
    """在死区约束下最小化形状误差。返回 (scale, deadzone, shape) 或 None。"""
    best = None
    fallback = None
    for scale in range(scale_lo, scale_hi + 1):
        s = float(scale)
        dz = 0.0
        sh = 0.0
        for top, bottom in corners:
            d, e = evaluate(top, bottom, s)
            dz = max(dz, d)
            sh = max(sh, e)
        if fallback is None or dz < fallback[1]:
            fallback = (scale, dz, sh)
        if dz <= deadzone_max and (best is None or sh < best[2]):
            best = (scale, dz, sh)
    return best, fallback


def fmt_table(inv_sqrt, scale, per_line=4):
    """把 INV_SQRT 按 (int)(ISF_SCALE * <literal>) 展开成 ISF_INIT_V 的行。

    字面量用 %.9g，保证 (int)(scale*literal) 在编译期折叠出的整数与本脚本
    算出的 int(scale*inv_sqrt[i]+0.5) 一致（若不一致则抛错，不静默产出坏表）。
    """
    parts = []
    for i, v in enumerate(inv_sqrt):
        lit = "%.9g" % v
        # %.9g 对 0.0 产出 "0"、对整数产出 "3"，直接加 f 会得到 C 里非法的
        # 整数字面量后缀(0f / 3f)。补上小数点确保是浮点字面量。
        if "." not in lit and "e" not in lit and "E" not in lit:
            lit += ".0"
        lit += "f"
        expect = int(scale * v + 0.5)
        actual = int(scale * float("%.9g" % v) + 0.5)
        if expect != actual:
            raise SystemExit(
                "字面量精度不足: cell %d 折叠出 %d，期望 %d" % (i, actual, expect)
            )
        parts.append("(int)(ISF_SCALE * %s)" % lit)

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
    ap.add_argument("--deadzone-max", type=float, default=20.0)
    ap.add_argument("--scale-lo", type=int, default=100)
    ap.add_argument("--scale-hi", type=int, default=2500)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    # 区间允许退化(top_min == top_max，即"只知道出厂锚点、无漂移")——语义上
    # 与 analog_core.h §6.6 的缺省回退一致：单点区间就是纯锚点推导。
    # 必需的条件只有两条：区间有序，且 top 侧整体浅于 bottom 侧(否则 t<=1、D 无定义)。
    if not (args.top_min <= args.top_max):
        raise SystemExit("区间非法: 要求 top_min <= top_max")
    if not (args.bottom_min <= args.bottom_max):
        raise SystemExit("区间非法: 要求 bottom_min <= bottom_max")
    if not (args.top_max < args.bottom_min):
        raise SystemExit("区间非法: 要求 top_max < bottom_min（否则 t<=1，D = M*t/(t-1) 无定义）")

    corners = [
        (t, b)
        for t in sorted(set([args.top_min, args.top_max] + list(CORNERS_TOP)))
        for b in sorted(set([args.bottom_min, args.bottom_max] + list(CORNERS_BOTTOM)))
        if args.top_min <= t <= args.top_max and args.bottom_min <= b <= args.bottom_max
    ]
    corners = [(t, b) for t, b in corners if t < b]

    inv_sqrt = build_table(args.bottom_max, args.ignore_bits)
    evaluate = make_eval(args.max_travel, args.bottom_max, args.ignore_bits, inv_sqrt)
    best, fallback = solve(evaluate, corners, args.deadzone_max,
                           args.scale_lo, args.scale_hi)

    if best is None:
        scale, dz, sh = fallback
        print(
            "警告: 死区 <= %.1f 无可行解，退化为死区最小解 S=%d (死区 %.1f)"
            % (args.deadzone_max, scale, dz),
            file=sys.stderr,
        )
        best = fallback

    scale, dz, sh = best
    body = fmt_table(inv_sqrt, scale)
    text = """/* 由 util/analog_isf_gen.py 生成，请勿手改。
 *
 *   absv 区间   [%d, %d]      (top/bottom 的漂移范围)
 *   MAX_TRAVEL  %d
 *   IGNORE_BITS %d            (每格 %d 个读数)
 *   INV_SQRT 格数    %d
 *
 *   SCALE = %d  死区上界 %.0f  形状误差上界 %.2f
 *   （在 [%d,%d]x[%d,%d] 的 %d 个角点上取最坏值）
 */
#ifndef ISF_SCALE
#    define ISF_SCALE %d
#endif

#define ISF_INIT_V \\
%s
""" % (
        args.top_min,
        args.bottom_max,
        args.max_travel,
        args.ignore_bits,
        1 << args.ignore_bits,
        len(inv_sqrt),
        scale,
        dz,
        sh,
        args.top_min,
        args.top_max,
        args.bottom_min,
        args.bottom_max,
        len(corners),
        scale,
        body,
    )

    with open(args.output, "w", newline="\n") as f:
        f.write(text)
    print("已生成 %s: SCALE=%d 死区=%.1f 形状误差=%.2f 格数=%d"
          % (args.output, scale, dz, sh, len(inv_sqrt)))


if __name__ == "__main__":
    main()
