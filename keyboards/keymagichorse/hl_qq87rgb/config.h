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


#ifndef SERIAL_NUMBER
#   define SERIAL_NUMBER "mjl_hl6095"
#endif

/*******************************************************matrix********************************************************/
/* key matrix size */
#define MATRIX_ROWS 6
#define MATRIX_COLS 17  // 做两列假灯
/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION ROW2COL

/* key matrix pins */
#define MATRIX_ROW_PINS { B6, B14, B13, B12, A8, B10 }
                        //  Q7      Q6      Q5     Q4
#define MATRIX_COL_PINS { NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, B8}
// 定义列映射到 74HC595 的管脚数量
#define COL_TO_74HC595_PINS_COUNT   16
// index = 595 pin
// value = col
#define HC595_PIN_TO_COL ((const int8_t[]){ \
    [_1_595_Q0] = 15,    \
    [_1_595_Q1] = 8,    \
    [_1_595_Q2] = 9,    \
    [_1_595_Q3] = 10,    \
    [_1_595_Q4] = 11,    \
    [_1_595_Q5] = 12,    \
    [_1_595_Q6] = 13,    \
    [_1_595_Q7] = 14,    \
                        \
    [_2_595_Q0] = 0,    \
    [_2_595_Q1] = 1,    \
    [_2_595_Q2] = 2,   \
    [_2_595_Q3] = 3,   \
    [_2_595_Q4] = 4,   \
    [_2_595_Q5] = 5,    \
    [_2_595_Q6] = 6,   \
    [_2_595_Q7] = 7,    \
})
#define I_595_NUM   2       // 用了多少个595
/* Pin connected to DS of 74HC595 */
#define DS_PIN_74HC595      A4
/* Pin connected to SH_CP of 74HC595 */
#define SHCP_PIN_74HC595    A6
/* Pin connected to ST_CP of 74HC595 */
#define STCP_PIN_74HC595    A5
/*******************************************************matrix********************************************************/

#ifdef BLUETOOTH_BHQ
// Its active level is "BHQ_IRQ_AND_INT_LEVEL of bhq.h " 
#   define BHQ_IQR_PIN          A1             
#   define BHQ_INT_PIN          A0             
#   define USB_POWER_SENSE_PIN  A10             // USB插入检测引脚  

#   define UART_DRIVER          SD2
#   define UART_TX_PIN          A2
#   define UART_TX_PAL_MODE     7
#   define UART_RX_PIN          A3
#   define UART_RX_PAL_MODE	    7

// STM32使用到的高速晶振引脚号，做低功耗需要用户配置，每款芯片有可能不一样的
#define LPM_STM32_HSE_PIN_IN     H1  
#define LPM_STM32_HSE_PIN_OUT    H0

#define REPORT_BUFFER_QUEUE_SIZE    68
#define BATTERY_ADC_PIN              B1
#define BATTERY_ADC_DRIVER           ADCD1

// usb 检测
#define USB_POWER_SENSE_PIN         A10
#define USB_POWER_CONNECTED_LEVEL   1    

#endif

// 1-高电平打开电源  0-低电平关闭电源，根据原理图设置
#define WS2812_POWER_PIN        B7
#define WS2812_POWER_ON_LEVEL   1  

#define RGB_MATRIX_LED_FLUSH_LIMIT 40
#define WS2812_BYTE_ORDER   WS2812_BYTE_ORDER_GRB
#define WS2812_PWM_DRIVER   PWMD3  // default: PWMD2
#define WS2812_PWM_CHANNEL  2  // default: 2
#define WS2812_PWM_PAL_MODE 2  // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
#define WS2812_DMA_STREAM   STM32_DMA1_STREAM2  // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_DMA_CHANNEL  5  // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.