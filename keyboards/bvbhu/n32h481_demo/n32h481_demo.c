/* Copyright 2026 Talo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

/*
 * n32h481_demo — mechanical switch + hall sensor + three lamps.
 *
 * Custom matrix (CUSTOM_MATRIX = lite):
 *   [0,0] = KEY1/KEY2 slide switch (KEY2 driven low, KEY1 pulled up)
 *   [0,1] = hall sensor OUT on HALLO (auto-detected polarity)
 *
 * Custom rgb_matrix driver (RGB_MATRIX_DRIVER = custom):
 *   lamp0 = PC3, lamp1 = PA15, lamp2 = PB3 — all active-low, common
 *   anode to 5V, on/off only (no PWM).
 *
 * Indicators (per frame): lamp0 follows the switch, lamp1 follows the
 * hall, lamp2 lights when either input is active.
 */

#include "quantum.h"

#include "gpio.h"
#include "rgb_matrix.h"

static bool    hall_idle;   /* HALLO level when no magnet is near */
static uint8_t lamp_rgb[3][3]; /* last colors written to the lamps */

void matrix_init_custom(void) {
    /* Slide switch: drive KEY2 low, read KEY1 with pull-up. */
    gpio_set_pin_output(DEMO_KEY2_PIN);
    gpio_write_pin_low(DEMO_KEY2_PIN);
    gpio_set_pin_input_high(DEMO_KEY1_PIN);

    /* Hall sensor OUT with internal pull-up. */
    gpio_set_pin_input_high(DEMO_HALL_PIN);

    /* Plain board LED (LED+ -> diode -> 1k -> LED-): steady on = powered. */
    gpio_set_pin_output(DEMO_LEDP_PIN);
    gpio_write_pin_high(DEMO_LEDP_PIN);
    gpio_set_pin_output(DEMO_LEDM_PIN);
    gpio_write_pin_low(DEMO_LEDM_PIN);

    /* Lamps are active-low (common anode to 5V): start off. */
    gpio_set_pin_output(DEMO_LAMP0_PIN);
    gpio_write_pin_high(DEMO_LAMP0_PIN);
    gpio_set_pin_output(DEMO_LAMP1_PIN);
    gpio_write_pin_high(DEMO_LAMP1_PIN);
    gpio_set_pin_output(DEMO_LAMP2_PIN);
    gpio_write_pin_high(DEMO_LAMP2_PIN);

    /* Auto-detect hall idle level: majority of N samples. */
    uint8_t ones = 0;
    for (uint8_t i = 0; i < DEMO_HALL_SAMPLES; i++) {
        ones += gpio_read_pin(DEMO_HALL_PIN);
        wait_us(200);
    }
    hall_idle = (ones > (DEMO_HALL_SAMPLES / 2));
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool switch_pressed = (gpio_read_pin(DEMO_KEY1_PIN) == 0); /* low = shorted to KEY2 */
    bool hall_active    = (gpio_read_pin(DEMO_HALL_PIN) != hall_idle);

    matrix_row_t row = (switch_pressed ? (matrix_row_t)1 : 0) | (hall_active ? (matrix_row_t)2 : 0);
    bool changed     = (current_matrix[0] != row);
    current_matrix[0] = row;
    return changed;
}

/* ------------------------------------------------------------------ */
/* Custom rgb_matrix driver: three active-low lamp pins, on/off only.  */
/* ------------------------------------------------------------------ */

static void demo_rgb_init(void) {
    /* Lamps are already configured in matrix_init_custom. */
}

static void demo_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index >= 0 && index < 3) {
        lamp_rgb[index][0] = red;
        lamp_rgb[index][1] = green;
        lamp_rgb[index][2] = blue;
    }
}

static void demo_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < 3; i++) {
        demo_set_color(i, red, green, blue);
    }
}

static void demo_flush(void) {
    const pin_t lamps[3] = {DEMO_LAMP0_PIN, DEMO_LAMP1_PIN, DEMO_LAMP2_PIN};
    for (int i = 0; i < 3; i++) {
        bool on = ((lamp_rgb[i][0] | lamp_rgb[i][1] | lamp_rgb[i][2]) != 0);
        if (on) {
            gpio_write_pin_low(lamps[i]); /* active low */
        } else {
            gpio_write_pin_high(lamps[i]);
        }
    }
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = demo_rgb_init,
    .flush         = demo_flush,
    .set_color     = demo_set_color,
    .set_color_all = demo_set_color_all,
};

/* ------------------------------------------------------------------ */
/* Per-frame indicators: lamp0 = switch, lamp1 = hall, lamp2 = either. */
/* ------------------------------------------------------------------ */

bool rgb_matrix_indicators_kb(void) {
    bool switch_pressed = (gpio_read_pin(DEMO_KEY1_PIN) == 0);
    bool hall_active    = (gpio_read_pin(DEMO_HALL_PIN) != hall_idle);

    rgb_matrix_set_color(0, switch_pressed ? 0xFF : 0x08, 0x00, 0x00); /* lamp0: red */
    rgb_matrix_set_color(1, hall_active ? 0x00 : 0x08, 0xFF, 0x00);    /* lamp1: green */
    rgb_matrix_set_color(2, (switch_pressed || hall_active) ? 0x00 : 0x08, 0x00, 0xFF); /* lamp2: blue */
    return false;
}
