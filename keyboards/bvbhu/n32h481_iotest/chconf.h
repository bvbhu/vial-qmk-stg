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

// The N32H48x SYSTICK driver supports only OSAL_ST_MODE_PERIODIC.
// Forcing CH_CFG_ST_TIMEDELTA to 0 selects PERIODIC mode in the OSAL
// (see lib/chibios/os/hal/osal/rt-nil/osal.h). QMK does not use the
// free-running alarm feature, so PERIODIC is safe here.
#define CH_CFG_ST_TIMEDELTA 0

#include_next <chconf.h>