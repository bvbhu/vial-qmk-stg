/* Copyright 2021 @ Keychron (https://www.keychron.com)
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
#include <string.h>
#include "atomic_util.h"
#include "config.h"
#include "74hc595.h"
#include "wait.h"
#include "timer.h"

#ifdef MATRIX_ROW_PINS
static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
#endif // MATRIX_ROW_PINS
#ifdef MATRIX_COL_PINS
static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;
#endif // MATRIX_COL_PINS


#define ROWS_PER_HAND (MATRIX_ROWS)

static inline void gpio_atomic_write_pin_low(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_write_pin_low(pin);
    }
}

static inline void gpio_atomic_write_pin_high(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_write_pin_high(pin);
    }
}

static inline void gpio_atomic_set_pin_output_low(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_output(pin);
        gpio_write_pin_low(pin);
    }
}

static inline void gpio_atomic_set_pin_input_high(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_input_high(pin);
    }
}

static inline uint8_t readMatrixPin(pin_t pin) {
    if (pin != NO_PIN) {
        return gpio_read_pin(pin);
    } else {
        return 1;
    }
}

static bool select_col(uint8_t col) {
    pin_t pin = col_pins[col];

    if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
        gpio_atomic_write_pin_low(pin);
#else
        gpio_atomic_set_pin_output_low(pin);
#endif
        return true;
    }
    return false;
}

static void unselect_col(uint8_t col) {
    pin_t pin = col_pins[col];

    if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
        gpio_atomic_write_pin_high(pin);
#else
        gpio_atomic_set_pin_input_high(pin);
#endif
    } 
}

static void unselect_cols(void) {
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        pin_t pin = col_pins[x];
        if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
            gpio_atomic_set_pin_output_high(pin);
#else
            gpio_atomic_set_pin_input_high(pin);
#endif
        }
    }
    shift595_write_all_high();
}

static inline void shift595_bit(bool bit)
{
    if (bit)
    {
        gpio_write_pin_high(DS_PIN_74HC595);
    }
    else
    {
        gpio_write_pin_low(DS_PIN_74HC595);
    }

    gpio_write_pin_high(SHCP_PIN_74HC595);
    gpio_write_pin_low(SHCP_PIN_74HC595);

    gpio_write_pin_high(STCP_PIN_74HC595);
    gpio_write_pin_low(STCP_PIN_74HC595);
}

static void matrix_init_pins(void) {
    shift595_pin_init();
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        if (col_pins[x] != NO_PIN) {
            gpio_set_pin_output(col_pins[x]);
        }
    }
#endif
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        if (row_pins[x] != NO_PIN) {
            gpio_atomic_set_pin_input_high(row_pins[x]);
        }
    }
    unselect_cols();
}

static void matrix_read_rows_on_col(matrix_row_t current_matrix[], uint8_t col)
{
    matrix_row_t row_shifter = ((matrix_row_t)1 << col);

    for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++)
    {
        if (readMatrixPin(row_pins[row_index]) == 0)
            current_matrix[row_index] |= row_shifter;
        else
            current_matrix[row_index] &= ~row_shifter;
    }
}
uint32_t m_t = 0;

void matrix_init_custom(void) {
    // initialize key pins
    matrix_init_pins();
    m_t = timer_read32();
}


// 还是通过col扫描row
// 1、 局部变量 每一行都需要刷新一次 定义一个col 有没有扫描过的列表  
// 2、 先把 COL_TO_74HC595_PINS_COUNT COL_TO_74HC595_PINS是74hc59 扫描一次
// 3、 最后在看一下有没有哪一个col没被扫描过 如果有没被扫描过的那就扫一次，已经被扫描过的就不操作io口
bool matrix_scan_custom(matrix_row_t current_matrix[])
{
    
    // if (timer_elapsed32(m_t) < 10) 
    // {
    //     return false;
    // }
    m_t = timer_read32();


    bool is_scan_col[MATRIX_COLS] = {false};
    matrix_row_t curr_matrix[MATRIX_ROWS] = {0};

    uint8_t num_585 = I_595_NUM * 8;

    // 压一个0进去
    shift595_bit(0);
    for (uint8_t i = 0; i < num_585; i++)
    {
        // wait_us(1);

        if (i < COL_TO_74HC595_PINS_COUNT)
        {
            int8_t col = HC595_PIN_TO_COL[i];
            // 校验col是否合法
            if (col >= 0 && col < MATRIX_COLS && col != 0xff)
            {
                is_scan_col[col] = true;

                matrix_read_rows_on_col(curr_matrix, col);
            }
        }
        // 读取完 压一个1出去
        shift595_bit(1);
    }

    /* ---- normal GPIO columns ---- */

    for (uint8_t col = 0; col < MATRIX_COLS; col++)
    {
        if (!is_scan_col[col])
        {
            if (!select_col(col))
                continue;

            // wait_us(1);

            matrix_read_rows_on_col(curr_matrix, col);

            unselect_col(col);
        }
    }

    bool changed = memcmp(current_matrix, curr_matrix, sizeof(curr_matrix)) != 0;

    if (changed)
        memcpy(current_matrix, curr_matrix, sizeof(curr_matrix));

    return changed;
}
