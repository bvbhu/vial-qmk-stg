/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  默认键程映射：线性兜底(全 weak，只实现尚未被占用)。
 *
 *    sw = clamp((absv - top) * 255 / (bottom - top), 0, 255)
 *
 *  top/bottom 直读核心层校准锚点，无派生参数 -> calibration_changed 空实现；
 *  bottom <= top 视为未校准，恒返 0。
 *
 * ========================================================================= */
#pragma once

#include "analog_core.h"

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    (void)ki;
    (void)top;
    (void)bottom;
}

__attribute__((weak)) uint8_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint32_t floor        = k->top_reading;
    uint32_t bottom       = k->bottom_reading;
    if (bottom <= floor) return 0; /* 未校准 / 非法锚点。 */

    if ((uint32_t)absv <= floor) return 0; /* 顶部(含)以下：行程 0。 */
    if ((uint32_t)absv >= bottom) return 255; /* 触底(含)以上：行程 255。 */

    uint32_t r = (uint32_t)(absv - (uint16_t)floor) * 255u / (bottom - floor);
    return (uint8_t)(r > 255u ? 255u : r);
}
