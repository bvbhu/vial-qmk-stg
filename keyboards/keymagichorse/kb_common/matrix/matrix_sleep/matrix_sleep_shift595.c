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
#include "matrix.h"
#include "quantum.h"
#include "atomic_util.h"
#include "gpio.h"
#include "matrix_sleep.h"

#if SHIFT595_ENABLED
#   include "74hc595.h"
#endif

// use for config wakeUp Pin
static const pin_t wakeUpRow_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const pin_t wakeUpCol_pins[MATRIX_COLS]   = MATRIX_COL_PINS;

void matrix_sleepConfig(void)
{

#if SHIFT595_ENABLED
    shift595_write_all(0x00);
    shift595_pin_sleep();
#endif
#if SHIFT595_ENABLED
    shift595_pin_sleep();
#endif

    uint8_t i = 0;
#if (DIODE_DIRECTION == COL2ROW)
    // Set row(low valid), read cols
    for (i = 0; i < matrix_cols(); i++)
    { // set col pull-up input
        if(wakeUpCol_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_input_high(wakeUpCol_pins[i]);
            palEnableLineEvent(wakeUpCol_pins[i], PAL_EVENT_MODE_FALLING_EDGE);
        }
    }
    for (i = 0; i < matrix_rows(); i++)
    { // set row output low level
        if(wakeUpRow_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_output(wakeUpRow_pins[i]);
            gpio_write_pin_low(wakeUpRow_pins[i]);
        }
    }
#elif (DIODE_DIRECTION == ROW2COL)

    // Set col(low valid), read rows
    for (i = 0; i < matrix_rows(); i++)
    { // set row pull-up input
        if(wakeUpRow_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_input_high(wakeUpRow_pins[i]);
            palEnableLineEvent(wakeUpRow_pins[i], PAL_EVENT_MODE_FALLING_EDGE);
        }
    }

    for (i = 0; i < matrix_cols(); i++)
    { // set col output low level
        if(wakeUpCol_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_output(wakeUpCol_pins[i]);
            gpio_write_pin_low(wakeUpCol_pins[i]);
        }
    }

#endif

}

void matrix_rtc_Config(void)
{
    shift595_write_all(0x00);
    shift595_pin_sleep();

    uint8_t i = 0;
#if (DIODE_DIRECTION == COL2ROW)
    for (i = 0; i < matrix_rows(); i++)
    { // set row output low level
        if(wakeUpRow_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_output(wakeUpRow_pins[i]);
            gpio_write_pin_low(wakeUpRow_pins[i]);
        }
    }
#elif (DIODE_DIRECTION == ROW2COL)
    for (i = 0; i < matrix_cols(); i++)
    { // set col output low level
        if(wakeUpCol_pins[i] == NO_PIN)
        {
            continue;
        } 
        ATOMIC_BLOCK_FORCEON {
            gpio_set_pin_output(wakeUpCol_pins[i]);
            gpio_write_pin_low(wakeUpCol_pins[i]);
        }
    }
#endif
}
