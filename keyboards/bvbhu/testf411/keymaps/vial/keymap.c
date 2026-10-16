/* Copyright 2024 bvbhu
 *
 * Pin test board, Vial-only build.
 *
 * Every one of the 22 key pins is an input with an internal pull-up and counts
 * as "pressed" when it is shorted to GND.  Keycodes cycle through W / Q / E / R
 * so each pin produces a distinguishable, harmless keystroke.  There is no
 * function layer: A14 is just another test key.
 *
 * Key index -> pin (same order as matrix_pins.direct in keyboard.json):
 *   0 C14  1 C15  2 A0   3 A1   4 A2   5 A3   6 A4
 *   7 A5   8 A6   9 A7  10 B0  11 B1  12 B2  13 B10
 *  14 B13 15 B14 16 B15 17 A8  18 A9  19 A10 20 A13 21 A14
 *
 * B12 is NOT a key, it drives the 3 WS2812 LEDs.
 *
 * Lighting is switched from Vial (Lighting / RGB page) or by remapping one of
 * these keys to RGB_MOD / RGB_TOG etc. in Vial — nothing is hardcoded here.
 */
#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_W, KC_Q, KC_E, KC_R, KC_W, KC_Q, KC_E,
        KC_R, KC_W, KC_Q, KC_E, KC_R, KC_W, KC_Q,
        KC_E, KC_R, KC_W, KC_Q, KC_E, KC_R, KC_W,
        KC_Q
    ),

    /* Vial requires four layers; 1-3 are empty and only exist for dynamic
     * keymap editing. */
    [1] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______
    ),

    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______
    ),

    [3] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,
        _______
    )
};
