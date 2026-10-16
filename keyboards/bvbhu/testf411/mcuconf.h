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

#include_next <mcuconf.h>

/* The generic STM32F411 board config assumes HSE = 8 MHz (Nucleo-F411RE).
 * Retarget the PLL to STM32_HSECLK so we get:
 *   VCO in  = HSE / PLLM   = 1 MHz
 *   VCO out = 1 MHz * 384  = 384 MHz   (spec: 100..432 MHz)
 *   SYSCLK  = VCO / PLLP   = 96 MHz    (F411 max 100 MHz)
 *   USB     = VCO / PLLQ   = 48 MHz
 */
#undef STM32_PLLM_VALUE
#undef STM32_PLLN_VALUE
#undef STM32_PLLP_VALUE
#undef STM32_PLLQ_VALUE

#define STM32_PLLM_VALUE (STM32_HSECLK / 1000000)
#define STM32_PLLN_VALUE 384
#define STM32_PLLP_VALUE 4
#define STM32_PLLQ_VALUE 8
