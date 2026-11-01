#include QMK_KEYBOARD_H
#include "bvbhu.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] =
{
	[0] = LAYOUT(
	    KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_MINS, KC_EQL,  F_13,
	    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_PGDN, KC_PGUP, KC_PSCR, KC_DEL,  KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
	    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_P7,   KC_P8,   KC_P9,   KC_LBRC, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_RBRC,
	    KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_P4,   KC_P5,   KC_P6,   KC_BSLS, KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
	    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_P1,   KC_P2,   KC_P3,   KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_ENT,
	    KC_LCTL, Fn,      KC_LGUI, KC_LALT, XXXXXXX, KC_SPC,  XXXXXXX, KC_P0,   KC_PDOT, XXXXXXX, RightSpace,    XXXXXXX, KC_LEFT, KC_UP,   KC_DOWN, KC_RGHT
	),

	[1] = LAYOUT(
	    _______,   _______, _______, _______, _______, _______, _______, _______, _______, _______,    _______, _______, _______,    S(KC_MINS), S(KC_EQL),  _______,
	    S(KC_GRV), S(KC_1), S(KC_2), S(KC_3), S(KC_4), S(KC_5), _______, _______, _______, _______,    S(KC_6), S(KC_7), S(KC_8),    S(KC_9),    S(KC_0),    _______,
	    _______,   XXXXXXX, C(KC_W), XXXXXXX, XXXXXXX, XXXXXXX, _______, _______, _______, S(KC_LBRC), RM_TOGG, KC_NUM,  KC_SCRL,    XXXXXXX,    G(KC_P),       S(KC_RBRC),
	    _______,   C(KC_A), C(KC_S), C(KC_D), C(KC_F), C(KC_G), _______, _______, _______, S(KC_BSLS), RM_VALU, XXXXXXX, XXXXXXX,    XXXXXXX,    S(KC_SCLN), S(KC_QUOT),
	    _______,   C(KC_Z), C(KC_X), C(KC_C), C(KC_V), C(KC_B), _______, _______, _______, C(S(KC_B)),  RM_VALD, C(KC_M), S(KC_COMM), S(KC_DOT),  S(KC_SLSH), _______,
	    _______,   _______, _______, _______, XXXXXXX, KC_SPC,  XXXXXXX, _______, _______, XXXXXXX,    _______, XXXXXXX, KC_LEFT,    KC_UP,      KC_DOWN,    KC_RGHT
	),

	[2] = LAYOUT(
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, KC_SPC,  _______, _______, _______, _______, KC_SPC,  _______, _______, _______, _______, _______
	),

	[3] = LAYOUT(
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, XXXXXXX, XXXXXXX, XXXXXXX, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, XXXXXXX, KC_MSTP, XXXXXXX, _______, _______, _______, _______, _______, _______, _______,
	    _______, _______, _______, _______, _______, _______, XXXXXXX, XXXXXXX, XXXXXXX, _______, _______, _______, _______, _______, _______, _______,
	    _______, XXXXXXX, XXXXXXX, XXXXXXX, _______, KC_SPC,  _______, XXXXXXX, XXXXXXX, _______, KC_SPC,  _______, _______, _______, _______, _______
	)
};

/* ---- 矩阵扫描率观测 ----
 * 每 10 秒打印一次核心维护的扫描率：get_matrix_scan_rate() 返回上一个 1 秒
 * 窗口内的扫描次数，即 scans/s。依赖 config.h 的 DEBUG_MATRIX_SCAN_RATE。
 * 必须用 uprintf：Vial 构建强制 -DNO_DEBUG，dprintf 会被编译期删除。 */
void housekeeping_task_user(void)
{
    static uint32_t last_ms = 0;

    const uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, last_ms) >= 10000)
    {
        last_ms = now;
        uprintf("matrix scan rate: %lu/s\n", (unsigned long)get_matrix_scan_rate());
    }
}

