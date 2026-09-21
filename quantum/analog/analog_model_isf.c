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
 *    K  = √top * D                 D,K按键保存，校准时重算
 *    V[i] = 1 /√(i)                编译期常量，但实际计算时，V先左移F位，K * V后结果再右移F位
 *
 *    absv  —— adc读数
 *    sw    —— 行程值，范围 0..M
 *    D     —— 映射参数D：ISF 映射截距，由校准端点推导
 *    K     —— 映射系数K：ISF 映射斜率，每键独立重算
 *    V[]   —— 定点逆平方根查表值：INV_SQRT(浮点逆平方根表)的定点态
 *    ISF_F —— V 的定点小数位：缩放因子即 2^F(V[i] = round(2^F * INV_SQRT[i]))
 *    isf_scalar_t —— ISF 派生标量类型(D/K 的类型)
 *
 *    ANALOG_MAX_TRAVEL              最大键程值 M                  analog_core.h §1.5
 *    ANALOG_TRAVEL_WIDE             M > 255 与否(决定派生参数位宽) analog_core.h §1.5
 *    ANALOG_ADC_BITS                ADC 位宽(QMK 推导)            analog_core.h §6.5
 *    ANALOG_ISF_IGNORE_BITS         查表求下标的右移位数           analog_core.h §6.5
 *                                   (idx = absv >> N，每个表项覆盖 2^N 个读数)
 *    ANALOG_TOPREADING_MIN          校准端点区间                   kb config.h
 *    ANALOG_TOPREADING_MAX          校准端点区间                   kb config.h
 *    ANALOG_BOTTOMREADING_MIN       校准端点区间                   kb config.h
 *    ANALOG_BOTTOMREADING_MAX       校准端点区间                   kb config.h
 * ========================================================================= */

#include "matrix.h" /* MATRIX_ROWS/COLS：必须在 analog_core.h 之前可见(见其 §1 的 #error) */
#include <math.h>
#include "analog_core.h"

/* isf_scalar_t(ISF 派生标量类型) / ISF_F(V 的定点小数位) / ISF_DEFAULT_D / ISF_DEFAULT_K /
 * ISF_INIT_V 在构建期由 util/analog_isf_gen.py 生成 $(INTERMEDIATE_OUTPUT)/src/analog_isf_table.inc */
#include "analog_isf_table.inc"

/* 派生参数(每键一份，校准端点变动时重算)：
 *   D = 映射参数D(截距)，K = 映射系数K(斜率)，均为 isf_scalar_t(ISF 派生标量类型) */
static isf_scalar_t D[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_D };
static isf_scalar_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_K };
/* V = 定点逆平方根查表值：逆平方根表（浮点）INV_SQRT 的定点态，V[i] = round(2^F * INV_SQRT[i])，
 * F 即 ISF_F。每个表项覆盖 2^ANALOG_ISF_IGNORE_BITS 个 adc读数。 */
static const uint16_t V[(ANALOG_BOTTOMREADING_MAX >> ANALOG_ISF_IGNORE_BITS) + 1] = {ISF_INIT_V};

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    if (top == 0 || bottom == 0 || top >= bottom) return; /* 无效读数：判据与线性模型同口径 */
    if (top > ANALOG_TOPREADING_MAX || bottom < ANALOG_BOTTOMREADING_MIN) return;

    float t = sqrtf((float)bottom / (float)top);
    float d = (float)ANALOG_MAX_TRAVEL * t / (t - 1.0f);

    D[ki] = (isf_scalar_t)(d + 0.5f);
    K[ki] = (isf_scalar_t)(sqrtf((float)top) * d + 0.5f);
}

/* absv(adc读数) -> sw(行程值) */
__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t            top    = k->top_reading;    /* 初始校准读数 */
    uint16_t            bottom = k->bottom_reading; /* 触底校准读数 */

    if (absv <= top) return 0;
    if (absv >= bottom) return ANALOG_MAX_TRAVEL;

    uint16_t idx = absv >> ANALOG_ISF_IGNORE_BITS; /* 表项下标：忽略低 N 位 */
    if (idx >= (ANALOG_BOTTOMREADING_MAX >> ANALOG_ISF_IGNORE_BITS) + 1u)
        return ANALOG_MAX_TRAVEL;

    int32_t r = D[ki] - (((int32_t)K[ki] * V[idx] + (1 << (ISF_F - 1))) >> ISF_F);
    
    if (r <= 0) return 0;
    if (r >= ANALOG_MAX_TRAVEL) return ANALOG_MAX_TRAVEL;
    return (analog_travel_t)r;
}
