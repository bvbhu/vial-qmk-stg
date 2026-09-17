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
 *  详见 wch-ble-bridge/uart_protocol.md §一
 *
 *  ⚠️ UART_TX_PAL_MODE / UART_RX_PAL_MODE 必须显式定义, 且值只能是 GPIOv1 的
 *     PAL_MODE_STM32_ALTERNATE_PUSHPULL(16)。
 *
 *  背景(F1 上曾导致"蓝牙能连上但按键无输出"的静默故障):
 *    platforms/chibios/drivers/uart_serial.c 的兜底逻辑是
 *        #ifdef USE_GPIOV1
 *            #define UART_TX_PAL_MODE PAL_MODE_ALTERNATE_PUSHPULL
 *        #else
 *            #define UART_TX_PAL_MODE 7
 *        #endif
 *    STM32F103 属于 USE_GPIOV1(chibios_config.h:98-99)。注意: 这里的
 *    PAL_MODE_ALTERNATE_PUSHPULL **并非未定义** —— chibios_config.h:100-101
 *    在 STM32F1XX 下已把它映射为 PAL_MODE_STM32_ALTERNATE_PUSHPULL(16),
 *    所以兜底分支本身是能用的。真正的坑是**别处照抄的裸值 7**: GPIOv1 只提供
 *    (lib/chibios/os/hal/ports/STM32/LLD/GPIOv1/hal_pal_lld.h:41,46)
 *        PAL_MODE_STM32_ALTERNATE_PUSHPULL   16
 *        PAL_MODE_STM32_ALTERNATE_OPENDRAIN  17
 *    裸值 7 既不是 16 也不是 17(它是 GPIOv2/v3 的 AF7 编号), 在本板上会把引脚
 *    配成错误模式。参考板 kbd67ble_ec/config.h:101,103 写的就是裸值 7, 不要照抄。
 *
 *    引脚一旦没被配成复用推挽, USART2 的 TX/RX 就不与 PA2/PA3 连通 —— 表现为
 *    uart_transmit() 盲写静默丢弃(无返回值可查), 而 BLE 链路仍由桥独立维持,
 *    于是"连得上、打不出字"。
 *
 *  故此处用宏名显式指定: 一是自文档, 二是不依赖兜底分支的映射是否成立。
 * ========================================================================= */
#define UART_DRIVER SD2
#define UART_TX_PIN A2
#define UART_TX_PAL_MODE PAL_MODE_STM32_ALTERNATE_PUSHPULL
#define UART_RX_PIN A3
#define UART_RX_PAL_MODE PAL_MODE_STM32_ALTERNATE_PUSHPULL

/* 60s休眠*/
#define RUN_MODE_PROCESS_TIME (1000 * 60)
/* 电池分压 ADC: BAT+ --100K-- A7 --100K-- GND */
#define BATTERY_ADC_PIN    A7
#define BATTERY_ADC_DRIVER ADCD1


/* USB 电源检测: PA6, 高电平=已插 USB
 *
 * 两种接法都支持:
 *   (a) 有分压:  5V --100K-- A6 --100K-- GND   (2.5V, 最稳, 推荐)
 *   (b) 无外电路: 5V --10K~100K(串阻, 限流)-- A6
 *       靠**内部下拉**(bhq_common.c / lpm_core.c 里的 gpio_set_pin_input_low)
 *       把"未插"钉在低电平; 插上 USB 时 5V 经串阻拉高即为已插。
 *       串一个电阻是为了限流/ESD, 不是分压 —— A6 是 5V 容忍脚(FT)。
 *
 * 注意: 不要用浮空输入。A6 悬空时读数随机, 会让本判断忽真忽假,
 *       连带影响电量读数、休眠判定与 USB 模式下的广播关闭逻辑。 */
#define USB_POWER_SENSE_PIN       A6
#define USB_POWER_CONNECTED_LEVEL 1

/* 低功耗 HSE 引脚: PD0=OSC_OUT, PD1=OSC_IN */
#define LPM_STM32_HSE_PIN_IN  D1
#define LPM_STM32_HSE_PIN_OUT D0
