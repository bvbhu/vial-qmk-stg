/* Copyright 2025 keymagichorse
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

#ifdef BLUETOOTH_BHQ
// Its active level is "BHQ_IRQ_AND_INT_LEVEL of bhq.h " 
#   define BHQ_IQR_PIN          A1             
#   define BHQ_INT_PIN          A0             



// STM32使用到的高速晶振引脚号，做低功耗需要用户配置，每款芯片有可能不一样的
#define LPM_STM32_HSE_PIN_IN     D1  
#define LPM_STM32_HSE_PIN_OUT    D0

#define REPORT_BUFFER_QUEUE_SIZE    68
#define BATTERY_ADC_PIN              A7
#define BATTERY_ADC_DRIVER           ADCD1
// usb 检测
#define USB_POWER_SENSE_PIN         A10
#define USB_POWER_CONNECTED_LEVEL   1    

#endif

#   define UART_DRIVER          SD2
#   define UART_TX_PIN          A2
#   define UART_RX_PIN          A3

// 0 = 低电平打开电源  高电平关闭电源
// 1 = 高电平打开电源  低电平关闭电源
#define WS2812_POWER_PIN        B10
#define WS2812_POWER_ON_LEVEL   0     

#define WS2812_BYTE_ORDER   WS2812_BYTE_ORDER_GRB
#define RGBLIGHT_LIMIT_VAL 180
#define RGBLIGHT_LAYER_BLINK