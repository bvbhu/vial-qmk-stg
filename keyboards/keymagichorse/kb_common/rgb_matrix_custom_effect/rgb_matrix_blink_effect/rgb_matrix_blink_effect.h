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

typedef struct {
    uint8_t index;          // LED 索引
    uint16_t blink_nums;    // 剩余闪烁次数（0 = 不闪）
    uint16_t on_time;       // 亮的时长 (ms)
    uint16_t off_time;      // 灭的时长 (ms)

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    // 内部状态
    uint8_t active;      // 是否激活
    uint8_t is_on;       // 当前亮灭状态
    uint16_t counter;    // 当前阶段计数器
} rgb_matrix_blink_t;

// 运行多少个灯闪烁，理论上一个就够了
#ifndef MAX_RGB_MATRIX_BLINK_TASKS
#   define MAX_RGB_MATRIX_BLINK_TASKS 1  
#endif

void rgb_matrix_blink_effect_init(void);
void rgb_matrix_blink(int index, uint8_t red, uint8_t green, uint8_t blue, uint16_t blink_nums, uint16_t on_ms, uint16_t off_ms) ;
void rgb_matrix_all_unblink(void) ;
// 丢到 rgb_matrix_indicators_advanced_user
bool rgb_matrix_blink_effect_hook(uint8_t led_min, uint8_t led_max);
