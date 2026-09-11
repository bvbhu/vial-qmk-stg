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
#include <stdint.h>
#include <stdbool.h>

// #define USE_74HC4051    // 8 通道多路复用器
// #define USE_74HC4067      // 16 通道多路复用器

#if defined(USE_74HC4051)
    #define MUX_CHANNEL_COUNT   8
#elif defined(USE_74HC4067)
    #define MUX_CHANNEL_COUNT   16
#else
    #error "Please define either USE_74HC4051 or USE_74HC4067"
#endif

// 74HC4051 / 74HC4067 通用通道 宏定义
#define MUX_CH0   0   // S3=0, S2=0, S1=0, S0=0
#define MUX_CH1   1   // S3=0, S2=0, S1=0, S0=1
#define MUX_CH2   2   // S3=0, S2=0, S1=1, S0=0
#define MUX_CH3   3   // S3=0, S2=0, S1=1, S0=1
#define MUX_CH4   4   // S3=0, S2=1, S1=0, S0=0
#define MUX_CH5   5   // S3=0, S2=1, S1=0, S0=1
#define MUX_CH6   6   // S3=0, S2=1, S1=1, S0=0
#define MUX_CH7   7   // S3=0, S2=1, S1=1, S0=1

#ifdef USE_74HC4067
#define MUX_CH8   8   // S3=1, S2=0, S1=0, S0=0
#define MUX_CH9   9   // S3=1, S2=0, S1=0, S0=1
#define MUX_CH10  10  // S3=1, S2=0, S1=1, S0=0
#define MUX_CH11  11  // S3=1, S2=0, S1=1, S0=1
#define MUX_CH12  12  // S3=1, S2=1, S1=0, S0=0
#define MUX_CH13  13  // S3=1, S2=1, S1=0, S0=1
#define MUX_CH14  14  // S3=1, S2=1, S1=1, S0=0
#define MUX_CH15  15  // S3=1, S2=1, S1=1, S0=1
#endif



#ifndef MUX_COUNT
    #error "Please define MUX_COUNT before including mux_74hc40xx.h"
#endif

#if defined(USE_74HC4051)

    #ifndef MUX_SET1_PIN
        #error "MUX_SET1_PIN not defined (S0 pin required for 74HC4051)"
    #endif
    #ifndef MUX_SET2_PIN
        #error "MUX_SET2_PIN not defined (S1 pin required for 74HC4051)"
    #endif
    #ifndef MUX_SET3_PIN
        #error "MUX_SET3_PIN not defined (S2 pin required for 74HC4051)"
    #endif

#elif defined(USE_74HC4067)

    #ifndef MUX_SET1_PIN
        #error "MUX_SET1_PIN not defined (S0 pin required for 74HC4067)"
    #endif
    #ifndef MUX_SET2_PIN
        #error "MUX_SET2_PIN not defined (S1 pin required for 74HC4067)"
    #endif
    #ifndef MUX_SET3_PIN
        #error "MUX_SET3_PIN not defined (S2 pin required for 74HC4067)"
    #endif
    #ifndef MUX_SET4_PIN
        #error "MUX_SET4_PIN not defined (S3 pin required for 74HC4067)"
    #endif

#endif

#ifndef MUX_EN_PINS
    #error "Please define MUX_EN_PINS, e.g. #define MUX_EN_PINS {xx, xx, ...}"
#endif


void mux_init(void);
void select_mux_channel(uint8_t encode_byte);
void cancel_all_mux_channel(void);


