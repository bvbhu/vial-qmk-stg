/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  键程映射：平方反比-快速(Hall 磁轴，tl96mgf072)。
 *  板级 config.h 里 #define ANALOG_MODEL_ISF 即启用本模型；
 *
 *    模型：absv = k / (d - sw)²   ⇒   sw = D - K * V
 *      D = 255 * t / (t - 1)，t = sqrt(bottom / top)，预计算
 *      K = sqrt(top) * D / 488，预计算
 *      V = 488 / sqrt(absv)，查表
 * ========================================================================= */
#pragma once

#include <math.h>
#include "analog_core.h"

/* 开机默认 D/K：从出厂校准锚点推导(对应参考实现的 INIT_D/INIT_K)。 */
#define ISF_INIT_T (sqrtf((float)ANALOG_DEFAULT_BOTTOM_READING / (float)ANALOG_DEFAULT_TOP_READING))
#define ISF_INIT_D (255.0f * ISF_INIT_T / (ISF_INIT_T - 1.0f))
static const int16_t ISF_DEFAULT_D = (int16_t)(ISF_INIT_D + 0.5f);
static const int16_t ISF_DEFAULT_K = (int16_t)(sqrtf((float)ANALOG_DEFAULT_TOP_READING) * ISF_INIT_D / 488.0f + 0.5f);

/* clang-format off */
static int16_t D[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_D };
static int16_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_K };

/* V[i] = (488/sqrt(4i) + 488/sqrt(4i+1) + 488/sqrt(4i+2) + 488/sqrt(4i+3)) / 4 */
static const int16_t V[512] = {
    372, 211, 159, 133, 117, 105, 97, 90, 84, 80, 76, 72, 69, 67, 64, 62,
    60, 59, 57, 55, 54, 53, 52, 50, 49, 48, 48, 47, 46, 45, 44, 44, 43,
    42, 42, 41, 40, 40, 39, 39, 38, 38, 37, 37, 37, 36, 36, 35, 35, 35,
    [50 ... 52] = 34, [53 ... 55] = 33, [56 ... 59] = 32, [60 ... 63] = 31,
    [64 ... 68] = 30, [69 ... 72] = 29, [73 ... 78] = 28, [79 ... 84] = 27,
    [85 ... 91] = 26, [92 ... 98] = 25, [99 ... 107] = 24, [108 ... 117] = 23,
    [118 ... 128] = 22, [129 ... 141] = 21, [142 ... 156] = 20, [157 ... 173] = 19,
    [174 ... 194] = 18, [195 ... 218] = 17, [219 ... 247] = 16, [248 ... 282] = 15,
    [283 ... 326] = 14, [327 ... 380] = 13, [381 ... 449] = 12, [450 ... 511] = 11,
};
/* clang-format on */

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    if (top == 0 || bottom == 0 || top >= bottom) return;

    float t = sqrtf((float)bottom / (float)top);
    float d = 255.0f * t / (t - 1.0f);
    D[ki]   = (int16_t)(d + 0.5f);
    K[ki]   = (int16_t)(sqrtf((float)top) * d / 488.0f + 0.5f);
}

__attribute__((weak)) uint8_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    if (absv > 2047) absv = 2047; /* V[] 表覆盖 0..2047(12-bit Hall)。 */
    if (absv == 0) return 0;

    int16_t result = D[ki] - K[ki] * V[absv >> 2];

    if (result < 0 || absv <= g_analog_key[ki].top_reading) return 0;
    if (result > 255 || absv >= g_analog_key[ki].bottom_reading) return 255;
    return (uint8_t)result;
}
