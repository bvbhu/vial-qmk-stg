/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

/* battery.c 调 adcSTM32EnableTSVREFE 做 VREFINT 校准; F1xx 驱动缺该函数, 在此声明,
 * 定义见 tl12blef103.c。config.h 经 -include 进所有编译单元(含 .S), 故用 __ASSEMBLER__ 守卫。 */
#ifndef __ASSEMBLER__
void adcSTM32EnableTSVREFE(void);
#endif

/* ============================================================================
 *  BHQ 桥接串口: USART2 @ 128000bps
 *    STM32 PA2 (TX) -> 桥 PB4 (RX)
 *    STM32 PA3 (RX) <- 桥 PB7 (TX)
 *
 *  ⚠️ TX 与 RX 的引脚模式必须不同 (F1 GPIOv1):
 *    TX: PAL_MODE_STM32_ALTERNATE_PUSHPULL (复用推挽)
 *    RX: PAL_MODE_INPUT (浮空输入)
 *  RX 曾误写成 ALTERNATE_PUSHPULL, 把 PA3 变成低阻抗输出与桥 PB7 对顶,
 *  钳死整条下行线, 表现为"上行通、下行全死"。
 *  不要照抄裸值 7(那是 GPIOv2 的 AF7), F1 上无效。
 * ========================================================================= */
#define UART_DRIVER SD2
#define UART_TX_PIN A2
#define UART_TX_PAL_MODE PAL_MODE_STM32_ALTERNATE_PUSHPULL
#define UART_RX_PIN A3
#define UART_RX_PAL_MODE PAL_MODE_INPUT

/* 电池分压 ADC: BAT+ --100K-- A7 --100K-- GND */
#define BATTERY_ADC_PIN    A7
#define BATTERY_ADC_DRIVER ADCD1


/* USB 电源检测: PA6, 高电平=已插 USB
 *   (a) 有分压: 5V--100K--A6--100K--GND (推荐)
 *   (b) 无外电路: 5V--串阻--A6, 靠内部下拉钉在低电平
 * 不要用浮空输入, A6 悬空会导致检测忽真忽假。 */
#define USB_POWER_SENSE_PIN       A6
/* 必须保持 1。曾误改为 0 做"反相实验", 结果 0==0 为 true,
 * 被误判为已插 USB -> 电量强制 100%、永不休眠。 */
#define USB_POWER_CONNECTED_LEVEL 1

/* 低功耗 HSE 引脚: PD0=OSC_OUT, PD1=OSC_IN */
#define LPM_STM32_HSE_PIN_IN  D1
#define LPM_STM32_HSE_PIN_OUT D0
