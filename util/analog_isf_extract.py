#!/usr/bin/env python3
"""从 kb 配置里取出 ISF 表生成器所需的 6 个量，打印成 make 可用的形式。

背景
----
util/analog_isf_gen.py 需要的 6 个输入(ANALOG_TOPREADING_MIN/MAX、
ANALOG_BOTTOMREADING_MIN/MAX、ANALOG_MAX_TRAVEL、ANALOG_ISF_IGNORE_BITS)
都是 **C 预处理宏**，而且它们的值可以来自

  1. kb config.h     (显式 #define)
  2. 本脚本的兜底：校准端点区间外边(TOPREADING_MIN)未定义则取内边
     (TOPREADING_MAX，即默认校准值)；BOTTOMREADING_MAX 未定义则
     由 ADC 量程推导
  3. quantum/analog/analog_core.h §6.5 的 #ifndef 兜底(IGNORE_BITS = 1)

这些兜底里含**算术表达式**(如 ANALOG_READING_MAX = ((1u << ANALOG_ADC_BITS) - 1u))，
预处理器的 # 字符串化**不做算术折叠**——它会原样吐出 "((1u << 12) - 1u)"。
所以纯 `cpp -E` 的路线拿不到可用的整数，必须借真正的编译器来做常量折叠。

做法
----
生成一个只含常量的探针 TU，把 6 个量写成数组初始化式，编译成 .o，
再从 .rodata 里按小端读回 6 个 uint32。全程不依赖任何运行期环境、
不需要链接、不需要执行目标机代码。

探针 TU 用按 #ifndef 语义解析并就地代换(见 resolve/expand)后的整常量表达式
填充——与 analog_core.h §6.5 的兜底(IGNORE_BITS、BOTTOMREADING_MAX 的 ADC
量程推导、TOPREADING_MIN 取内边)保持同一套语义，"没配区间的板"也能得到与
固件实际编译时完全一致的值。
"""

import argparse
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile

# 探针取出的 6 个量，顺序即 .rodata 里的顺序(make 侧按同一顺序取值)。
FIELDS = (
    "ANALOG_MAX_TRAVEL",
    "ANALOG_ISF_IGNORE_BITS",
    "ANALOG_TOPREADING_MIN",
    "ANALOG_TOPREADING_MAX",
    "ANALOG_BOTTOMREADING_MIN",
    "ANALOG_BOTTOMREADING_MAX",
)

PROBE_TEMPLATE = """\
/* 由 util/analog_isf_extract.py 生成，仅用于常量折叠取值。 */
#define ANALOG_ISF_PROBE_TOPREADING_MIN  (%(q_top_min)s)
#define ANALOG_ISF_PROBE_TOPREADING_MAX  (%(q_top_max)s)
#define ANALOG_ISF_PROBE_BOTTOM_MIN     (%(q_bottom_min)s)
#define ANALOG_ISF_PROBE_BOTTOM_MAX     (%(q_bottom_max)s)
#define ANALOG_ISF_PROBE_MAX_TRAVEL     (%(q_max_travel)s)
#define ANALOG_ISF_PROBE_IGNORE_BITS    (%(q_ignore_bits)s)

const unsigned int analog_isf_probe[6] = {
    ANALOG_ISF_PROBE_MAX_TRAVEL,
    ANALOG_ISF_PROBE_IGNORE_BITS,
    ANALOG_ISF_PROBE_TOPREADING_MIN,
    ANALOG_ISF_PROBE_TOPREADING_MAX,
    ANALOG_ISF_PROBE_BOTTOM_MIN,
    ANALOG_ISF_PROBE_BOTTOM_MAX,
};
"""


def die(msg):
    sys.stderr.write("analog_isf_extract: 错误: %s\n" % msg)
    raise SystemExit(1)


def read_macro(path, name):
    """从 C 头文件里取 `#define NAME <value>` 的原始值串(最后一处定义优先)。

    只做最朴素的扫描：跳过注释，取该宏第一次/最后一次出现的定义体。
    QMK 的 config.h 风格很规整，不需要完整预处理器。
    """
    if not path or not os.path.isfile(path):
        return None
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    # 去块注释与行注释，避免误命中注释里的示例
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    value = None
    # 行续接的 #define 也要支持
    text = re.sub(r"\\\n", " ", text)
    for line in text.splitlines():
        m = re.match(r"\s*#\s*define\s+%s\s+(.+?)\s*$" % re.escape(name), line)
        if m:
            value = m.group(1).strip()
    return value


def strip_inline_comment(value):
    return re.sub(r"/\*.*?\*/", " ", value, flags=re.S).strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", required=True, help="C 编译器(用于常量折叠，需能产出 .o)")
    ap.add_argument("--config-h", action="append", default=[],
                    help="config.h，按'从浅到深'给(可多次；后来者优先)")
    ap.add_argument("--keymap-config-h", default=None,
                    help="键位级 config.h(优先级最高，等价于追加到 --config-h 末尾)")
    ap.add_argument("--defaults-h", action="append", default=[],
                    help="提供 #ifndef 兜底的头文件(analog_core.h §6.5)")
    ap.add_argument("--format", choices=("make", "json", "shell", "sh"), default="make")
    ap.add_argument("--workdir", default=None,
                    help="探针源/目标文件的落盘目录(默认系统临时目录)")
    ap.add_argument("--extra-cflags", default="")
    ap.add_argument("--emit-gen-cmd", action="store_true",
                    help="折叠完直接以子进程调用生成器(见 main 末尾的说明)")
    ap.add_argument("--gen", default=None, help="--emit-gen-cmd 时的生成器脚本路径")
    ap.add_argument("--output", default=None, help="--emit-gen-cmd 时的产物路径")
    args = ap.parse_args()

    # --- 1. 逐级取宏：keymap config.h(最后包含) > kb config.h(深的优先) ---
    # config.h 的包含顺序是 KEYBOARD_PATH_5..1，后面的覆盖前面的；
    # 调用方按"从浅到深"传入，这里后来者优先。
    sources = list(args.config_h)
    if args.keymap_config_h:
        sources.append(args.keymap_config_h)

    def resolve(name, default_expr=None):
        for path in reversed(sources):
            v = read_macro(path, name)
            if v is not None:
                return strip_inline_comment(v), path
        v = read_macro_first(args.defaults_h, name)
        if v is not None:
            return v, "<defaults>"
        return default_expr, None

    # 默认校准值 = 校准端点区间内边(analog_core.h §6)：必须由 kb config.h 定义
    # (core 在缺失时 #error)。本脚本同样要求二者都给到，否则报错。
    top_max_raw = resolve("ANALOG_TOPREADING_MAX")[0]
    bottom_min_raw = resolve("ANALOG_BOTTOMREADING_MIN")[0]

    if top_max_raw is None:
        die("kb config.h 未定义 ANALOG_TOPREADING_MAX (默认校准值: 初始校准读数)")
    if bottom_min_raw is None:
        die("kb config.h 未定义 ANALOG_BOTTOMREADING_MIN (默认校准值: 触底校准读数)")

    # 兜底表达式(如 ANALOG_BOTTOMREADING_MAX 的 ADC 量程推导)里引用的名字
    # 必须在探针 TU 里可见，所以把解析到的整串**就地代换**进去，而不是原样抄名字。
    def expand(expr, subst):
        for name, value in subst.items():
            expr = re.sub(r"\b%s\b" % re.escape(name), "(%s)" % value, expr)
        return expr

    # ANALOG_BOTTOMREADING_MAX：kb 可定义，未定义才由 ADC 量程推导。
    # 与 analog_core.h §6.5 的 #ifndef 语义一致：
    #     #ifndef ANALOG_BOTTOMREADING_MAX
    #         #define ANALOG_BOTTOMREADING_MAX ((1u << ANALOG_ADC_BITS) - 1u)
    #     #endif
    # ANALOG_ADC_BITS 由 ADC_RESOLUTION 推导，见 analog_core.h §6.5。
    #
    # ⚠️ ANALOG_ADC_BITS 是一个 #[if/elif] 条件链里的**派生**宏，本脚本的朴素扫描
    # 看到的是链末那条(6)，不是板子实际命中的那条。故它只允许由 ADC_RESOLUTION
    # 推导，**绝不能**去 defaults_h 里读；kb 显式定义时才直接采信。
    adc_res = resolve("ADC_RESOLUTION")[0]
    adc_bits_expr = read_macro_from(sources, "ANALOG_ADC_BITS") or \
        adc_bits_from_resolution(adc_res)
    subst = {"ANALOG_ADC_BITS": adc_bits_expr}
    reading_max_expr = expand(
        read_macro_from(sources, "ANALOG_BOTTOMREADING_MAX")
        or "((1u << ANALOG_ADC_BITS) - 1u)", subst)
    subst["ANALOG_BOTTOMREADING_MAX"] = reading_max_expr

    top_max = expand(top_max_raw, subst)
    bottom_min = expand(bottom_min_raw, subst)
    # 校准端点区间外边未定义则取内边(默认校准值)，退化成零宽区间；
    # 生成器已放宽支持 top_min == top_max。
    top_min = expand(resolve("ANALOG_TOPREADING_MIN", top_max)[0], subst)
    bottom_max = reading_max_expr
    max_travel = expand(resolve("ANALOG_MAX_TRAVEL", "255")[0], subst)
    ignore_bits = expand(resolve("ANALOG_ISF_IGNORE_BITS", "1")[0], subst)

    exprs = {
        "q_top_min": top_min,
        "q_top_max": top_max,
        "q_bottom_min": bottom_min,
        "q_bottom_max": bottom_max,
        "q_max_travel": max_travel,
        "q_ignore_bits": ignore_bits,
    }

    values = fold(exprs, args.cc, args.extra_cflags, args.workdir)

    if args.emit_gen_cmd:
        # 为什么不打印数值让 make 拼命令行：BUILD_CMD(builddefs/message.mk:36)
        # 把整条 CMD 塞进 `LOG=$$( ... )` 的命令替换里，CMD 里的 `$` 会被外层
        # shell 再展开一次。于是 `$$ANALOG_TOPREADING_MIN` 到 shell 手里变成
        # `$ANALOG_TOPREADING_MIN`，实测取到的是错位的名字
        # (`invalid int value: 'NALOG_TOPREADING_MIN'`)。
        # 与其在 make 里叠转义层数，不如让本脚本直接把生成器跑起来——
        # 值在 Python 侧是干净的整数，没有任何 shell 参与。
        if not args.gen or not args.output:
            die("--emit-gen-cmd 需要同时给 --gen 与 --output")
        # 顺手落盘本次实际用到的 6 个整数：生成结果不对时看它即可区分
        # "取值错了"还是"生成器错了"。
        if args.workdir:
            try:
                os.makedirs(args.workdir, exist_ok=True)
                with open(os.path.join(args.workdir, "extracted.txt"),
                          "w", encoding="utf-8", newline="\n") as f:
                    for i, name in enumerate(FIELDS):
                        f.write("%s = %d\n" % (name, values[i]))
            except OSError:
                pass  # 诊断产物，写不了不影响构建
        gen_cmd = [
            sys.executable, args.gen,
            "--top-min", str(values[FIELDS.index("ANALOG_TOPREADING_MIN")]),
            "--top-max", str(values[FIELDS.index("ANALOG_TOPREADING_MAX")]),
            "--bottom-min", str(values[FIELDS.index("ANALOG_BOTTOMREADING_MIN")]),
            "--bottom-max", str(values[FIELDS.index("ANALOG_BOTTOMREADING_MAX")]),
            "--max-travel", str(values[FIELDS.index("ANALOG_MAX_TRAVEL")]),
            "--ignore-bits", str(values[FIELDS.index("ANALOG_ISF_IGNORE_BITS")]),
            "--output", args.output,
        ]
        # 生成器的进度/警告要能透到构建日志里，故 stdout/stderr 直通。
        rc = subprocess.call(gen_cmd)
        raise SystemExit(rc)

    if args.format == "json":
        import json
        out = {FIELDS[i]: values[i] for i in range(len(FIELDS))}
        out["_exprs"] = exprs
        print(json.dumps(out, indent=2))
    elif args.format in ("shell", "sh"):
        # 可被 POSIX sh 直接 `. args.sh` 引用
        for i, name in enumerate(FIELDS):
            print("%s=%d" % (name, values[i]))
    else:
        # make：`$(NAME)` 引用形式，便于 `include args.mk`
        for i, name in enumerate(FIELDS):
            print("%s := %d" % (name, values[i]))


def read_macro_first(paths, name):
    for path in paths:
        v = read_macro(path, name)
        if v is not None:
            return strip_inline_comment(v)
    return None


def read_macro_from(paths, name):
    """只在显式给出的 config.h 链里找(不碰兜底头文件)，后来者优先。"""
    for path in reversed(list(paths)):
        v = read_macro(path, name)
        if v is not None:
            return strip_inline_comment(v)
    return None


def adc_bits_from_resolution(res):
    """analog_core.h §6.5：ADC_RESOLUTION -> ANALOG_ADC_BITS。

    ⚠️ 这里必须按**标识符**判而不是拿数值比：ADC_CFGR1_RES_12BIT 是 QMK 侧
    的枚举名，本脚本看不到它的数值定义。analog_core.h 的做法是
    `#elif ADC_RESOLUTION == ADC_CFGR1_RES_12BIT || ADC_RESOLUTION == 12`，
    两边等值即可，故名字与数字两种写法都要认。
    """
    if not res:
        return "10"  # 与 QMK 的 ADC_RESOLUTION 默认值一致
    tok = res.strip()
    # 去掉可能的尾随注释/多余括号
    tok = re.sub(r"/\*.*?\*/", "", tok).strip()
    tok = tok.strip("()").strip()
    named = {
        "ADC_CFGR1_RES_12BIT": "12",
        "ADC_CFGR1_RES_10BIT": "10",
        "ADC_CFGR1_RES_8BIT": "8",
        "ADC_CFGR1_RES_6BIT": "6",
    }
    if tok in named:
        return named[tok]
    try:
        bits = int(tok, 0)
    except ValueError:
        return "10"
    return str(bits) if bits in (12, 10, 8, 6) else "10"


def fold(exprs, cc, extra_cflags, workdir=None):
    """编译探针 TU，从目标文件读回 6 个已折叠的 uint32。

    workdir 由调用方给(通常是构建中间目录)：沙箱/CI 下系统临时目录可能不可写，
    所以默认不依赖 tempfile。给了 workdir 就复用它并保留探针源便于排查。
    """
    if workdir:
        os.makedirs(workdir, exist_ok=True)
        tmp = workdir
        cleanup = False
    else:
        tmp = tempfile.mkdtemp(prefix="analog_isf_extract_")
        cleanup = True
    try:
        src = os.path.join(tmp, "probe.c")
        obj = os.path.join(tmp, "probe.o")
        with open(src, "w", newline="\n", encoding="utf-8") as f:
            f.write(PROBE_TEMPLATE % exprs)

        # --cc 可能带前缀(如 "ccache arm-none-eabi-gcc")，按词法切开，
        # 否则整个前缀会被当成一个可执行文件名。
        #
        # ⚠️ 不能用 shlex.split(..., posix=True)：它会把 Windows 路径里的
        # 反斜杠当转义符吃掉("D:\Program Files\...\gcc.exe" 变成 "D:Program Filesgcc.exe")。
        # posix=False 保留反斜杠；再去掉可能存在的成对引号。
        cc_argv = [tok.strip('"') for tok in shlex.split(cc, posix=False)]
        # 路径里带空格时(未加引号)上面会被切成两段。若首段不是可执行文件、
        # 而拼回去是，则按整串当一个路径处理。
        if len(cc_argv) > 1 and not os.path.isfile(cc_argv[0]):
            joined = cc.strip('"')
            if os.path.isfile(joined):
                cc_argv = [joined]
        if not cc_argv:
            die("--cc 为空：Makefile 里的 $(CC) 可能尚未定义(检查是否误用了 :=)")
        cmd = cc_argv + ["-c", "-O1", "-o", obj, src]
        if extra_cflags:
            cmd[1:1] = [tok.strip('"') for tok in shlex.split(extra_cflags, posix=False)]
        try:
            proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
        except OSError as e:
            die("无法执行编译器 %s: %s" % (" ".join(cc_argv), e))
        if proc.returncode != 0:
            die("探针编译失败(宏表达式含非整常量?)\n%s" %
                proc.stderr.decode("utf-8", "replace"))

        with open(obj, "rb") as f:
            blob = f.read()
    finally:
        if cleanup:
            shutil.rmtree(tmp, ignore_errors=True)

    data = extract_rodata(blob)
    if len(data) < 24:
        die("从目标文件里只取到 %d 字节，期望至少 24" % len(data))
    return list(struct.unpack("<6I", data[:24]))


def extract_rodata(blob):
    """从 ELF 目标文件里取出 analog_isf_probe 的 24 字节。

    直接解析 ELF 节表，避免依赖 objcopy/nm 的外部调用。
    """
    if blob[:4] != b"\x7fELF":
        die("目标文件不是 ELF(编译器输出异常?)")
    # ELF32/ELF64 都由 ld 产出，这里按 elfclass 分支
    ei_class = blob[4]
    if ei_class == 1:
        # ELF32: e_shoff@0x20 (4), e_shentsize@0x2E (2), e_shnum@0x30 (2), e_shstrndx@0x32 (2)
        shoff, = struct.unpack_from("<I", blob, 0x20)
        shentsize, = struct.unpack_from("<H", blob, 0x2E)
        shnum, = struct.unpack_from("<H", blob, 0x30)
        shstrndx, = struct.unpack_from("<H", blob, 0x32)
        fmt = "<IIIIIIIIII"      # name,type,flags,addr,offset,size,link,info,addralign,entsize
        noff, nsize = 0, 4
    else:
        shoff, = struct.unpack_from("<Q", blob, 0x28)
        shentsize, = struct.unpack_from("<H", blob, 0x3A)
        shnum, = struct.unpack_from("<H", blob, 0x3C)
        shstrndx, = struct.unpack_from("<H", blob, 0x3E)
        fmt = "<IIQQQQIIQQ"
        noff, nsize = 0, 4

    def section(idx):
        base = shoff + idx * shentsize
        return struct.unpack_from(fmt, blob, base)

    # 节名表
    shstr = section(shstrndx)
    if ei_class == 1:
        str_off, str_size = shstr[4], shstr[5]
    else:
        str_off, str_size = shstr[4], shstr[5]
    strtab = blob[str_off:str_off + str_size]

    for i in range(shnum):
        sh = section(i)
        name_off = sh[noff]
        end = strtab.find(b"\x00", name_off)
        name = strtab[name_off:end].decode("ascii", "replace")
        if name in (".rodata", ".rodata.analog_isf_probe", ".data"):
            if ei_class == 1:
                off, size = sh[4], sh[5]
            else:
                off, size = sh[4], sh[5]
            if size >= 24:
                return blob[off:off + size]
    die("目标文件里没有找到 .rodata 节")


if __name__ == "__main__":
    main()
