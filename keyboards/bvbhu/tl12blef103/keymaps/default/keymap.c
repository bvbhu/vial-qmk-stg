/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * tl12blef103 默认 keymap
 *
 * LAYOUT 参数顺序 (与 keyboard.json layout 一致):
 *   [0,0] [0,1] [0,2] [0,3] [0,4] [0,5]
 *   [1,0] [1,1] [1,2] [1,3] [1,4] [1,5]
 *
 * Layer 0 直接放蓝牙测试键, 其余键为可见输出键, 方便逐键验证上报。
 */
#include QMK_KEYBOARD_H

#include "config.h"
#include "via.h"
#include "bhq_common.h"
#include "wireless.h"
#include "transport.h"
#include "report_buffer.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* USB Host / BT1 / BT2 / BT3 / 电量 / 2.4G Host / A B C D E 空格 */
    [0] = LAYOUT(
        USB_TOG, BLE_SW1, BLE_SW2, BLE_SW3, BAT_INFO, RF_TOG,
        KC_A,    KC_B,    KC_C,    KC_D,    KC_E,     KC_SPACE
    ),

    [1] = LAYOUT(
        KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6,
        KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12
    ),
};
