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

/* Vial 键盘唯一标识（stg/eave65 专用随机 UID，与 stg65_ec 的 UID 区分）。
 * eave65 的矩阵与物理布局虽然和 stg65_ec 相同，但作为独立型号使用独立 UID：
 * 两块板在 Vial 里是不同设备，布局文件（.vil）不通用。 */
#define VIAL_KEYBOARD_UID {0x64, 0x0F, 0xDE, 0x45, 0x7B, 0x27, 0x58, 0x77}

#define DYNAMIC_KEYMAP_LAYER_COUNT 4

#define VIAL_COMBO_ENTRIES 16
#define VIAL_TAP_DANCE_ENTRIES 16
#define VIAL_KEY_OVERRIDE_ENTRIES 16
