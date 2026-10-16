/* Copyright 2024 bvbhu
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

/* Three WS2812 LEDs on B12 (bitbang driver, see keyboard.json).
 * Light them up right after boot so the data line / wiring can be checked
 * without needing to press anything. */
#define RGB_MATRIX_DEFAULT_ON true
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_ALL
#define RGB_MATRIX_DEFAULT_HUE 0
#define RGB_MATRIX_DEFAULT_SAT 255
#define RGB_MATRIX_DEFAULT_VAL 120
#define RGB_MATRIX_DEFAULT_SPD 128

#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
#define RGB_MATRIX_HUE_STEP 8
#define RGB_MATRIX_SAT_STEP 16
#define RGB_MATRIX_VAL_STEP 16
#define RGB_MATRIX_SPD_STEP 16

/* The 22 key pins are inputs with internal pull-ups; a key is "pressed"
 * when the pin is shorted to GND.  (List is informational only — the pin
 * order actually used comes from matrix_pins.direct in keyboard.json.) */
