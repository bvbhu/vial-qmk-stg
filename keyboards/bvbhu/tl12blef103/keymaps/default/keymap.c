/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * tl12blef103 默认 keymap
 *
 * LAYOUT 参数顺序 (与 keyboard.json layout 一致):
 *   [0,0] [0,1] [0,2] [0,3] [0,4] [0,5]
 *   [1,0] [1,1] [1,2] [1,3] [1,4] [1,5]
 *
 * Layer 0 (主层): A-E 为输出键, 空格 / 回车为 MO(1) —— 按下 (按住) 即切到
 *                 Layer 1 (功能层), 松开回到本层。
 * Layer 1 (功能层): 有线 USB_TOG / 蓝牙 BLE_SW1..3 / 2.4G RF_TOG /
 *                 NKRO 切换 NK_TOGG, 全部集中在此层; A-E 与空格位沿
 *                 KC_TRNS 下落透传到 Layer 0。
 */
#include QMK_KEYBOARD_H

#include "config.h"
#include "via.h"
#include "bhq_common.h"
#include "wireless.h"
#include "transport.h"
#include "report_buffer.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* 主层: 12 键 = 11 个字母 A-K + 空格位 [1,5]=MO(1) (按住切 Layer 1) */
    [0] = LAYOUT(
        KC_A,    KC_B,    KC_C,    KC_D,    KC_E,    KC_F,
        KC_G,    KC_H,    KC_I,    KC_J,    KC_K,    MO(1)
    ),

    /* 功能层: 有线 / 蓝牙 3 槽 / 2.4G / NKRO 切换 */
    [1] = LAYOUT(
        USB_TOG, BLE_SW1, BLE_SW2, BLE_SW3, RF_TOG, NK_TOGG,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
