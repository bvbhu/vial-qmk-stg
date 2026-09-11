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
#include "quantum.h"
#include "rgb_matrix.h"
#include "rgb_matrix_battery_effect.h"
#include "battery.h"
#include "color.h"

// 是否显示rgb电量指示灯
uint8_t rgb_bat_show_flag  = 0;
uint16_t rgb_bat_led_idx[10] = RGB_MATRIX_BATTERY_SHOW_RGB_INDEXX;

uint8_t rgb_bat_high_color[3]   = {RGB_MATRIX_BATTERY_HIGH_COLOR};
uint8_t rgb_bat_medium_color[3] = {RGB_MATRIX_BATTERY_MEDIUM_COLOR};
uint8_t rgb_bat_low_color[3]    = {RGB_MATRIX_BATTERY_LOW_COLOR};

void rgb_matrix_battery_effect_init(void)
{
    rgb_bat_show_flag = 0;
}

void rgb_matrix_battery_effect_enabled(void)
{
    rgb_bat_show_flag = 1;
}

void rgb_matrix_battery_effect_disabled(void)
{
    rgb_bat_show_flag = 0;
}
bool rgb_matrix_battery_effect_hook(uint8_t led_min, uint8_t led_max)
{
    if(rgb_bat_show_flag == 1)   
    {
        
#if RGB_MATRIX_SHOW_BATTERY_RGB_OTHER_RGB_BLACK == 1
        for (size_t i = 0; i < RGB_MATRIX_LED_COUNT; i++)
        {
            rgb_matrix_set_color(i, RGB_BLACK);
        }
#endif

        uint8_t bat_led_count = battery_driver_sample_percent() / 10;
        if (battery_driver_sample_percent() > 0 && bat_led_count == 0) {
            bat_led_count = 1;  
        }
        if (bat_led_count > 10) {
            bat_led_count = 10; 
        }
        uint8_t r = 0, g = 0, b = 0;
        if (battery_driver_sample_percent() < 30) {
            r = rgb_bat_low_color[0];
            g = rgb_bat_low_color[1];
            b = rgb_bat_low_color[2];
        } else if (battery_driver_sample_percent() < 70) {
            r = rgb_bat_medium_color[0];
            g = rgb_bat_medium_color[1];
            b = rgb_bat_medium_color[2];
        } else {
            r = rgb_bat_high_color[0];
            g = rgb_bat_high_color[1];
            b = rgb_bat_high_color[2];
        }
        for (size_t j = 0; j < bat_led_count; j++) {
            uint8_t led_index = rgb_bat_led_idx[j];
            if (led_index < RGB_MATRIX_LED_COUNT) {
                rgb_matrix_set_color(led_index, r, g, b);
            }
        }
        return false;
    }
    return false;
}
