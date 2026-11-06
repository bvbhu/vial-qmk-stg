/* Copyright 2026 <bvbhu>
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Minimal polling UART1 debug output for the N32H481 (PA9 = USART1_TX, AF5).
 * Init assumes SYSCLK already switched to PLL (APB2 = 120 MHz).
 * No interrupt, no RX, no ChibiOS driver — pure register polling.
 */
#pragma once

#include <stdint.h>

void dbg_uart_init(void);
void dbg_putc(char c);
void dbg_puts(const char *s);
void dbg_hex8(uint32_t v);
void dbg_hex32(uint32_t v);
void dbg_reg32(const char *label, uint32_t addr);