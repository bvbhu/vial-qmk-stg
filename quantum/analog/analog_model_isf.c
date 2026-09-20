/* Copyright 2026 bvbhu
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  键程映射：平方反比-快速(磁轴)。kb rules.mk 声明 ANALOG_MODEL = isf 即编入本文件。
 *
 *    absv = k / (d - sw)²     轴体读数与键程的关系(磁轴)
 *    sw   = D - K * V
 *
 *    D  = M * t / (t - 1)，t = √(bottom/top)，M = ANALOG_MAX_TRAVEL
 *    K  = √top * D / SCALE                 逐键量，校准时重算
 *    V[i] = SCALE * INV_SQRT[i]                 编译期常量，INV_SQRT 由生成器产出
 *
 *    SCALE 是缩放常数，减少KV取整计算丢失的精度。
 *
 *    ANALOG_MAX_TRAVEL              行程域满量程 M              analog_core.h §1.5
 *    ANALOG_TRAVEL_WIDE             M > 255 与否(决定派生量位宽) analog_core.h §1.5
 *    ANALOG_ADC_BITS                ADC 位宽(QMK 推导)          analog_core.h §6.5
 *    ANALOG_ISF_IGNORE_BITS         查表忽略 absv 低几位        analog_core.h §6.5
 *    ANALOG_TOPREADING_MIN          两端读数范围                kb config.h
 *    ANALOG_TOPREADING_MAX          两端读数范围                kb config.h
 *    ANALOG_BOTTOMREADING_MIN       两端读数范围                kb config.h
 *    ANALOG_BOTTOMREADING_MAX       两端读数范围                kb config.h
 * ========================================================================= */

#include "matrix.h" /* MATRIX_ROWS/COLS：必须在 analog_core.h 之前可见(见其 §1 的 #error) */
#include <math.h>
#include "analog_core.h"

/* 派生参数位宽：窄域 int16，宽域 int32 */
#if ANALOG_TRAVEL_WIDE
typedef int32_t isf_scalar_t;
#    define ISF_SCALAR_MAX 0x7fffffffll
#else
typedef int16_t isf_scalar_t;
#    define ISF_SCALAR_MAX 0x7fffll
#endif

/* ISF_SCALE 与 ISF_INIT_V 在构建期生成 $(INTERMEDIATE_OUTPUT)/src/analog_isf_table.inc */
#include "analog_isf_table.inc"

#define ISF_INIT_T (sqrtf((float)ANALOG_BOTTOMREADING_MIN / (float)ANALOG_TOPREADING_MAX))
#define ISF_INIT_D (ANALOG_MAX_TRAVEL * ISF_INIT_T / (ISF_INIT_T - 1.0f))
static const isf_scalar_t ISF_DEFAULT_D = (isf_scalar_t)(ISF_INIT_D + 0.5f);
static const isf_scalar_t ISF_DEFAULT_K = (isf_scalar_t)(sqrtf((float)ANALOG_TOPREADING_MAX) * ISF_INIT_D / (float)ISF_SCALE + 0.5f);

static isf_scalar_t D[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_D };
static isf_scalar_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_K };
static const isf_scalar_t V[(ANALOG_BOTTOMREADING_MAX >> ANALOG_ISF_IGNORE_BITS) + 1] = {ISF_INIT_V};

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    if (top == 0 || bottom == 0 || top >= bottom) return;
    if (top > ANALOG_TOPREADING_MAX || bottom < ANALOG_BOTTOMREADING_MIN) return;

    float t = sqrtf((float)bottom / (float)top);
    float d = (float)ANALOG_MAX_TRAVEL * t / (t - 1.0f);

    D[ki] = (isf_scalar_t)(d + 0.5f);
    K[ki] = (isf_scalar_t)(sqrtf((float)top) * d / (float)ISF_SCALE + 0.5f);
}

__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t            top    = k->top_reading;
    uint16_t            bottom = k->bottom_reading;

    if (absv <= top) return 0;
    if (absv >= bottom) return ANALOG_MAX_TRAVEL;

    uint16_t idx = absv >> ANALOG_ISF_IGNORE_BITS;
    if (idx >= (ANALOG_BOTTOMREADING_MAX >> ANALOG_ISF_IGNORE_BITS) + 1u) 
        return ANALOG_MAX_TRAVEL;

    int32_t r = (int32_t)D[ki] - (int32_t)K[ki] * (int32_t)V[idx];

    if (r <= 0) return 0;
    if (r >= (int32_t)ANALOG_MAX_TRAVEL) return ANALOG_MAX_TRAVEL;
    return (analog_travel_t)r;
}
