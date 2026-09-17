#pragma once

#include_next <mcuconf.h>

// USART2 (A2/A3) — 供 BLE keymap 的桥接串口使用
#undef STM32_SERIAL_USE_USART2
#define STM32_SERIAL_USE_USART2 TRUE

// ADC1 — 供 BLE keymap 的电池电压采样 (A7, kb_common/battery.c)
#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE
