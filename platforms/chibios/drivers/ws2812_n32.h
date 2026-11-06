/*
    Copyright (C) 2026 bvbhu (N32H48x port for vial-qmk-stg-n32h481)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 or (at your option)
    any later version.
*/

#pragma once

#include <stdint.h>
#include "gpio.h"

/* Data line for the WS2812 strip. */
#ifndef WS2812_DI_PIN
#    define WS2812_DI_PIN C3
#endif

/* Number of LEDs chained on the strip; kept in sync with keyboard.json
   ("rgb_matrix": { "led_count": N }). */
#ifndef WS2812_LED_COUNT
#    define WS2812_LED_COUNT 3
#endif

/* Bit-banged WS2812 driver: the N32 port in this tree provides no PWM or GPT
   LLD, so the 800 kHz waveform is generated with the DWT cycle counter.
   ws2812_flush() masks interrupts for the ~90 us data phase only; the reset
   latch is waited with interrupts enabled so USB/Vial stays responsive. */
void ws2812_init(void);
void ws2812_set_color(int index, uint8_t red, uint8_t green, uint8_t blue);
void ws2812_set_color_all(uint8_t red, uint8_t green, uint8_t blue);
void ws2812_flush(void);

#define ws2812_pin_high() gpio_write_pin_high(WS2812_DI_PIN)
#define ws2812_pin_low()  gpio_write_pin_low(WS2812_DI_PIN)
