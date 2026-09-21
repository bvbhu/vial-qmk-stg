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

void calibrate_matrix(void); /* kbd67ble_ec_matrix.c：开机采样初始校准读数 */

void board_init(void) {
#if defined(BLUETOOTH_BHQ)
#   if defined(KB_LPM_ENABLED)
    lpm_init();
#   endif
#endif
}

void keyboard_post_init_kb(void) {
    /* analog_init 在 keyboard_post_init_quantum 链尾之前已完成，
     * 此时初始校准读数采样写入 top_reading 才会被核心层正确接受 */
    calibrate_matrix();
    keyboard_post_init_user(); /* 覆盖了弱默认实现，必须手动回调 keymap 层 */
}

void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_wireless_task();
#   if defined(KB_LPM_ENABLED)
    lpm_task();
#   endif
#endif
}
