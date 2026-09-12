#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
	KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,  KC_F5,    KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,  KC_BSPC,
	KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_HOME, KC_END,  KC_PSLS,
	KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSLS, KC_P7,   KC_P8,   KC_P9,   KC_PAST,
	KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_P4,   KC_P5,   KC_P6,   KC_PMNS,
	KC_LSFT, KC_SLSH, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_UP,   KC_P1,   KC_P2,   KC_P3,   KC_PPLS,
	KC_LCTL, MO(1),   KC_LGUI, KC_LALT, KC_SPC,  KC_SPC,  KC_SPC,  KC_APP,  KC_LBRC, KC_RBRC, KC_LEFT, KC_DOWN, KC_RGHT, KC_P0,   KC_PDOT, KC_ENT
	),

    [1] = LAYOUT(
	RM_TOGG, RM_NEXT, RM_VALU, RM_SPDU, RGB_M_R, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
	KC_TRNS, RM_PREV, RM_VALD, RM_SPDD, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
	KC_TRNS, KC_TRNS, RM_HUEU, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_HOME, KC_TRNS, KC_PGUP, KC_TRNS,
	KC_TRNS, RM_SATD, RM_HUED, RM_SATU, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NUM,  KC_TRNS, KC_TRNS,
	KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_END,  KC_TRNS, KC_PGDN, KC_TRNS,
	KC_TRNS, MO(1),   KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_MUTE, KC_VOLD, KC_VOLU, KC_TRNS, KC_TRNS, KC_TRNS, QK_BOOT, KC_DEL,  KC_PENT
	),

    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [3] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    )
};

// bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
// {
//     if (host_keyboard_led_state().caps_lock)
//     {	//CapsLock 锁定时强制 32 号 LED 的颜色
//         RGB_MATRIX_INDICATOR_SET_COLOR(47, 185, 0, 0);
//     }
//     else
//     {	//CapsLock 解锁时强制 32 号 LED 的颜色
//     }

//     if (host_keyboard_led_state().num_lock)
//     {	//NumLock 锁定时强制 45 号 LED 的颜色
//     }
//     else
//     {	//NumLock 解锁时强制 45 号 LED 的颜色
//         RGB_MATRIX_INDICATOR_SET_COLOR(34, 185, 0, 0);
//     }

//     switch (get_highest_layer(layer_state))
//     {
//         case 0:
//             break;
//         case 1:
//                 RGB_MATRIX_INDICATOR_SET_COLOR(14,  0,175,175);
//             break;
//         default:
//             break;
//     }
//     return false;
// }
