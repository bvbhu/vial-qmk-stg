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

#include_next <board.h>

/* STM32F072 板上高速晶振频率。
 * 该值只在 mcuconf.h 打开 HSE（STM32_HSE_ENABLED TRUE + STM32_PLLSRC_HSE）时参与
 * PLL 计算；默认 mcuconf.h 走 HSI/2 x12 = 48MHz，不依赖外部晶振，因此此处即使与
 * 实物不符也不影响启动。 */
#undef STM32_HSECLK
#define STM32_HSECLK 16000000
