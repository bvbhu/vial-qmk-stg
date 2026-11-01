/* Copyright 2024 bvbhu
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

/* F411 走的是通用板 GENERIC_STM32_F411XE，它的 board.h 是 Nucleo-F411RE 的，
 * 默认按 HSE = 8 MHz 计算时钟树。本板晶振不是 8 MHz，必须在这里改写：
 *   16 MHz（与 bvbhu 其他 F411 板一致，默认）
 *   25 MHz（若本板实际焊的是 25 MHz，注释掉 16000000、改用 25000000）
 * 填错会导致 SYSCLK/PLL 全错，USB 枚举不了（或根本起不来）。 */
#undef STM32_HSECLK
#define STM32_HSECLK 16000000
// #define STM32_HSECLK 25000000
