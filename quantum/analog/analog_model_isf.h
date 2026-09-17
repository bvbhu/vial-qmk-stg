/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* =========================================================================
 *  键程映射：平方反比-快速(Hall 磁轴)。板级 config.h 定义 ANALOG_MODEL_ISF 启用。
 *
 *    absv = k/(d-sw)²  ⇒  sw = D - K*V
 *    D = M*t/(t-1)，t = sqrt(bottom/top)；K = sqrt(top)*D/488；V = 488/sqrt(absv) 查表
 *    M = ANALOG_MAX_TRAVEL；488 只是把 D*sqrt(top/absv) 拆成 K、V 两半的比例尺。
 *
 *  热路径只在 top < absv < bottom 求值(先早退再查表)，那里 K*V ≲ 1.07*D，
 *  乘积被 D 界住、不随 absv 变小而发散。
 * ========================================================================= */
#pragma once

#include <math.h>
#include "analog_core.h"

#define ISF_MAX_TRAVEL ((float)ANALOG_MAX_TRAVEL)

/* 派生参数宽度：窄域 int16(历史布局)，宽域 int32(D 会超出 int16)。 */
#if ANALOG_TRAVEL_WIDE
typedef int32_t isf_scalar_t;
#    define ISF_SCALAR_MAX 2147483647ll
#else
typedef int16_t isf_scalar_t;
#    define ISF_SCALAR_MAX 32767ll
#endif

/* 宽度充分性断言：D_max ≤ M*2B/(B-A)，其中
 *   B = DEFAULT_BOTTOM + GUARD, A = DEFAULT_TOP - GUARD
 * 由 t/(t-1) ≤ 2B/(B-A) 放缩而来。
 *
 * ⚠️ 分母必须用**出厂锚点跨度** B-A，不能用 clamp 域的 span：
 * fill_defaults() 是直写出厂锚点、不过 clamp 的，所以上电初值恰好落在
 * clamp 带**之外**；拿 clamp 域去界它会让断言在"锚点几乎相等"的极端配置下
 * 误判通过，而实际 D 已溢出 int16 回绕成负数 → 该键永远输出 0(死键)。
 * 触发说明满量程相对锚点跨度太贪心：调小 M 或拉开 DEFAULT_BOTTOM/DEFAULT_TOP。 */
#define ISF_D_BOUND_NUM ((uint64_t)ANALOG_MAX_TRAVEL * 2u * (uint64_t)(ANALOG_DEFAULT_BOTTOM_READING + ANALOG_CAL_GUARD))
#define ISF_D_BOUND_DEN ((uint64_t)(ANALOG_DEFAULT_BOTTOM_READING - ANALOG_DEFAULT_TOP_READING))
_Static_assert((ANALOG_DEFAULT_BOTTOM_READING - ANALOG_DEFAULT_TOP_READING) > 0,
               "ISF 需要出厂默认 bottom > top，否则分母为 0");
_Static_assert(ISF_D_BOUND_NUM * 8ll <= ISF_D_BOUND_DEN * ISF_SCALAR_MAX,
               "ANALOG_MAX_TRAVEL 相对校准锚点跨度过大：ISF 派生参数 D 会超出 isf_scalar_t，请调小满量程或拉开 DEFAULT_BOTTOM/DEFAULT_TOP");

/* 开机默认 D/K(对应参考实现的 INIT_D/INIT_K)。
 * sqrtf() 在 ISO C 里非常量表达式，靠 GCC 编译期折叠；换非 GCC 工具链需改为运行时初始化。 */
#define ISF_INIT_T (sqrtf((float)ANALOG_DEFAULT_BOTTOM_READING / (float)ANALOG_DEFAULT_TOP_READING))
#define ISF_INIT_D (ISF_MAX_TRAVEL * ISF_INIT_T / (ISF_INIT_T - 1.0f))
static const isf_scalar_t ISF_DEFAULT_D = (isf_scalar_t)(ISF_INIT_D + 0.5f);
static const isf_scalar_t ISF_DEFAULT_K = (isf_scalar_t)(sqrtf((float)ANALOG_DEFAULT_TOP_READING) * ISF_INIT_D / 488.0f + 0.5f);

/* clang-format off */
static isf_scalar_t D[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_D };
static isf_scalar_t K[ANALOG_NUM_KEYS] = { [0 ... (ANALOG_NUM_KEYS - 1)] = ISF_DEFAULT_K };

/* V[i] = (488/sqrt(4i) + 488/sqrt(4i+1) + 488/sqrt(4i+2) + 488/sqrt(4i+3)) / 4
 * i=0 那格含 488/sqrt(0)，此处的 372 是让 absv→0 时 V 有界的凑值；
 * 且热路径只在 top < absv < bottom 求值(top ≥ GUARD = 8)，故下标 0..1 实际不可达。 */
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

/* V[] 覆盖的 absv 上限：12-bit Hall 满量程。表按 4 个采样点一格平均，
 * 故格数 = (MAX_ABS+1)/4；热路径按 absv>>2 索引前必须先把 absv 夹到本上限，
 * 否则下标越界。下面这条断言把它与表的实际长度绑死。 */
#define ISF_V_TABLE_MAX_ABS 2047u
_Static_assert(sizeof(V) / sizeof(V[0]) * 4u == ISF_V_TABLE_MAX_ABS + 1u, "V[] 长度与 ISF_V_TABLE_MAX_ABS 不匹配：表是按 absv>>2 索引的");

__attribute__((weak)) void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom) {
    if (ki >= ANALOG_NUM_KEYS) return;

    if (top == 0 || bottom == 0 || top >= bottom) return;

    float t = sqrtf((float)bottom / (float)top);
    float d = ISF_MAX_TRAVEL * t / (t - 1.0f);
    D[ki]   = (isf_scalar_t)(d + 0.5f);
    K[ki]   = (isf_scalar_t)(sqrtf((float)top) * d / 488.0f + 0.5f);
}

__attribute__((weak)) analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint16_t            top    = k->top_reading;
    uint16_t            bottom = k->bottom_reading;

    /* 未校准 / 锚点倒挂：恒 0(与线性两实现同口径；K[ki] 此时可能是未派生的初值)。 */
    if (bottom <= top) return 0;
    /* 顶部(含)以下：行程 0。**必须先于查表与乘积**，理由见文件头溢出论证。 */
    if (absv <= top) return 0;
    /* 触底(含)以上：满量程。先于 V[] 下标计算，故 absv 超过 2047 也安全。 */
    if (absv >= bottom) return ANALOG_MAX_TRAVEL;

    if (absv > ISF_V_TABLE_MAX_ABS) absv = ISF_V_TABLE_MAX_ABS;

    int32_t r = (int32_t)D[ki] - (int32_t)K[ki] * (int32_t)V[absv >> 2];

    /* 量化台阶 = K，r 可以是 0（整级量化正好跨过 D）也可以为负，不只是舍入。 */
    if (r <= 0) return 0;
    if (r >= (int32_t)ANALOG_MAX_TRAVEL) return ANALOG_MAX_TRAVEL;
    return (analog_travel_t)r;
}
