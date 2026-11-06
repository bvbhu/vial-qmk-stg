/* Copyright 2026 Talo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

// #define HAL_USE_ADC TRUE

#define HAL_USE_USB TRUE

/* ChibiOS SERIAL driver (SD2 = USART2).  Enabled here so the N32 USARTv1
   LLD is actually compiled and exercised by the bvbhu/n32h481_iotest:default
   build.  Pin muxing is left to the board/user code; call sdStart(&SD2, NULL)
   for 38400 8N1 before using the driver. */
#define HAL_USE_SERIAL TRUE

#include_next <halconf.h>