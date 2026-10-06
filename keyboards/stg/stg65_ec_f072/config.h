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

/* stg65_ec_f072 = stg65_ec 的 STM32F072xB（Cortex-M0）版本，硬件功能与引脚定义沿用
 * stg65_ec（STM32F411）不变，改动只在与主控相关的部分：
 *   - 时钟/PLL/USB：见 mcuconf.h（HSI/2x12=48MHz，USB 走 HSI48）
 *   - USART2 的 PAL AF 编号：F411=7 -> F0=1
 *   - LPM 休眠的芯片层驱动：KB_LPM_DRIVER = lpm_stm32f0_rtc_ec_v1（见两个 keymap 的 rules.mk）
 *   - HSE 引脚：H0/H1 -> PF0/PF1（默认不用 HSE）
 * EC 矩阵行/列脚、mux 脚、放电/检测/运放脚、BHQ 蓝牙脚、电池脚、WS2812 脚均不变。 */

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
    { B4, B3, A15 }

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
#define ANALOG_TOPREADING_MAX 600         /* 初始校准读数：默认校准值，同时是 top 的钳位上界(开机实测覆盖；旧固件固定触发550可用，初始校准读数必低于它) */
#define ANALOG_BOTTOMREADING_MIN 900      /* 触底校准读数：默认校准值，同时是 bottom 的钳位下界(可被校准推至1023) */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 0xA00  /* 触发行程(0..ANALOG_MAX_TRAVEL) */
#define ANALOG_DEFAULT_RELEASE_THRESHOLD 0x600    /* 释放行程(0..ANALOG_MAX_TRAVEL) */
#define CALIBRATION_THRESHOLD 32               /* 实时校准：偏离校准端点超过该ADC计数才更新 */

// 最大键程值(0=顶部, M=触底)。
#define ANALOG_MAX_TRAVEL 0xFFF
// *********************************************** Vial analog 出厂默认值 ***********************************************

#ifdef BLUETOOTH_BHQ
// Its active level is "BHQ_IRQ_AND_INT_LEVEL of bhq.h "
#   define BHQ_IQR_PIN          A1
#   define BHQ_INT_PIN          A0
#   define USB_POWER_SENSE_PIN  B7             // USB插入检测引脚
#   define USB_POWER_CONNECTED_LEVEL   1

#   define UART_DRIVER          SD2
/* 裸 AF 编号（STM32F0 上 PA2/PA3 的 USART2 为 AF1）：uart_serial.c 的 GPIOv2 分支
 * 会再包一层 PAL_MODE_ALTERNATE()。F0 的 GPIO 是 GPIOv2（MODER/AFR 风格，同 F4），
 * 因此写裸 AF 编号即可（F411 上同引脚是 AF7，F0 是 AF1，勿照抄）。 */
#   define UART_TX_PIN          A2
#   define UART_TX_PAL_MODE     1
#   define UART_RX_PIN          A3
#   define UART_RX_PAL_MODE     1

/* 高速晶振引脚（F0 的 OSC_IN/OSC_OUT = PF0/PF1，注意与 F4 的 H0/H1 不同）。
 * 仅在 mcuconf.h 打开 HSE 时才被 lpm_chip_stm32f0.h 使用（休眠前关 HSE、唤醒后重开）；
 * 默认走 HSI，不依赖晶振，这两个引脚保持默认模拟态即可。 */
#define LPM_STM32_HSE_PIN_IN     F0
#define LPM_STM32_HSE_PIN_OUT    F1

/* BLE 发送队列深度：每项 sizeof(report_buffer_t)=48B 常驻 RAM
 * （type 1 + pad 3 + tm 4 + report_data[40]）。
 * STM32F072 只有 16KB RAM，而本机型常驻开销很大：
 *   4096B F0 版模拟 EEPROM 缓存(WordBuf，legacy 驱动把整片 EEPROM 缓存在 RAM)
 *  +1280B Vial analog 逐键状态(80 键)
 *  +1184B Vial tap-dance/combo/key-override 表(各 16 项)
 *  +3072B ChibiOS 主栈/进程栈(F0 默认 0x400/0x800)
 *  F411 原值 68 项 = 3264B 时链接直接溢出（实测 bss+data ≈ 18.0KB > 16384B）。
 * 32 项 = 1536B 是**能链接通过的最大值**（再多一项 48B 就溢出），
 * 在 3ms 排水间隔下约 96ms、按真实空口延迟(≥10ms/条)算 ≥320ms 的突发缓冲，
 * 远超人手打字所需的排队深度。
 * 若日后需要腾 RAM：每减 1 项省 48B；VIAL_*_ENTRIES 每减 1 项省约 74B；
 * 不要动 FEE_DENSITY_BYTES（Vial 布局最低需 ~2.3KB，减到 2048 会被
 * nvm_dynamic_keymap.c 的 ≥100B 宏区断言挡下），也不要减小 ChibiOS 栈。 */
#define REPORT_BUFFER_QUEUE_SIZE    32
#define BATTERY_ADC_PIN              A7

#endif

// ws2812
#define WS2812_POWER_PIN    B14
#define WS2812_BYTE_ORDER   WS2812_BYTE_ORDER_GRB
#define RGBLIGHT_LIMIT_VAL 180
#define RGBLIGHT_LAYER_BLINK
#define RGBLIGHT_LAYERS_RETAIN_VAL 2
