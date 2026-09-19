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
#pragma once

// *********************************************** EC 静电容矩阵 ***********************************************

#define MATRIX_ROWS 5
#define MATRIX_COLS 16

// split 键位（无实体键），扫描时跳过
#define UNUSED_POSITIONS_LIST \
{ \
  {2, 14}, \
  {3, 13}, \
  {4,  3}, {4,  4}, {4,  5}, {4,  7}, {4,  8}, {4,  9}, {4,  12} \
}

// 行引脚
#define MATRIX_ROW_PINS \
    { B6, A13, B0, B1, A10 }

// 列引脚（EC矩阵列走ADC/MUX，无数字引脚，用NO_PIN占位以满足低功耗唤醒扫描）
#define MATRIX_COL_PINS \
    { NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN }

// 有多少个mux
#define AMUX_COUNT 2
// mux最大支持多少列，1切8  1切16。分别对应74HC4051和74HC4067
#define AMUX_MAX_COLS_COUNT 8

// mux的使能脚
#define AMUX_EN_PINS \
    { B9, B8 }

// mux的set脚
#define AMUX_SEL_PINS \
    { B3, A15, A14 }

// 每个AMUX的列数
#define AMUX_COL_CHANNELS_SIZES \
    { 8 , 8 }

// 列对应到mux的pin的映射关系
// 这里的第1个数据是1，那就代表COL0连到了第1个mux的第A1 Pin
#define AMUX_0_COL_CHANNELS \
    { 1, 0, 3, 2, 4, 6, 5, 7 }
// 这里的第1个数据是1，那就代表COL8连到了第2个mux的第A1 Pin
#define AMUX_1_COL_CHANNELS \
    { 1, 2, 0, 3, 4, 5, 7, 6 }
#define AMUX_COL_CHANNELS AMUX_0_COL_CHANNELS, AMUX_1_COL_CHANNELS

// 放电脚
#define DISCHARGE_PIN A5
// 检测脚
#define ANALOG_PORT A4
// 放大器使能脚
#define OPAMP_EN_PIN A6
// 高电平使能
#define OPAMP_EN_ACTIVE 1
// 放电时间(us)
#define DISCHARGE_TIME 1

// *********************************************** Vial analog 出厂默认值 ***********************************************
// 行程域：sw = (absv - top) * M / (bottom - top)，M = ANALOG_MAX_TRAVEL(见下)。absv 为原始 ADC 读数。
// 核心钳位 top ≤ DEFAULT_TOP-8、bottom ≥ DEFAULT_BOTTOM+8；开机实测静置读数重写 top。
#define ANALOG_DEFAULT_TOP_READING 600         /* 静置锚点初值(开机实测覆盖；旧固件固定触发550可用，静置必低于它) */
#define ANALOG_DEFAULT_BOTTOM_READING 900      /* 触底锚点初值(可被校准推至1023) */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 3000  /* 触发行程(0..ANALOG_MAX_TRAVEL) */
#define ANALOG_DEFAULT_RELEASE_THRESHOLD 1000    /* 释放行程(0..ANALOG_MAX_TRAVEL，与触发点相距 2000 = 半量程) */
#define CALIBRATION_THRESHOLD 32               /* 实时校准：偏离锚点超过该ADC计数才更新 */

// 行程域满量程(0=顶部, M=触底)，经 0xF0 caps 发给 Vial。本板 EC 锚点跨度约 300-400 ADC 计数，
// 但满量程取 4000 换取细粒度滑块(行程域与 ADC 计数无关，只是刻度密度)。
// 满量程 >255 使行程域升 uint16：配置/记录/读数线格式随之从 12/8/3 变为 16/12/4 字节，
// 出厂阈值必须同步改小否则静态断言直接编译失败。
#define ANALOG_MAX_TRAVEL 4000
// *********************************************** Vial analog 出厂默认值 ***********************************************

#ifdef BLUETOOTH_BHQ
// Its active level is "BHQ_IRQ_AND_INT_LEVEL of bhq.h "
#   define BHQ_IQR_PIN          A1
#   define BHQ_INT_PIN          A0
#   define USB_POWER_SENSE_PIN  B7             // USB插入检测引脚
#   define USB_POWER_CONNECTED_LEVEL   1

#   define UART_DRIVER          SD2
/* STM32F411 是 GPIOv2，AF7 = USART2 的 TX/RX，故 PA2/PA3 的复用功能号是 7。
 * 不写裸值：板子换到 F1(GPIOv1，复用功能无编号)后裸值即非法。
 * PAL_MODE_ALTERNATE(7) 在 GPIOv2 上与旧裸值等价，但语义更明确。 */
#   define UART_TX_PIN          A2
#   define UART_TX_PAL_MODE     PAL_MODE_ALTERNATE(7)
#   define UART_RX_PIN          A3
#   define UART_RX_PAL_MODE     PAL_MODE_ALTERNATE(7)

// STM32使用到的高速晶振引脚号，做低功耗需要用户配置，每款芯片有可能不一样的
#define LPM_STM32_HSE_PIN_IN     H1
#define LPM_STM32_HSE_PIN_OUT    H0

#define REPORT_BUFFER_QUEUE_SIZE    68
#define BATTERY_ADC_PIN              A7

#endif

// ws2812
#define WS2812_POWER_PIN    B14
#define WS2812_BYTE_ORDER   WS2812_BYTE_ORDER_GRB
#define RGBLIGHT_LIMIT_VAL 180
#define RGBLIGHT_LAYER_BLINK
#define RGBLIGHT_LAYERS_RETAIN_VAL 2
