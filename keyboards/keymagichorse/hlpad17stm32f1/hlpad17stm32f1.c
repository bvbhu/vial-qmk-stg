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
#include "quantum.h"

#if defined(BLUETOOTH_BHQ)
#   include "bhq.h"
#   include "bhq_common.h"
#endif

#if defined(KB_LPM_ENABLED)
#   include "lpm.h"
#endif

#if defined(KB_DEBUG)
#   include "km_printf.h"
#endif

#include "uart.h"
#include "hal.h"
void board_init(void) 
{
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) | AFIO_MAPR_SWJ_CFG_DISABLE;
#if defined(BLUETOOTH_BHQ)
    bhq_common_init();
#   if defined(KB_LPM_ENABLED)
    lpm_init();
#   endif
#endif
}

void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_wireless_task();
    #   if defined(KB_LPM_ENABLED)
        lpm_task();
    #   endif
#endif
}

led_config_t g_led_config = {

    // Key Matrix to LED Index
    {
        {  0,  1,  2,  3 },
        {  4,  5,  6,  7 },
        {  8,  9, 10, NO_LED },
        { 11, 12, 13, NO_LED },
        { 14, NO_LED, 15, 16 }
    },

    // LED Physical Position (x, y)
    {
        {  0,  0 },  // 0  Num Lock
        { 16,  0 },  // 1  /
        { 32,  0 },  // 2  *
        { 48,  0 },  // 3  -

        { 48, 16 },  // 4  +
        { 32, 16 },  // 5  9
        { 16, 16 },  // 6  8
        {  0, 16 },  // 7  7

        {  0, 32 },  // 8  4
        { 16, 32 },  // 9  5
        { 32, 32 },  // 10 6

        { 48, 48 },  // 11 Enter
        { 32, 48 },  // 12 3
        { 16, 48 },  // 13 2

        {  0, 48 },  // 14 1
        {  0, 64 },  // 15 0
        { 32, 64 },  // 16 .
    },

    // LED Flags
    {
        1,1,1,1,
        1,1,1,1,
        1,1,1,
        1,1,1,
        1,1,1
    }
};
