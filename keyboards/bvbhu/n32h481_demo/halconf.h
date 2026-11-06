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

#define HAL_USE_USB TRUE

/* Vial Analog: PA4 (ADC1_IN4) samples the linear Hall sensor.  The generic
   halconf.h template already defaults HAL_USE_ADC to TRUE and common_features.mk
   adds -DHAL_USE_ADC=TRUE when ANALOG_MODEL is set; pin it explicitly here so
   the board does not silently lose its ADC if either of those changes. */
#undef HAL_USE_ADC
#define HAL_USE_ADC TRUE

#include_next <halconf.h>
