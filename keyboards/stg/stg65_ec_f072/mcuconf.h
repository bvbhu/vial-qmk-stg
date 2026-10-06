/* Copyright 2024 keymagichorse
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* stg65_ec_f072：STM32F072xB（Cortex-M0，最高 48MHz）的 ChibiOS 配置。
 *
 * 相对 stg65_ec(STM32F411) 的差异：
 *   - F0 的 PLL 不是 F4 的 PLLM/PLLN/PLLP/PLLQ，而是「PLLSRC + 可选 PREDIV + PLLMUL」；
 *     F4 的那一套宏在 F0 上不存在，必须删掉（否则 hal_lld 编译报未定义）。
 *   - F0 的 USB 时钟只能选 HSI48 或 PLL/1.5（RCC_CFGR3.USBSW），实用上必须走 HSI48，
 *     因此 STM32_HSI48_ENABLED 必须为 TRUE（ChibiOS 只在它为 TRUE 时才打开 HSI48）。
 *   - 无 RTC dedicated RTCPRE：F0 的 RTC 时钟源只有 LSE/LSI/HSE-32，用 LSI 即可。
 */

#pragma once

#include_next <mcuconf.h>

/* ****************************** 时钟 ******************************
 * 默认走内部 HSI，不依赖外部晶振（若板上没有晶振而误开 HSE，stm32_clock_init()
 * 会卡在等 HSERDY 的死循环里，整板不启动；反之板上焊了晶振而用 HSI 也能跑）：
 *
 *   HSI 8MHz / 2 = 4MHz  --(PLLMUL x12)-->  48MHz = SYSCLK = HCLK = PCLK
 *
 * USB：HSI48 直接 48MHz（RCC_CFGR3.USBSW = HSI48），与上面 PLL 无关。
 *
 * 若确认板上焊了 16MHz 晶振、想改用 HSE（更准的 UART 波特率与 flash 时序），
 * 把下面这几项改成：
 *   #define STM32_HSE_ENABLED  TRUE
 *   #define STM32_PLLSRC       STM32_PLLSRC_HSE   // 16MHz / PREDIV(2) = 8MHz
 *   #define STM32_PREDIV_VALUE 2
 *   #define STM32_PLLMUL_VALUE 6                  // 8MHz x 6 = 48MHz
 * 并把 config.h 里 LPM_STM32_HSE_PIN_IN/OUT 指向 PF0/PF1（F0 的 OSC_IN/OSC_OUT，
 * 两者一起配合 LPM 休眠前关 HSE、唤醒后重开）。 */

#undef STM32_HSI_ENABLED
#define STM32_HSI_ENABLED TRUE
#undef STM32_HSI14_ENABLED
#define STM32_HSI14_ENABLED TRUE /* F0 的 ADC 时钟源（STM32_ADCSW_HSI14） */
#undef STM32_HSI48_ENABLED
#define STM32_HSI48_ENABLED TRUE /* USB 时钟源，必须打开 */
#undef STM32_LSI_ENABLED
#define STM32_LSI_ENABLED TRUE /* RTC / LPM 周期唤醒的时钟源 */
#undef STM32_HSE_ENABLED
#define STM32_HSE_ENABLED FALSE /* 见上方说明：默认不用外部晶振 */
#undef STM32_LSE_ENABLED
#define STM32_LSE_ENABLED FALSE

#undef STM32_SW
#define STM32_SW STM32_SW_PLL
#undef STM32_PLLSRC
#define STM32_PLLSRC STM32_PLLSRC_HSI_DIV2
#undef STM32_PREDIV_VALUE
#define STM32_PREDIV_VALUE 1
#undef STM32_PLLMUL_VALUE
#define STM32_PLLMUL_VALUE 12 /* 4MHz x 12 = 48MHz */

#undef STM32_HPRE
#define STM32_HPRE STM32_HPRE_DIV1
#undef STM32_PPRE
#define STM32_PPRE STM32_PPRE_DIV1

#undef STM32_USBSW
#define STM32_USBSW STM32_USBSW_HSI48

/* ****************************** 串口（BHQ 蓝牙桥） ******************************
 * PA2/PA3 = USART2（F0 上 AF1），供 BHQ 桥 128000-8N1 通信用。 */
#undef STM32_SERIAL_USE_USART2
#define STM32_SERIAL_USE_USART2 TRUE

/* ****************************** ADC ******************************
 * ADC1：EC 静电容矩阵逐键采样（ANALOG_PORT=PA4 = ADC_IN4）
 *       + 电池电压（BATTERY_ADC_PIN=PA7 = ADC_IN7）+ VREFINT(通道17)。 */
#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE

/* ****************************** RTC ******************************
 * 内部 LSI 40kHz（F0 的 LSI 是 40kHz，不是 F4 的 32kHz）：
 * LPM 休眠期间用 RTC 周期唤醒轮询 EC 矩阵、以及低电量时只留 USB 唤醒。
 * F0 的 RTC 是 RTCv2（与 F4 同一 LLD），支持 rtcSTM32SetPeriodicWakeup()；
 * F0 没有 F4 那样的 RTC 时钟预分频宏（STM32_RTCPRE_VALUE），故不设置。 */
#undef STM32_RTCSEL
#define STM32_RTCSEL STM32_RTCSEL_LSI
/* ****************************** RTC ****************************** */
