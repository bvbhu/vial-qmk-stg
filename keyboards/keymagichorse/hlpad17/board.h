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

#include_next <board.h>
// at32的board.h里面默认宏定义了一个AT32F415KB
// 取消后重新选择自己用的
#undef AT32F415KB
#define AT32F415CC
// board.h
#undef  AT32_HEXTCLK
#define AT32_HEXTCLK 16000000
// #define AT32_HEXTCLK 8000000