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
 * 被测试的 IO 排在 direct 矩阵里， keyboard.json 的 matrix_pins.direct 是 2 行：
 *   第 0 行 24 个脚： A0..A9, A15, B0..B5, B8, B9, C0..C4
 *   第 1 行 23 个脚： C5, D8..D14, E7..E14, F3, F4, F5, F7..F10（末位补 null/NO_PIN）
 * 之所以要拆成两行，是因为 QMK 的 matrix_row_t 最宽只有 uint32_t（MATRIX_COLS 上限 32），
 * 47 个脚放不进一行；同时 lib/python/qmk/constants.py 的 COL_LETTERS 在第 35 位开始重复 'i'，
 * 单行列数超过 ~36 会让生成的 LAYOUT 宏参数重名（duplicate macro parameter 'k0i'）。
 *
 * 键码从 26 个字母 + 10 个数字里循环挑选（36 个一组，不够就从头再取）；
 * 把某个排针短接到 GND，电脑就输入对应的字符 —— 有字符 = 该脚没虚焊。
 *
 * 布局坐标只用于给 QMK 一个合法的 LAYOUT 数组，无物理意义。
 *
 * 引脚顺序（与 keyboard.json 的 matrix_pins.direct 一致，共 47 个）：
 *   A0..A9, A15, B0..B5, B8, B9, C0..C5, D8..D14, E7..E14, F3, F4, F5, F7..F10
 *
 * 这 47 个脚是 LQFP64 上真实邦定、且不与板载必需功能冲突的脚。排除原因：
 *   H0 / H1    —— 8MHz 高速晶振 HSE（不是 32.768kHz！OSC_IN/OSC_OUT）
 *   B12 / B13  —— USB HS VBUS / OTG ID（usbhs_bsp 把它们配成了 USB 功能）
 *   B14 / B15  —— USB HS D- / D+（键盘通信必需）
 *   A13 / A14  —— SWDIO / SWCLK（保留 SWD 以便反复烧录）
 *   B7         —— BOOT0（硬件启动模式脚）
 *   NRST / VDD / VSS / VDDA / VSSA / VREF+ —— 非 IO
 * 本封装未邦定（写了也不存在，会读到幽灵电平）：
 *   A10 A11 A12、B6 B10 B11、C6..C15、D0..D7 D15、E0..E6 E15、F0 F1 F2 F6 F11..F15
 *   以及整个 GPIOG
 */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        /* 第 0 行：A0..A9, A15 */
        KC_A, KC_B, KC_C, KC_D, KC_E, KC_F, KC_G, KC_H,
        KC_I, KC_J, KC_K,
        /* 第 0 行：B0..B5, B8, B9 */
        KC_L, KC_M, KC_N, KC_O, KC_P, KC_Q, KC_R, KC_S,
        /* 第 0 行：C0..C4 */
        KC_T, KC_U, KC_V, KC_W, KC_X,

        /* 第 1 行：C5 */
        KC_Y,
        /* 第 1 行：D8..D14 */
        KC_Z, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6,
        /* 第 1 行：E7..E14 */
        KC_7, KC_8, KC_9, KC_0, KC_A, KC_B, KC_C, KC_D,
        /* 第 1 行：F3, F4, F5, F7..F10 */
        KC_E, KC_F, KC_G, KC_H, KC_I, KC_J, KC_K
    )
};
