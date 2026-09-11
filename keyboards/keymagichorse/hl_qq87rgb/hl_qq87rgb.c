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
#include "battery.h"



void board_init(void) 
{
#if defined(BLUETOOTH_BHQ)
    bhq_common_init();
#   if defined(KB_LPM_ENABLED)
    lpm_init();
#   endif
#endif

#   if defined(KB_DEBUG)
    km_printf_init();
    km_printf("hello rtt log1111111\r\n");
#   endif
}
void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_wireless_task();
    #   if defined(KB_LPM_ENABLED)
        lpm_task();
    #   endif
#endif
}




#ifdef RGB_MATRIX_ENABLE 

led_config_t g_led_config = {
	{
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
		{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
		{34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50},
		{51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED},
		{63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, NO_LED, NO_LED, NO_LED, 75, NO_LED},
		{76, 77, 78, NO_LED, NO_LED, NO_LED, 79, NO_LED, NO_LED, 80, 81, 82, 83, NO_LED, 84, 85, 86},
	}, {
		{6, 7}, {21, 7}, {34, 7}, {46, 7}, {58, 7}, {74, 7}, {86, 7}, {98, 7}, {110, 7}, {126, 7}, {138, 7}, {150, 7}, {163, 7}, {178, 7}, {193, 7}, {206, 7}, {218, 7}, 
		{6, 22}, {18, 22}, {31, 22}, {43, 22}, {55, 22}, {68, 22}, {80, 22}, {92, 22}, {104, 22}, {117, 22}, {129, 22}, {141, 22}, {153, 22}, {172, 22}, {193, 22}, {206, 22}, {218, 22}, 
		{9, 32}, {25, 32}, {37, 32}, {49, 32}, {61, 32}, {74, 32}, {86, 32}, {98, 32}, {110, 32}, {123, 32}, {135, 32}, {147, 32}, {160, 32}, {175, 40}, {193, 32}, {206, 32}, {218, 32}, 
		{11, 42}, {28, 42}, {40, 42}, {52, 42}, {64, 42}, {77, 42}, {89, 42}, {101, 42}, {114, 42}, {126, 42}, {138, 42}, {150, 42}, 
		{14, 52}, {34, 52}, {46, 52}, {58, 52}, {71, 52}, {83, 52}, {95, 52}, {107, 52}, {120, 52}, {132, 52}, {144, 52}, {167, 52}, {206, 52}, 
		{8, 62}, {23, 62}, {38, 62}, {84, 62}, {130, 62}, {146, 62}, {161, 62}, {176, 62}, {193, 62}, {206, 62}, {218, 62}, 
	}, {
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
	}
};

#endif

