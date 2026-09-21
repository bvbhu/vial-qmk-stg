/* Copyright 2026 bvbhu
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  快速线性键程映射：把除法换成"乘法 + 移位"。
 *
 *  kb rules.mk 声明 ANALOG_MODEL = linear_fast 即编入本文件；
 *
 *  数学等价(与 analog_model_linear.c 纯除法线性同一条曲线)：
 *    原线性：  sw = (absv - top) * M / (bottom - top)            // 一次 32 位除法
 *    本模型：  sw = ((absv - top) * K[ki]) >> 8                  // 一次乘法 + 移位
 *      其中 K[ki] = round(M * 256 / (bottom - top))             // 校准时预计算
 *      absv = adc读数(kb 由原始 ADC 换算的差值绝对值，核心层不碰 ADC)
 *      sw   = 行程值(行程域值，范围 0..M)
 *      M = ANALOG_MAX_TRAVEL(最大键程值，kb 可配)
 *
 * ========================================================================= */

#include "matrix.h" /* MATRIX_ROWS/COLS：必须在 analog_core.h 之前可见(见其 §1 的 #error) */
#include "analog_core.h"

/* 8 位输出 + 8 位小数定点：K = round((M<<8) / span)，热路径右移本位数。
 * 分数位固定 8、与最大键程值无关——最大键程值只进 K 的分子。 */
#define LINFAST_FRAC_BITS 8u

/* K 的宽度随行程域走：M=255 时 K ≤ 255<<8 放进 uint16(与历史布局一致)；
 * M 升入 uint16 域后 K 最大 (65535<<8)/1 ≈ 1.7e7，必须 uint32。
 * 热路径乘积 d*K 上限 ≈ M<<8 + span/2，uint32 恒接得住。 */
#if ANALOG_TRAVEL_WIDE
typedef uint32_t linfast_k_t;
#else
typedef uint16_t linfast_k_t;
#endif

/* 编译期默认 K：从默认校准值推导(与 ISF_DEFAULT_D/K 同源)。
 *   span = BOTTOMREADING_MIN - TOPREADING_MAX (>0 由下方静态断言保证)
 *   K    = round(M*256 / span) = ((M<<8) + span/2) / span            */
_Static_assert(ANALOG_BOTTOMREADING_MIN > ANALOG_TOPREADING_MAX,
               "linear_fast 模型需要出厂默认 bottom > top，否则默认 K 除零");
#define LINFAST_DEFAULT_SPAN ((uint32_t)((uint32_t)ANALOG_BOTTOMREADING_MIN - (uint32_t)ANALOG_TOPREADING_MAX))
#define LINFAST_DEFAULT_K     ((linfast_k_t)((((uint32_t)ANALOG_MAX_TRAVEL << LINFAST_FRAC_BITS) + (LINFAST_DEFAULT_SPAN / 2u)) / LINFAST_DEFAULT_SPAN))

/* 每键预计算的 8.8 定点倒数乘子(派生参数)；默认即 LINFAST_DEFAULT_K，校准时由
 * analog_backend_calibration_changed 重算。span<=0(无效读数)时恒为 0。 */
static linfast_k_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = LINFAST_DEFAULT_K };

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    /* 与线性兜底同口径：top/bottom 无效读数或倒挂 => K=0(热路径恒返 0)。 */
    if (top == 0 || bottom == 0 || top >= bottom) {
        K[ki] = 0;
        return;
    }

    uint32_t span = (uint32_t)bottom - (uint32_t)top;
    /* K 公式见文件头。溢出论证：span>=1 => K <= M<<8(最大键程值域内)；
     * 热路径乘积 (absv-top)*K < span*K ≈ M<<8 + span/2，对最大 span(2047)
     * 与最大 M(65535)约 1.7e7，uint32 接得住。 */
    K[ki] = (linfast_k_t)((((uint32_t)ANALOG_MAX_TRAVEL << LINFAST_FRAC_BITS) + (span / 2u)) / span);
}

/* absv(adc读数) -> sw(行程值) */
__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t top    = k->top_reading;    /* 初始校准读数 */
    uint16_t bottom = k->bottom_reading; /* 触底校准读数 */
    if (bottom <= top) return 0; /* 无效读数(K[ki] 亦为 0)。 */

    if (absv <= top) return 0;          /* 顶部(含)以下：行程值 0。 */
    if (absv >= bottom) return ANALOG_MAX_TRAVEL; /* 触底(含)以上：最大键程值。 */

    /* 一次乘法 + 一次 >>8，替代原 32 位除法。
     * 窄域是 16x16->32；宽域 K 为 uint32，实为 32x32，uint32 中间量仍接得住。 */
    uint16_t d = (uint16_t)(absv - top); /* top < absv < bottom 已由上面两条早退保证 */
    uint32_t r = ((uint32_t)d * (uint32_t)K[ki]) >> LINFAST_FRAC_BITS;
    /* 大 span 时定点上取整可能多 1 LSB，封顶到最大键程值(不是 255)。 */
    return (r > (uint32_t)ANALOG_MAX_TRAVEL) ? ANALOG_MAX_TRAVEL : (analog_travel_t)r;
}
