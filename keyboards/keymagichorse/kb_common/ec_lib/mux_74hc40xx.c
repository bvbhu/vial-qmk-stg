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
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "gpio.h"
#include "mux_74hc40xx.h"
pin_t mux_en_pins[MUX_COUNT] = MUX_EN_PINS;
// init
void mux_init(void)
{
    uint8_t i = 0;

    // init en, pin model out and high
    for (i = 0; i < MUX_COUNT; i++)
    {
        setPinOutput(mux_en_pins[i]);
        writePinHigh(mux_en_pins[i]);
    }

    // init set_xx, pin model out and low
#   if defined(USE_74HC4051)
        // 74HC4051
        setPinOutput(MUX_SET1_PIN);
        setPinOutput(MUX_SET2_PIN);
        setPinOutput(MUX_SET3_PIN);

        writePinLow(MUX_SET1_PIN);
        writePinLow(MUX_SET2_PIN);
        writePinLow(MUX_SET3_PIN);

#   elif defined(USE_74HC4067)
        // 74HC4067
        setPinOutput(MUX_SET1_PIN);
        setPinOutput(MUX_SET2_PIN);
        setPinOutput(MUX_SET3_PIN);
        setPinOutput(MUX_SET4_PIN);

        writePinLow(MUX_SET1_PIN);
        writePinLow(MUX_SET2_PIN);
        writePinLow(MUX_SET3_PIN);
        writePinLow(MUX_SET4_PIN);
#   endif
}
static void mux_en_all_high(void)
{
    for (uint8_t i = 0; i < MUX_COUNT; i++)
    {
        writePinHigh(mux_en_pins[i]);
    }
}
static void mux_set_all_low(void)
{
#if defined(USE_74HC4051)
    writePinLow(MUX_SET1_PIN);
    writePinLow(MUX_SET2_PIN);
    writePinLow(MUX_SET3_PIN);
#elif defined(USE_74HC4067)
    writePinLow(MUX_SET1_PIN);
    writePinLow(MUX_SET2_PIN);
    writePinLow(MUX_SET3_PIN);
    writePinLow(MUX_SET4_PIN);
#endif
}

void select_mux_channel(uint8_t encode_byte)
{
    mux_set_all_low();
    uint8_t ch =  MUX_GET_CHANNEL(encode_byte);
#   if defined(USE_74HC4051)
        // 74HC4051
        writePin(MUX_SET1_PIN, (ch >> 0) & 1);
        writePin(MUX_SET2_PIN, (ch >> 1) & 1);
        writePin(MUX_SET3_PIN, (ch >> 2) & 1);
#   elif defined(USE_74HC4067)
        // 74HC4067
        writePin(MUX_SET1_PIN, (ch >> 0) & 1);
        writePin(MUX_SET2_PIN, (ch >> 1) & 1);
        writePin(MUX_SET3_PIN, (ch >> 2) & 1);
        writePin(MUX_SET4_PIN, (ch >> 3) & 1);
#   endif

    uint8_t mux_i = MUX_GET_INDEX(encode_byte);
    // all en high
    mux_en_all_high();
    // select mux_i low
    writePinLow(mux_en_pins[mux_i]);
}
// 取消所有mux
void cancel_all_mux_channel(void)
{
    mux_set_all_low();
    // all en high
    mux_en_all_high();
}
// 74HC4051
// 74HC4067


