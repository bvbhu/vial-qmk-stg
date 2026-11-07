/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tl12wlh481 default keymap -- identical layout to bvbhu/tl12blef103.
 *
 * LAYOUT argument order (matches keyboard.json):
 *   [0,0] [0,1] [0,2] [0,3] [0,4] [0,5]
 *   [1,0] [1,1] [1,2] [1,3] [1,4] [1,5]
 *
 * Layer 0: A-K and MO(1) on the 7u spacebar position.
 * Layer 1: USB_TOG / BLE_SW1..3 / RF_TOG / NK_TOGG.
 */
#include QMK_KEYBOARD_H

#include "config.h"
#include "via.h"
#include "bhq_common.h"
#include "wireless.h"
#include "transport.h"
#include "report_buffer.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_A,    KC_B,    KC_C,    KC_D,    KC_E,    KC_F,
        KC_G,    KC_H,    KC_I,    KC_J,    KC_K,    MO(1)
    ),

    [1] = LAYOUT(
        USB_TOG, BLE_SW1, BLE_SW2, BLE_SW3, RF_TOG, NK_TOGG,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
