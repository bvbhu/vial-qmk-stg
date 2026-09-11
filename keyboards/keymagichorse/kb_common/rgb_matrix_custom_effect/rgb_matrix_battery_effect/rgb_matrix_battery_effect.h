/* Copyright 2025 keymagichorse
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
#include "color.h"

// 显示电量时 是否熄灭其他的rgb灯
#ifndef RGB_MATRIX_SHOW_BATTERY_RGB_OTHER_RGB_BLACK
#   define RGB_MATRIX_SHOW_BATTERY_RGB_OTHER_RGB_BLACK     1
#endif

// 显示电量百分比的 rgb
#ifndef RGB_MATRIX_BATTERY_SHOW_RGB_INDEXX
#   define RGB_MATRIX_BATTERY_SHOW_RGB_INDEXX   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
#endif

// 电量高中低的颜色
#ifndef RGB_MATRIX_BATTERY_HIGH_COLOR
#   define RGB_MATRIX_BATTERY_HIGH_COLOR    RGB_GREEN
#endif

#ifndef RGB_MATRIX_BATTERY_MEDIUM_COLOR
#   define RGB_MATRIX_BATTERY_MEDIUM_COLOR  RGB_YELLOW
#endif

#ifndef RGB_MATRIX_BATTERY_LOW_COLOR
#   define RGB_MATRIX_BATTERY_LOW_COLOR     RGB_RED
#endif


void rgb_matrix_battery_effect_init(void);
void rgb_matrix_battery_effect_enabled(void);
void rgb_matrix_battery_effect_disabled(void);
bool rgb_matrix_battery_effect_hook(uint8_t led_min, uint8_t led_max);
