/* Copyright 2024 keymagichorse
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

/* Vial 键盘唯一标识：**有意与 stg/stg65_ec(STM32F411) 保持同一个 UID**——
 * 两者是同一块 PCB、同一套矩阵/布局，只是主控不同，因此设备身份（VID 0x6829 /
 * PID 0x6501 / USB 产品名 STG65_EC）与这里 UID 全部保持一致，主机与 Vial 都视为
 * 同一台键盘，已有的布局与键位配置直接通用。 */
#define VIAL_KEYBOARD_UID {0x35, 0x27, 0x09, 0xFF, 0x9D, 0xE5, 0xC4, 0x2A}

#define DYNAMIC_KEYMAP_LAYER_COUNT 4

#define VIAL_COMBO_ENTRIES 16
#define VIAL_TAP_DANCE_ENTRIES 16
#define VIAL_KEY_OVERRIDE_ENTRIES 16
