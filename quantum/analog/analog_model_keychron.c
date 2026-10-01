/* Copyright 2026 bvbhu
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  Keychron HE cubic travel mapping, normalized to 0..ANALOG_MAX_TRAVEL.
 *
 *  kb rules.mk 声明 ANALOG_MODEL = keychron 即编入本文件。
 *
 *  Mathematical model (Keychron 原始 ADC 方向: 松开高/按下低, zero > full):
 *
 *    P(x) = A + B·x + C·x² + D·x³
 *    Q(y) = P(ZR − y) − P(ZR)
 *    sw   = M · Q(adc − top) / Q(bottom − top)
 *
 *  Keychron 原始代码: x = adc − (zero − 3121)，按下时 adc 减小、x 从 3121
 *  走向 ≈1940(触底)。本仓库坐标统一为 Top < Bottom(按下读数增大)，把 Keychron
 *  曲线**镜像**后用 y = adc − top 复刻同一段多项式——即取多项式 3121 左侧
 *  (x = 3121 − y) 的段，而不是 3121 右侧。这与把 Keychron 内部 0..240 travel
 *  归一化到 0..M 数学等价，是本映射的"归一化适配"，不是 Keychron C 代码的
 *  逐行复刻(原代码没有 clamp 到 full，而是 clamp 到 245 的内部刻度)。
 *
 *  参数(方案常量，勿随板改; A 在差分中抵消):
 *    A = 426.88962, B = -0.48358, C = 2.04637e-4, D = -2.99368e-8, ZR = 3121
 *    c1 = -(B + 2C·ZR + 3D·ZR²) ≈  0.081046710466     (= -P'(ZR))
 *    c2 =  (C + 3D·ZR)          ≈ -7.566125840e-5     (=  P''(ZR)/2, **负**)
 *    c3 = -D                    =  2.99368e-8         (= -P'''(ZR)/6)
 *  对三次多项式 Taylor 展开精确、无截断; Q 恒正、严格递增(Q' 无实根且首项为正),
 *  sw 随 adc 单调 0..M，边界严格 F(top)=0、F(bottom)=M。
 *  参考点核对: Keychron REF_FULL_RANGE = 3121-1940 = 1181 →
 *  Q(1181) = P(1940)-P(3121) ≈ 39.4991，与原始 Scale = 40/39.4991 吻合。
 *
 *  定点(热路径 int32，按 y ≤ KC_Y_MAX = 4095 即 12 位 ADC 量程设计):
 *    t1 = (C1·y) >> 9                   C1 = round(c1·2^16) =  5311
 *    t2 = (((C2·y) >> 10)·y) >> 13      C2 = round(c2·2^30) = -81241
 *    t3 = ((((C3·y)·y) >> 11)·y) >> 12  C3 = round(c3·2^30) =  32
 *    q7 = t1 + t2 + t3 = Q(y)·2^7       (y=4095 时各中间量上限
 *                                        |C2·y|≤3.3e8 / C3·y²≤5.4e8 / q7∈[9,1.4e5])
 *    sw = (q7·scale[ki] + 2^18) >> 19   scale[ki] = round(M·2^12 / Q(bottom-top))
 *
 *  scale 的跨度约束(校准回调拒绝并保留旧值，出厂默认跨度由静态断言把关):
 *    - 窄行程域(M ≤ 255): scale 是 uint16，需 Q(span) ≥ M·2^12/65535，
 *      即跨度 span = bottom-top ≥ 250(KC_MIN_SPAN)；span=250 → scale≈65.5k
 *    - 宽行程域(M > 255): scale 是 uint32，span ≥ 1 即可；末段乘法升 uint64
 *      (Cortex-M0 上为软乘，宽行程域板子才付这个代价)
 *
 *  精度(跨度 350..650，即 tl96mgf072 参考配置): 最坏曲线误差 ≈ 1.0 行程单位
 *  (含 ≤0.5 整数输出固有舍入)，典型 < 0.3；极端大跨度(≈1900+)≈ 3.3。
 *  误差主要来自负二次项引入的逐级截断与 c3 定点舍入，对 0..M 行程占比 <1.3%。
 * ========================================================================= */

#include "matrix.h" /* MATRIX_ROWS/COLS：必须在 analog_core.h 之前可见(见其 §1 的 #error) */
#include "analog_core.h"

/* ---- Keychron HE 固定三次多项式(方案常量，勿随板改；仅校准冷路径用) ---- */
#define KC_A 426.88962f
#define KC_B -0.48358f
#define KC_C 2.04637e-4f
#define KC_D -2.99368e-8f
#define KC_ZR 3121

/* ---- 定点常量(热路径) ----
 * 按 y ≤ KC_Y_MAX 设计；|C2·y| 在 y=4095 时 3.33e8、C3·y·y 5.37e8、末段乘积
 * ((C2·y)>>10)·y 1.33e9，全部在 int32 内(二次项为负，中间量用有符号)。 */
#define KC_Y_MAX 4095 /* 12 位 ADC 量程；BOTTOMREADING_MAX 默认即 ADC 量程 */
#define KC_C1_Q16 5311
#define KC_C2_Q30 -81241
#define KC_C3_Q30 32
#define KC_POLY_SHIFT 7u            /* q7 = Q(y)·2^KC_POLY_SHIFT */
#define KC_SCALE_SHIFT 12u          /* scale = M·2^KC_SCALE_SHIFT / Q(span) */
#define KC_SCALE_TOTAL_SHIFT (KC_POLY_SHIFT + KC_SCALE_SHIFT) /* 19 */

/* 12 位 ADC 量程与定点格式匹配(出厂 BOTTOMREADING_MAX 默认 = (1<<ADC_BITS)-1 ≤ 4095) */
_Static_assert((uint32_t)ANALOG_BOTTOMREADING_MAX <= KC_Y_MAX,
               "keychron 模型定点常量按 12 位 ADC 量程设计：ANALOG_BOTTOMREADING_MAX 不得超出 4095");

/* scale 承载类型与最小跨度(见文件头)。 */
#if ANALOG_TRAVEL_WIDE
typedef uint32_t kc_scale_t;
#    define KC_MIN_SPAN 1u /* scale 是 uint32，任意 span>0 都装得下 */
#else
typedef uint16_t kc_scale_t;
#    define KC_MIN_SPAN 250u /* scale 是 uint16：Q(250)≈16.0 ⇒ M·2^12/Q ≤ 65.5k < 65535 */
#endif

/* 出厂默认跨度也须 ≥ KC_MIN_SPAN(校准回调的重算口径，编译期把关)。
 * 核心层已断言 BOTTOMREADING_MIN > TOPREADING_MAX，宽域恒成立；窄域是实约束。 */
_Static_assert((uint32_t)ANALOG_BOTTOMREADING_MIN - (uint32_t)ANALOG_TOPREADING_MAX >= KC_MIN_SPAN,
               "keychron 模型要求出厂默认校准跨度 bottom-top ≥ KC_MIN_SPAN，否则 scale 溢出承载类型");

/* 编译期默认 scale(占位值)：按出厂默认校准端点(span = BOTTOMREADING_MIN - TOPREADING_MAX)
 * 用与热路径同一组定点常量近似计算。仅覆盖 analog_init() 重算循环(analog_core.c)
 * 之前的空窗——init 一定在首次扫描前逐键重算，故近似 ±2% 无影响。
 * Q 的宏估值(逐级四舍五入)在极小跨度下可能 ≤ 0，此时退化为 0 不除零。 */
#define KC_DEFAULT_SPAN ((uint32_t)((uint32_t)ANALOG_BOTTOMREADING_MIN - (uint32_t)ANALOG_TOPREADING_MAX))
#define KC_DEFAULT_Q1    (((KC_C1_Q16 * (int32_t)KC_DEFAULT_SPAN + (1 << 15)) >> 16))
#define KC_DEFAULT_Q2    ((((KC_C2_Q30 * (int32_t)KC_DEFAULT_SPAN) >> 10) * (int32_t)KC_DEFAULT_SPAN + (1 << 19)) >> 20)
#define KC_DEFAULT_Q3    (((((KC_C3_Q30 * (int32_t)KC_DEFAULT_SPAN) * (int32_t)KC_DEFAULT_SPAN) >> 11) * (int32_t)KC_DEFAULT_SPAN + (1 << 18)) >> 19)
#define KC_DEFAULT_Q     ((int32_t)(KC_DEFAULT_Q1 + KC_DEFAULT_Q2 + KC_DEFAULT_Q3))
#define KC_DEFAULT_SCALE \
    ((kc_scale_t)((KC_DEFAULT_Q > 0) \
                      ? ((((uint32_t)ANALOG_MAX_TRAVEL << KC_SCALE_SHIFT) + (uint32_t)(KC_DEFAULT_Q / 2)) / (uint32_t)KC_DEFAULT_Q) \
                      : 0u))

/* 每键派生参数：scale = M·2^12/Q(bottom-top)，校准端点变动时重算 */
static kc_scale_t scale[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = KC_DEFAULT_SCALE };

/* Q(y) = P(ZR-y) - P(ZR)：Keychron 三次多项式在参考零点左侧的段(校准冷路径，浮点)。 */
static float kc_q_float(uint32_t y) {
    /* span 可能 > ZR，x = ZR - y 可为负——浮点减法无回绕，Keychron 多项式数学上
     * 在整个实轴上单调，负 x 仍给出合法的递增 Q(y)。 */
    float x  = (float)KC_ZR - (float)y;
    float p0 = ((KC_D * (float)KC_ZR + KC_C) * (float)KC_ZR + KC_B) * (float)KC_ZR + KC_A;
    float p  = ((KC_D * x + KC_C) * x + KC_B) * x + KC_A;
    return p - p0;
}

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    /* 与 ISF 同口径：无效读数(top/bottom 为 0 或倒挂)或校准端点越界 => 保留旧 scale，
     * 不清零(清零会让该键退化成恒定输出)。 */
    if (top == 0 || bottom == 0 || top >= bottom) return;
    if (top > ANALOG_TOPREADING_MAX || bottom < ANALOG_BOTTOMREADING_MIN) return;

    uint32_t span = (uint32_t)bottom - (uint32_t)top;
    if (span < KC_MIN_SPAN) return; /* 跨度过小：scale 超出承载类型，保留旧 scale */

    /* span ≥ 1 时 Q(span) > 0(Q 严格递增且 Q(0)=0)，无除零。 */
    scale[ki] = (kc_scale_t)(((float)ANALOG_MAX_TRAVEL * (float)(1u << KC_SCALE_SHIFT)) / kc_q_float(span) + 0.5f);
}

/* absv(adc读数) -> sw(行程值) */
__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t            top    = k->top_reading;    /* 初始校准读数 */
    uint16_t            bottom = k->bottom_reading; /* 触底校准读数 */
    if (bottom <= top) return 0; /* 无效读数 */

    if (absv <= top) return 0;          /* 顶部(含)以下：行程值 0。 */
    if (absv >= bottom) return ANALOG_MAX_TRAVEL; /* 触底(含)以上：最大键程值。 */

    uint32_t y = (uint32_t)absv - (uint32_t)top;
    if (y >= KC_Y_MAX) return ANALOG_MAX_TRAVEL; /* 超出定点设计域(正常配置到不了)：防御 */

    /* Q(y) 在 Q7(见文件头定点节)；二次项为负故用 int32。y∈[1,4095] 时 q7 恒 > 0。 */
    int32_t t1 = (KC_C1_Q16 * (int32_t)y) >> 9;
    int32_t t2 = (((KC_C2_Q30 * (int32_t)y) >> 10) * (int32_t)y) >> 13;
    int32_t t3 = ((((KC_C3_Q30 * (int32_t)y) * (int32_t)y) >> 11) * (int32_t)y) >> 12;
    int32_t q7 = t1 + t2 + t3;

    /* sw = (q7·scale + 2^(total-1)) >> total。
     * 窄域: q7·scale ≤ M·2^19 ≈ 1.3e8，uint32 中间量稳；宽域升 uint64(Cortex-M0
     * 软乘，仅宽行程域板子付代价)。 */
    uint32_t r;
#if ANALOG_TRAVEL_WIDE
    r = (uint32_t)((((uint64_t)(uint32_t)q7 * scale[ki]) + (1ull << (KC_SCALE_TOTAL_SHIFT - 1))) >> KC_SCALE_TOTAL_SHIFT);
#else
    r = (((uint32_t)q7 * (uint32_t)scale[ki]) + (1u << (KC_SCALE_TOTAL_SHIFT - 1))) >> KC_SCALE_TOTAL_SHIFT;
#endif
    if (r > (uint32_t)ANALOG_MAX_TRAVEL) return ANALOG_MAX_TRAVEL; /* 定点上取整可能多 1 LSB */
    return (analog_travel_t)r;
}
