/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  快速线性键程映射：把除法换成"乘法 + 移位"。
 *
 *  板级 config.h 里 #define ANALOG_MODEL_LINEAR_FAST 即启用本模型；
 *
 *  数学等价(与 analog_model.h 内联线性兜底同一条线性曲线)：
 *    原线性：  sw = (absv - top) * 255 / (bottom - top)          // 一次 32 位除法
 *    本模型：  sw = ((absv - top) * K[ki]) >> 8                  // 一次乘法 + 移位
 *      其中 K[ki] = round(255 * 256 / (bottom - top))           // 校准时预计算
 *
 * ========================================================================= */
#pragma once

#include "analog_core.h"

/* 8 位输出 + 8 位小数定点：K = round((255<<8) / span)，热路径右移本位数。 */
#define LINFAST_FRAC_BITS 8u

/* 编译期出厂默认 K：从出厂锚点推导(与 ISF_DEFAULT_D/K 同源)。
 *   span = DEFAULT_BOTTOM - DEFAULT_TOP (>0 由下方静态断言保证)
 *   K    = round(255*256 / span) = ((255<<8) + span/2) / span         */
_Static_assert(ANALOG_DEFAULT_BOTTOM_READING > ANALOG_DEFAULT_TOP_READING,
               "ANALOG_MODEL_LINEAR_FAST 需要出厂默认 bottom > top，否则默认 K 除零");
#define LINFAST_DEFAULT_SPAN ((uint32_t)((uint32_t)ANALOG_DEFAULT_BOTTOM_READING - (uint32_t)ANALOG_DEFAULT_TOP_READING))
#define LINFAST_DEFAULT_K     ((uint16_t)((((uint32_t)(255u << LINFAST_FRAC_BITS)) + (LINFAST_DEFAULT_SPAN / 2u)) / LINFAST_DEFAULT_SPAN))

/* 每键预计算的 8.8 定点倒数乘子；出厂即 LINFAST_DEFAULT_K，校准时由
 * analog_backend_calibration_changed 重算。span<=0(未校准)时恒为 0。 */
static uint16_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = LINFAST_DEFAULT_K };

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    /* 与线性兜底同口径：top/bottom 非法或倒挂 => 未校准，K=0(热路径恒返 0)。 */
    if (top == 0 || bottom == 0 || top >= bottom) {
        K[ki] = 0;
        return;
    }

    uint32_t span = (uint32_t)bottom - (uint32_t)top;
    /* K = round((255<<8) / span) = ((255<<8) + span/2) / span。
     * span>=1 => K<=65280 放进 uint16；热路径乘积 (absv-top)*K < (span-1)*(65280/span+0.5)
     * ≈ 65280+span/2，对最大 span(2047)约 6.7e4，uint32 接得住，绝不溢出。 */
    K[ki] = (uint16_t)((((uint32_t)(255u << LINFAST_FRAC_BITS)) + (span / 2u)) / span);
}

__attribute__((weak)) uint8_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t floor  = k->top_reading;
    uint16_t bottom = k->bottom_reading;
    if (bottom <= floor) return 0; /* 未校准 / 非法锚点(K[ki] 亦为 0)。 */

    if ((uint16_t)absv <= floor) return 0;   /* 顶部(含)以下：行程 0。 */
    if ((uint16_t)absv >= bottom) return 255; /* 触底(含)以上：行程 255。 */

    /* 一次 16x16->32 乘法 + 一次 >>8，替代原 32 位除法。 */
    uint16_t d = (uint16_t)((uint16_t)absv - floor); /* floor < absv < bottom 已由 clamp 保证 */
    uint32_t r = ((uint32_t)d * (uint32_t)K[ki]) >> LINFAST_FRAC_BITS;
    return (uint8_t)(r > 255u ? 255u : r); /* 大 span 时定点上取整可能多 1 LSB，封顶 */
}
