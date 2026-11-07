/* Copyright 2026 Talo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 */

/*
 * tl12wlh481 -- TL12 wireless matrix board on the N32H481RE (LQFP64).
 *
 * Same physical matrix board as bvbhu/tl12blef103: 2 rows x 6 columns,
 * COL2ROW, 12 WS2812 LEDs, the same LAYOUT / rgb_matrix layout / Vial layout.
 * Only the MCU and the pin assignment differ.
 *
 * ============================================================================
 *  PIN ASSIGNMENT IS FIXED (user-defined wiring, 2026-11-07)
 *
 *  Every matrix/RGB pin below is now a real assignment, not a proposal.
 *  All of them were checked against the LQFP64 bond-out table in
 *  bvbhu/n32h481_iotest/readme.md (E7 is in the E7..E14 bonded group, C4 in
 *  the C0..C4 group, GPIOE clock is enabled by the PAL LLD on demand).
 *  If the wiring changes, update BOTH this file and keyboard.json
 *  "matrix_pins" -- they must agree.
 *
 *  The bridge UART is NOT on USART2/PA2-PA3: the board wires the bridge to
 *  PD8/PD9, so it uses USART3 (SD3) with AF17/AF18 -- USART3 supports full
 *  pin mapping on N32H48x, see section 2.
 * ============================================================================
 */
#pragma once

/* ---------------------------------------------------------------------------
 * 1. Pin assignment -- keep in sync with keyboard.json "matrix_pins"
 *
 *    ROW0 B0 | ROW1 C4
 *    COL0 A5 | COL1 A7 | COL2 B2 | COL3 C5 | COL4 B1 | COL5 A6
 *    WS2812 DIN E7
 *
 *    Reserved on this package, never use for the matrix:
 *      B12/B13  USB HS VBUS / OTG ID
 *      B14/B15  USB HS D- / D+   (the tl12blef103 board used PB14/PB15 here
 *                                 -- those are the USB pins on N32H481!)
 *      A13/A14  SWDIO / SWCLK
 *      B7       BOOT0
 *      H0/H1    HSE OSC_IN / OSC_OUT (unused: the PLL runs from HSI)
 *      Unbonded: A10 A11 A12, B6 B10 B11, C6..C15, D0..D7 D15,
 *                E0..E6 E15, F0 F1 F2 F6 F11..F15, all GPIOG
 *
 *    These defines are documentation only: the pin numbers actually compiled
 *    in come from keyboard.json.  They are kept in step so the C side and the
 *    JSON side cannot drift apart unnoticed.
 * ------------------------------------------------------------------------- */
#define TL12WL_ROW0_PIN B0
#define TL12WL_ROW1_PIN C4
#define TL12WL_COL0_PIN A5
#define TL12WL_COL1_PIN A7
#define TL12WL_COL2_PIN B2
#define TL12WL_COL3_PIN C5
#define TL12WL_COL4_PIN B1
#define TL12WL_COL5_PIN A6

/* WS2812 data line (GPIOE, clock gated by palSetLineMode on demand).
 * ws2812_n32.h defaults to C3; this overrides it.
 *
 * The chain is the TL12 matrix board's 12 LEDs, powered from 3.3V, so the
 * 3.3V data level already clears the 0.7 x VDD threshold -- no level shifter.
 * The dev board's own 3 LEDs (RGB0..RGB2, fed from 5V and ending at RGBOUT)
 * are NOT part of this chain: wire matrix DIN straight to E7, do not tap
 * RGBOUT, otherwise the chain gains 3 leading LEDs and the index shifts. */
#define WS2812_DI_PIN    E7
#define WS2812_LED_COUNT 12

/* ---------------------------------------------------------------------------
 * 2. BHQ wireless bridge UART (USART3 / SD3, 128000 8N1)
 *
 *    Wired to PD8/PD9 because that is where the bridge's D8/D9 pads land:
 *      PD8 (N32 TX, AF17) -> 桥 D8 (= A8 / PA8, UART1 RXD1)
 *      PD9 (N32 RX, AF18) <- 桥 D9 (= A9 / PA9, UART1 TXD1)
 *
 *    WHY USART3 AND NOT USART2:
 *      PD8/PD9 have no fixed USART function in the SDK AF list (only
 *      ATIM1_CH3/CH3N, GTIM9/10, SPI3, I2S3, FEMC and ETH).  They can still
 *      carry a UART because N32H48x gives USART3/UART5/UART8 a *full pin
 *      mapping* feature (reference manual Table 5-29; datasheet feature list:
 *      "TX/RX of USART3/UART5/UART8 can be mapped to all pins"):
 *          USART3_TX -> ANY port pin, AFSEL = AF17
 *          USART3_RX -> ANY port pin, AFSEL = AF18
 *      No AFIO remap register is involved -- the AF number alone routes it.
 *      So any bonded GPIO can be the bridge UART; that also means the old
 *      "only PA2/PA3 is usable" restriction never applied to USART3.
 *
 *    Verified against the official SDK:
 *      Nations.N32H47x_48x_Library.1.2.0/firmware/CMSIS/device/n32h47x_48x.h
 *          USART3_BASE = APB1PERIPH_BASE + 0x1000
 *          RCC_APB1PCLKEN_USART3EN / RCC_APB1PRST_USART3RST = 0x00000008
 *          USART3_IRQn = 39  ->  VectorDC (0x40 + 39*4 = 0xDC)
 *      which matches N32_USART3_HANDLER = VectorDC in n32_isr.h and the
 *      SD3 block in hal_serial_lld.c.
 *
 *    TX gets the highest output drive (128000 bps), RX an internal pull-up
 *    so an idle/unconnected line cannot trigger spurious start bits.
 *    Both AF macros stay overridable from the keymap config.h.
 * ------------------------------------------------------------------------- */
#ifndef TL12WL_UART_TX_AF
#    define TL12WL_UART_TX_AF 17 /* USART3_TX, any pin */
#endif
#ifndef TL12WL_UART_RX_AF
#    define TL12WL_UART_RX_AF 18 /* USART3_RX, any pin */
#endif

#define UART_DRIVER       SD3
#define UART_TX_PIN       D8
#define UART_RX_PIN       D9
#define UART_TX_PAL_MODE  (PAL_MODE_ALTERNATE(TL12WL_UART_TX_AF) | PAL_N32_OTYPE_PUSHPULL | PAL_N32_OSPEED_HIGHEST)
#define UART_RX_PAL_MODE  (PAL_MODE_ALTERNATE(TL12WL_UART_RX_AF) | PAL_N32_PUPDR_PULLUP)

/* USB power sense: high = USB plugged in.  bhq_common_init() pulls the pin
 * low internally so an unconnected pin does not read as "on USB".
 *
 * NOTE: this used to be A6 (same as tl12blef103), but A6 is COL5 on this
 * wiring.  bhq_common_init() unconditionally calls
 * gpio_set_pin_input_low(USB_POWER_SENSE_PIN), so leaving A6 here would have
 * reconfigured the column line as an input and killed COL5.  Moved to C3,
 * which the WS2812 data line no longer uses.
 *
 * The macro has to stay defined: bhq_common.c uses it unguarded in
 * bhq_common_init().  Nothing in this build reads the value back (LPM and
 * battery are off, so usb_power_connected() is garbage-collected), which is
 * why an unwired C3 is harmless -- it just reads 0. */
#define USB_POWER_SENSE_PIN       C3
#define USB_POWER_CONNECTED_LEVEL 1

/* ---------------------------------------------------------------------------
 * 3. N32H481 platform specifics
 * ------------------------------------------------------------------------- */
/* Dynamic keymap / Vial config lives in the on-chip Flash dual-page mirror
 * (drivers/eeprom/eeprom_custom.c, 2 x 8 KB pages at 0x0807C000). */
#define EEPROM_SIZE 2048

/* N32 USBHS implements OUT endpoints 0 and 9..15 only.  The stock interrupt
 * OUT RAW endpoint is dead on this silicon; use the EP0 SET_REPORT transport
 * instead (same as bvbhu/n32h481_demo).  BHQ's 0x97/0x27 RAW forwarding
 * depends on this working. */
#define RAW_HID_SET_REPORT_TRANSPORT 1

#include_next <config.h>
