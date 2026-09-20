/* Copyright 2026 bvbhu
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  线性键程映射(纯除法)：原内联兜底的独立翻译单元。
 *
 *  kb rules.mk 声明 ANALOG_MODEL = linear 即编入本文件。
 *
 *  曲线：sw = clamp((absv - top) * M / (bottom - top), 0, M)
 *    M = ANALOG_MAX_TRAVEL(行程域满量程，kb 可配)；
 *    top/bottom 直读核心层校准锚点、无派生参数 => calibration_changed 空实现；
 *    bottom <= top 视为未校准，恒返 0。
 *
 *  与 analog_model_linear_fast.c 同一条曲线，但用一次 32 位除法、不预计算 K：
 *  适合不想为模型层留 per-key 派生量的板(如静电容，锚点跨度小、除法精度够)。
 * ========================================================================= */

#include "matrix.h" /* MATRIX_ROWS/COLS：必须在 analog_core.h 之前可见(见其 §1 的 #error) */
#include "analog_core.h"

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    (void)ki;
    (void)top;
    (void)bottom;
}

__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint32_t            top    = k->top_reading;
    uint32_t            bottom = k->bottom_reading;
    if (bottom <= top) return 0; /* 未校准 / 非法锚点。 */

    if ((uint32_t)absv <= top) return 0;                 /* 顶部(含)以下：行程 0。 */
    if ((uint32_t)absv >= bottom) return ANALOG_MAX_TRAVEL; /* 触底(含)以上：满量程。 */

    uint32_t r = ((uint32_t)absv - top) * ANALOG_MAX_TRAVEL / (bottom - top);
    return (r > (uint32_t)ANALOG_MAX_TRAVEL) ? ANALOG_MAX_TRAVEL : (analog_travel_t)r;
}
