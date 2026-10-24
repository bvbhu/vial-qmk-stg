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
 * 被测试的 IO 排在 direct 矩阵里，keyboard.json 的 matrix_pins.direct 是 2 行：
 *   第 0 行 24 个脚： A0..A9, A15, B0..B5, B8, B9, C0..C4
 *   第 1 行 23 个脚： C5, D8..D14, E7..E14, F3, F4, F5, F7..F10（末位补 null/NO_PIN）
 * 之所以要拆成两行，是因为 QMK 的 matrix_row_t 最宽只有 uint32_t（MATRIX_COLS 上限 32），
 * 47 个脚放不进一行；同时 lib/python/qmk/constants.py 的 COL_LETTERS 在第 35 位开始重复 'i'，
 * 单行列数超过 ~36 会让生成的 LAYOUT 宏参数重名（duplicate macro parameter 'k0i'）。
 *
 * 键码分配：47 个脚各占一个唯一键码，用「大小写」区分，PC 上直接看到 A..Z / a..u：
 *   第 0..25 个脚（按 LAYOUT 顺序）→ S(KC_A) .. S(KC_Z)  → 输出大写 A..Z
 *   第 26..46 个脚                → KC_A  .. KC_U       → 输出小写 a..u
 *   S(kc) 是 QMK 的 Shift+键 宏（quantum/quantum_keycodes.h:79 `#define S(kc) LSFT(kc)`），
 *   所以大写 = 按住 Shift 输入该字母。
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

/* 脚位 -> 键码 的对应表（顺序 = LAYOUT 参数顺序，47 个）
 *
 * 序号 | 引脚 | 键码   | PC 上看到
 * -----+------+--------+----------
 *   0  | A0   | S(KC_A)| A
 *   1  | A1   | S(KC_B)| B
 *   2  | A2   | S(KC_C)| C
 *   3  | A3   | S(KC_D)| D
 *   4  | A4   | S(KC_E)| E
 *   5  | A5   | S(KC_F)| F
 *   6  | A6   | S(KC_G)| G
 *   7  | A7   | S(KC_H)| H
 *   8  | A8   | S(KC_I)| I
 *   9  | A9   | S(KC_J)| J
 *  10  | A15  | S(KC_K)| K
 *  11  | B0   | S(KC_L)| L
 *  12  | B1   | S(KC_M)| M
 *  13  | B2   | S(KC_N)| N
 *  14  | B3   | S(KC_O)| O
 *  15  | B4   | S(KC_P)| P
 *  16  | B5   | S(KC_Q)| Q
 *  17  | B8   | S(KC_R)| R
 *  18  | B9   | S(KC_S)| S
 *  19  | C0   | S(KC_T)| T
 *  20  | C1   | S(KC_U)| U
 *  21  | C2   | S(KC_V)| V
 *  22  | C3   | S(KC_W)| W
 *  23  | C4   | S(KC_X)| X
 *  24  | C5   | S(KC_Y)| Y
 *  25  | D8   | S(KC_Z)| Z
 *  26  | D9   | KC_A   | a
 *  27  | D10  | KC_B   | b
 *  28  | D11  | KC_C   | c
 *  29  | D12  | KC_D   | d
 *  30  | D13  | KC_E   | e
 *  31  | D14  | KC_F   | f
 *  32  | E7   | KC_G   | g
 *  33  | E8   | KC_H   | h
 *  34  | E9   | KC_I   | i
 *  35  | E10  | KC_J   | j
 *  36  | E11  | KC_K   | k
 *  37  | E12  | KC_L   | l
 *  38  | E13  | KC_M   | m
 *  39  | E14  | KC_N   | n
 *  40  | F3   | KC_O   | o
 *  41  | F4   | KC_P   | p
 *  42  | F5   | KC_Q   | q
 *  43  | F7   | KC_R   | r
 *  44  | F8   | KC_S   | s
 *  45  | F9   | KC_T   | t
 *  46  | F10  | KC_U   | u
 */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        /* 第 0 行：A0..A9, A15, B0..B5, B8, B9, C0..C4 —— 大写 A..X */
        S(KC_A), S(KC_B), S(KC_C), S(KC_D), S(KC_E), S(KC_F), S(KC_G), S(KC_H),
        S(KC_I), S(KC_J), S(KC_K),
        S(KC_L), S(KC_M), S(KC_N), S(KC_O), S(KC_P), S(KC_Q), S(KC_R), S(KC_S),
        S(KC_T), S(KC_U), S(KC_V), S(KC_W), S(KC_X),

        /* 第 1 行：C5, D8 —— 大写 Y, Z */
        S(KC_Y),
        S(KC_Z),
        /* 第 1 行：D9..D14 —— 小写 a..f */
        KC_A, KC_B, KC_C, KC_D, KC_E, KC_F,
        /* 第 1 行：E7..E14 —— 小写 g..n */
        KC_G, KC_H, KC_I, KC_J, KC_K, KC_L, KC_M, KC_N,
        /* 第 1 行：F3, F4, F5, F7..F10 —— 小写 o..u */
        KC_O, KC_P, KC_Q, KC_R, KC_S, KC_T, KC_U
    )
};
