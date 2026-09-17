/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "hal.h"
#include "quantum.h"

#if defined(BLUETOOTH_BHQ)
#    include "bhq.h"        /* 0x11 配置指令组帧 */
#    include "bhq_common.h" /* bhq_common_init / bhq_wireless_task / process_record_bhq */
#endif

#if defined(KB_LPM_ENABLED)
#    include "lpm.h" /* lpm_init / lpm_task */
#endif

#include <string.h>

void board_init(void);
void keyboard_post_init_kb(void);
void housekeeping_task_kb(void);
bool process_record_kb(uint16_t keycode, keyrecord_t *record);

/* ============================================================================
 *  引脚速查(与 keyboard.json 保持一致, 便于接线时对照)
 *
 *  矩阵 (COL2ROW) —— 与 keyboard.json 一致:
 *    ROW: A9, B15
 *    COL: B12, B14, B5, A8, A10, B13
 *  WS2812 数据线: B6
 *  BHQ 串口: TX=A2(→桥 PB4), RX=A3(←桥 PB7)
 *
 *  COL2ROW 含义: 电流从 ROW 流向 COL。ROW 是**驱动**端(逐行拉低扫描),
 *  COL 是**读入**端(带上拉)。二极管方向: 阳极朝 ROW、阴极朝 COL。
 * ========================================================================= */
