/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "hal.h"
#include "quantum.h"
#include "rgb_matrix.h"
#include "ws2812_n32.h"

#if defined(BLUETOOTH_BHQ)
#    include "bhq.h"        /* 0x11 SET_CONFIG frame builder */
#    include "bhq_common.h" /* bhq_common_init / bhq_wireless_task / process_record_bhq */
#endif

#include <string.h>

void board_init(void);
void keyboard_post_init_kb(void);
void housekeeping_task_kb(void);
bool process_record_kb(uint16_t keycode, keyrecord_t *record);

#if defined(BLUETOOTH_BHQ)
void tl12wl_config_task(void); /* delayed 0x11 SET_CONFIG, defined in the .c */
#endif

/*
 * Pin quick reference -- MUST stay in sync with keyboard.json "matrix_pins"
 * and the pin block at the top of config.h.  These are N32H481RE (LQFP64)
 * pins, NOT the STM32F103 pins from tl12blef103.
 *
 *   ROW: B0, C4
 *   COL: A5, A7, B2, C5, B1, A6
 *   WS2812 DIN: E7 (12 LEDs, GRB)
 *   BHQ UART:   PD8 (TX -> bridge D8), PD9 (RX <- bridge D9), 128000 8N1
 *               USART3 + AF17/AF18 (fully pin-mappable peripheral)
 *   USB sense:  C3 (high = plugged in; optional, unused in this build)
 *
 * COL2ROW: rows are driven low one at a time, columns are read with pull-ups;
 * diode anodes face the rows.
 */
