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
#pragma once

#include "quantum.h"


typedef struct {
    uint16_t keycode;
    uint8_t  row;
    uint8_t  col;
    uint16_t rgb_matrix_index;
} wireless_key_rgb_index_t;

#define WIRELESS_KEY_MAX 5
extern wireless_key_rgb_index_t g_wireless_key_rgb_index_list[WIRELESS_KEY_MAX];


void wireless_keycode_rgb_index_init(void);
void wireless_keycode_rgb_index_refresh(void);

/* keycode -> rgb matrix index */
uint16_t rgb_matrix_index_by_wireless_keycode(uint16_t keycode);

