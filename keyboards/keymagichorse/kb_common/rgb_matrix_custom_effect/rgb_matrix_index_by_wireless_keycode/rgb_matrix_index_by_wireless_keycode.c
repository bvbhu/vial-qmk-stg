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
#include "rgb_matrix_index_by_wireless_keycode.h"
#include "rgb_matrix.h"
#include "keymap_introspection.h"
#include "bhq_common.h"
/* =====================================================
 * Global Array
 * ===================================================== */

wireless_key_rgb_index_t
g_wireless_key_rgb_index_list[WIRELESS_KEY_MAX];


/* =====================================================
 * Internal: scan keymap and locate keys
 * ===================================================== */

static void wireless_key_rgb_index_find( wireless_key_rgb_index_t *find_list, uint8_t list_size)
{
    uint8_t layer_count = keymap_layer_count();

    for (uint8_t l = 0; l < layer_count; l++) {
        for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {

                keypos_t key = {
                    .row = r,
                    .col = c
                };

                uint16_t keycode =
                    keymap_key_to_keycode(l, key);

                if (keycode > OU_AUTO) {

                    for (uint8_t i = 0; i < list_size; i++) {

                        if (find_list[i].keycode == keycode)
                        {
                            find_list[i].row = r;
                            find_list[i].col = c;
                        }
                    }
                }
            }
        }
    }

    /* row/col -> rgb matrix index */
    for (uint8_t i = 0; i < list_size; i++) {
        find_list[i].rgb_matrix_index = g_led_config.matrix_co[find_list[i].row][find_list[i].col];
    }
}

void wireless_keycode_rgb_index_refresh(void)
{
    wireless_key_rgb_index_find( g_wireless_key_rgb_index_list, WIRELESS_KEY_MAX);
}

void wireless_keycode_rgb_index_init(void)
{
    for (uint8_t i = 0; i < WIRELESS_KEY_MAX; i++) {
        g_wireless_key_rgb_index_list[i].row = 0;
        g_wireless_key_rgb_index_list[i].col = 0;
        g_wireless_key_rgb_index_list[i].rgb_matrix_index = 0;
    }

    /* set keycodes */
    g_wireless_key_rgb_index_list[0].keycode   = RF_TOG;
    g_wireless_key_rgb_index_list[1].keycode  = USB_TOG;
    g_wireless_key_rgb_index_list[2].keycode = BLE_SW1;
    g_wireless_key_rgb_index_list[3].keycode = BLE_SW2;
    g_wireless_key_rgb_index_list[4].keycode = BLE_SW3;

    wireless_key_rgb_index_find( g_wireless_key_rgb_index_list, WIRELESS_KEY_MAX);
}


// 根据按键值 获取 rgb矩阵灯的索引
uint16_t rgb_matrix_index_by_wireless_keycode(uint16_t keycode)
{
    for (uint8_t i = 0; i < WIRELESS_KEY_MAX; i++)
    {
        if (g_wireless_key_rgb_index_list[i].keycode == keycode)
        {
            return g_wireless_key_rgb_index_list[i].rgb_matrix_index;
        }
    }

    return 0;
}
