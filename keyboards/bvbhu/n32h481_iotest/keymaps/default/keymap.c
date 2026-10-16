/* Copyright 2026 Talo
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

#include QMK_KEYBOARD_H

/* N32H481REL7K 引脚虚焊测试固件
 *
 * 所有被测试的 IO 都排在矩阵第 0 行，列号 = keyboard.json 里 direct 数组的序号。
 * 键码从 26 个字母 + 10 个数字里循环挑选（36 个一组，不够就从头再取）；
 * 把某个排针短接到 GND，电脑就输入对应的字符 —— 有字符 = 该脚没虚焊。
 *
 * 布局坐标只用于给 QMK 一个合法的 LAYOUT 数组，无物理意义。
 *
 * 引脚顺序（与 keyboard.json 的 matrix_pins.direct 一致，共 86 个）：
 *   C0..C15, A0..A10, A15, B0..B13, E2..E15, D0..D15, F2..F15
 * 排除：H0/H1（32.768kHz 晶振）、B14/B15（用户指定）、A11/A12（USB）、
 *       A13/A14（SWDIO/SWCLK，接了键就不能 SWD 调试）。
 */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        /* C0..C15 */
        KC_A, KC_B, KC_C, KC_D, KC_E, KC_F, KC_G, KC_H,
        KC_I, KC_J, KC_K, KC_L, KC_M, KC_N, KC_O, KC_P,
        /* A0..A10, A15 */
        KC_Q, KC_R, KC_S, KC_T, KC_U, KC_V, KC_W, KC_X,
        KC_Y, KC_Z, KC_1, KC_2,
        /* B0..B13 */
        KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9,
        KC_0, KC_A, KC_B, KC_C, KC_D, KC_E,
        /* E2..E15 */
        KC_F, KC_G, KC_H, KC_I, KC_J, KC_K, KC_L,
        KC_M, KC_N, KC_O, KC_P, KC_Q, KC_R, KC_S, KC_T,
        /* D0..D15 */
        KC_U, KC_V, KC_W, KC_X, KC_Y, KC_Z, KC_1, KC_2,
        KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0,
        /* F2..F15 */
        KC_A, KC_B, KC_C, KC_D, KC_E, KC_F, KC_G, KC_H,
        KC_I, KC_J, KC_K, KC_L, KC_M, KC_N
    )
};
