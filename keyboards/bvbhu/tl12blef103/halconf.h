#pragma once

/* kb_common 低功耗 (lpm_core.c / matrix_sleep_default_stm32.c) 调用 PAL 事件 API
 * (palEnableLineEvent), 该 API 在 hal_pal.h 中受
 * `PAL_USE_CALLBACKS == TRUE || PAL_USE_WAIT == TRUE` 守卫, 默认全关 → 隐式声明报错。
 * 须在 include_next 前预定义 (模板用 #if !defined 守卫, 预定义优先);
 * 配置与 keymagichorse 各键盘 halconf 一致。 */
#define PAL_USE_CALLBACKS TRUE
#define PAL_USE_WAIT      TRUE

#include_next <halconf.h>

// 电池 ADC 采样需要 HAL_USE_ADC
#undef HAL_USE_ADC
#define HAL_USE_ADC TRUE
