/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  键程映射模型选编(模型层标准钩子 analog_model_sw /
 *  analog_backend_calibration_changed 的实现入口)。
 *
 *  analog_core.c 只 #include 本头；本头按板级 config.h 的宏选编实现：
 *    ANALOG_MODEL_ISF         → analog_model_isf.h        (平方反比-快速，磁轴)
 *    ANALOG_MODEL_LINEAR_FAST → analog_model_linear_fast.h (线性，乘+移替代除法)
 *    不定义                    → 下文 #else 内联线性兜底    (纯除法，全 weak)
 *
 *  三个分支的钩子全为 weak；板级 .c 仍可强符号覆盖单个钩子(ELF 语义)。
 *  线性兜底曲线：sw = clamp((absv - top) * M / (bottom - top), 0, M)，
 *  M = ANALOG_MAX_TRAVEL(行程域满量程，板级可配)；
 *  top/bottom 直读核心层校准锚点、无派生参数 -> calibration_changed 空实现；
 *  bottom <= top 视为未校准，恒返 0。
 * ========================================================================= */
#pragma once

#include "analog_core.h"

/* 同时选两个模型属配置错误：#elif 本会让 ISF 静默胜出，直接报错更安全。 */
#if defined(ANALOG_MODEL_ISF) && defined(ANALOG_MODEL_LINEAR_FAST)
#    error "ANALOG_MODEL_ISF 与 ANALOG_MODEL_LINEAR_FAST 不可同时定义，config.h 二选一"
#endif

/* ---- 选择键程映射模型 ----*/

#if defined(ANALOG_MODEL_ISF)
#    include "analog_model_isf.h"
#elif defined(ANALOG_MODEL_LINEAR_FAST)
#    include "analog_model_linear_fast.h"
#else

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    (void)ki;
    (void)top;
    (void)bottom;
}

__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint32_t            floor  = k->top_reading;
    uint32_t            bottom = k->bottom_reading;
    if (bottom <= floor) return 0; /* 未校准 / 非法锚点。 */

    if ((uint32_t)absv <= floor) return 0;                 /* 顶部(含)以下：行程 0。 */
    if ((uint32_t)absv >= bottom) return ANALOG_MAX_TRAVEL; /* 触底(含)以上：满量程。 */

    uint32_t r = (uint32_t)(absv - (uint16_t)floor) * ANALOG_MAX_TRAVEL / (bottom - floor);
    return (r > (uint32_t)ANALOG_MAX_TRAVEL) ? ANALOG_MAX_TRAVEL : (analog_travel_t)r;
}
#endif