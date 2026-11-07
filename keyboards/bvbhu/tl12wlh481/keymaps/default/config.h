/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tl12wlh481 default keymap -- Vial settings.
 */
#pragma once

/* Vial keyboard UID (8 bytes).  Must differ from tl12blef103
 * {0x54,0x4C,0x31,0x32,0x42,0x4C,0x45,0x01} even though the matrix board
 * is identical: sharing a UID makes Vial treat both boards as one device and
 * cross-write their dynamic keymaps. */
#define VIAL_KEYBOARD_UID {0x54, 0x4C, 0x31, 0x32, 0x57, 0x4C, 0x48, 0x81}

#define DYNAMIC_KEYMAP_LAYER_COUNT 4

/* TAP_DANCE / COMBO / KEY_OVERRIDE / REPEAT_KEY are disabled keyboard-wide in
 * rules.mk, so no VIAL_*_ENTRIES definitions are needed here. */

/* ---------------------------------------------------------------------------
 * Bridge UART pin mapping.  The keyboard-level config.h already sets
 * UART_DRIVER=SD3, UART_TX_PIN=D8, UART_RX_PIN=D9 with:
 *     TL12WL_UART_TX_AF = 17   (USART3_TX, valid on ANY port pin)
 *     TL12WL_UART_RX_AF = 18   (USART3_RX, valid on ANY port pin)
 * because N32H48x maps USART3 TX/RX to arbitrary IO (reference manual
 * Table 5-29).  No AFIO remap register is needed.
 *
 * To move the link to a different pair of pins, just override the two pins
 * in the keyboard config.h -- the AF numbers stay 17/18 on every port:
 *
 *   #define UART_TX_PIN D5
 *   #define UART_RX_PIN D6
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * RGB Matrix effects (VialRGB + QMK-only).  12 LEDs is cheap enough to keep
 * the full set; this mirrors bvbhu/n32h481_demo.
 * ------------------------------------------------------------------------- */
#define ENABLE_RGB_MATRIX_ALPHAS_MODS
#define ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#define ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_BREATHING
#define ENABLE_RGB_MATRIX_BAND_SAT
#define ENABLE_RGB_MATRIX_BAND_VAL
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL

#define ENABLE_RGB_MATRIX_CYCLE_ALL
#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#define ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
#define ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#define ENABLE_RGB_MATRIX_CYCLE_SPIRAL
#define ENABLE_RGB_MATRIX_DUAL_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS

#define ENABLE_RGB_MATRIX_RAINDROPS
#define ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
#define ENABLE_RGB_MATRIX_HUE_BREATHING
#define ENABLE_RGB_MATRIX_HUE_PENDULUM
#define ENABLE_RGB_MATRIX_HUE_WAVE

#define ENABLE_RGB_MATRIX_TYPING_HEATMAP
#define ENABLE_RGB_MATRIX_DIGITAL_RAIN
#define ENABLE_RGB_MATRIX_PIXEL_RAIN
#define ENABLE_RGB_MATRIX_PIXEL_FRACTAL

#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#define ENABLE_RGB_MATRIX_SPLASH
#define ENABLE_RGB_MATRIX_MULTISPLASH
#define ENABLE_RGB_MATRIX_SOLID_SPLASH
#define ENABLE_RGB_MATRIX_SOLID_MULTISPLASH

#define ENABLE_RGB_MATRIX_FLOWER_BLOOMING
#define ENABLE_RGB_MATRIX_STARLIGHT
#define ENABLE_RGB_MATRIX_STARLIGHT_SMOOTH
#define ENABLE_RGB_MATRIX_STARLIGHT_DUAL_SAT
#define ENABLE_RGB_MATRIX_STARLIGHT_DUAL_HUE
#define ENABLE_RGB_MATRIX_RIVERFLOW
#define ENABLE_RGB_MATRIX_PIXEL_FLOW

#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_ALL
#define RGB_MATRIX_DEFAULT_VAL 160
