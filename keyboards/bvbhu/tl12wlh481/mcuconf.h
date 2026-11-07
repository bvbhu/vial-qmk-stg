/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include_next <mcuconf.h>

/* BHQ wireless bridge: USART3 = SD3 on PD8 (TX) / PD9 (RX).
 *
 * USART3 is used -- not USART2 -- because the bridge is wired to PD8/PD9.
 * N32H48x gives USART3/UART5/UART8 a full pin-mapping feature: USART3_TX can
 * be routed to ANY port pin with AF17 and USART3_RX to ANY port pin with
 * AF18 (reference manual Table 5-29, "Some signals of USART3 support full pin
 * mapping").  So PD8/PD9 need nothing but the right AFSEL value; no AFIO
 * remap/chip-select register is involved.
 *
 * USART2 is disabled so only one SERIAL instance claims an IRQ vector. */
#undef N32_SERIAL_USE_USART3
#define N32_SERIAL_USE_USART3 TRUE

#undef N32_SERIAL_USE_USART2
#define N32_SERIAL_USE_USART2 FALSE

/* USART1 stays free of the HAL SERIAL driver: n32h481_demo uses it as a
 * bare polling console, and sharing it would collide with the ISR vectors. */
#undef N32_SERIAL_USE_USART1
#define N32_SERIAL_USE_USART1 FALSE
