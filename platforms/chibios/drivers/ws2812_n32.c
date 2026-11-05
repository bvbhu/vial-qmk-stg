/*
    Copyright (C) 2026 bvbhu (N32H48x port for vial-qmk-stg-n32h481)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 or (at your option)
    any later version.

    WS2812 driver for the N32H481 (bit-banged, DWT-timed).
*/

/**
 * @file    ws2812_n32.c
 * @brief   WS2812 / WS2812B driver for N32H481 using DWT-timed bit-banging.
 *
 * Why bit-banging: the N32 port in this tree only provides ADC/GPIO/OTG/
 * SYSTICK/USART LLDs - there is no PWM or GPT driver.
 *
 * Why DWT instead of a NOP loop: a `while (n--) __NOP();` loop costs 4-5
 * cycles per iteration (decrement, compare, branch), not one.  The first
 * version of this driver assumed one cycle and therefore produced bit
 * periods 4-5x too long; every bit was sampled as a logical 1 and all three
 * LEDs lit pure white.  The Cortex-M4 debug watchpoint unit provides a
 * free-running cycle counter (DWT->CYCCNT) which gives exact timing that
 * does not depend on instruction costs, pipeline behaviour or compiler
 * version.
 *
 * Interrupt discipline (this is what fixes the random flicker):
 * WS2812 decides 0 vs 1 from the width of the HIGH phase (~150 ns decision
 * window) and treats any LOW gap longer than 50 us as an end-of-frame
 * latch.  One frame here is 3 LEDs * 24 bits * 1.25 us = 90 us.  With
 * interrupts enabled, a USB/SysTick ISR landing anywhere in that window
 * stretches a HIGH (turning a 0 into a 1) or a LOW (latching a partial
 * frame) -- the strip then shows garbled colours until the next frame.
 * Static test colours masked the problem (the next frame repainted the same
 * values), but animated effects repaint every 16 ms and expose it.
 *
 * QMK's stock platforms/chibios/drivers/ws2812_bitbang.c wraps the whole
 * frame in chSysLock()/chSysUnlock() for exactly this reason.  Here we mask
 * interrupts with PRIMASK for the DATA phase only (~90 us, well below the
 * 1 ms USB SOF period) and keep the 80 us reset latch outside the critical
 * section, so the USB/Vial stack is never starved for a whole frame.  The
 * earlier "leave interrupts on" choice was made while the NOP-timed version
 * still had millisecond-long frames; it no longer applies.
 */

#include "quantum.h"
#include "gpio.h"
#include "ws2812_n32.h"

/* Core clock used to convert nanoseconds into DWT cycles.
   Taken from this board's mcuconf.h: HSI (8 MHz) x PLLMULFCT(30) with
   PLLOUTPRES=1 -> 240 MHz.  Getting this wrong scales every bit period and
   turns the whole strip white. */
#ifndef WS2812_N32_SYSCLK
#    define WS2812_N32_SYSCLK 240000000U
#endif

#define NS_TO_CYCLES(ns) ((uint32_t)(((uint64_t)(ns) * (uint64_t)WS2812_N32_SYSCLK) / 1000000000ULL))

#define WS2812_T1H_CYC NS_TO_CYCLES(800)
#define WS2812_T1L_CYC NS_TO_CYCLES(450)
#define WS2812_T0H_CYC NS_TO_CYCLES(400)
#define WS2812_T0L_CYC NS_TO_CYCLES(850)
#define WS2812_RESET_CYC NS_TO_CYCLES(80000)

/* 72 bits * 1.25 us = 90 us of pure bit time; allow 150 us for GPIO calls,
   loop overhead and flash fetch stalls before flagging an overrun. */
#define WS2812_DATA_CYC_BUDGET NS_TO_CYCLES(150000U)

/* GRB triplet per LED. */
static uint8_t ws2812_leds[WS2812_LED_COUNT][3];

/* Runtime diagnostics, readable over SWD via the ELF symbol addresses.
   These make the fix verifiable without looking at the LEDs: a healthy
   firmware has a growing flush_cnt, a masked_max_cyc of roughly 22k-36k
   cycles (~90-150 us) and an overrun_cnt that stays at zero. */
volatile uint32_t ws2812_dbg_flush_cnt;
volatile uint32_t ws2812_dbg_masked_cyc;
volatile uint32_t ws2812_dbg_masked_max_cyc;
volatile uint32_t ws2812_dbg_overrun_cnt;
volatile uint32_t ws2812_dbg_dwt_reinit_cnt;
volatile uint32_t ws2812_dbg_stall_cnt;
volatile uint32_t ws2812_dbg_frame_cyc;

/* Every deadline is derived from DWT->CYCCNT, so a stopped cycle counter
   turns dwt_delay_until() into a guard-loop timeout and emits a completely
   garbled frame.  A J-Link session (halt / connect / disconnect) is known to
   freeze or disable CYCCNTEN on this part: measured dwt_reinit_cnt == 2 with
   otherwise healthy frames.  Check that the counter is actually moving at
   the start of each flush, re-initialise it if not, and skip the frame
   entirely if it still will not run -- a dark frame is far less visible than
   a frame of random colours.  CYCCNT is only reset here, when no deadline
   is live. */
static inline int __attribute__((always_inline)) dwt_check(void) {
    uint32_t a = DWT->CYCCNT;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    if (DWT->CYCCNT != a) {
        return 1;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL &= (uint32_t)~DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    ws2812_dbg_dwt_reinit_cnt++;

    a = DWT->CYCCNT;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    return DWT->CYCCNT != a;
}

static inline void __attribute__((always_inline)) dwt_delay_until(uint32_t deadline) {
    /* Bounded wait so a mis-tuned value can never hang the firmware. */
    uint32_t guard = 100000U;
    while ((int32_t)(DWT->CYCCNT - deadline) < 0 && --guard) {
    }
}

/* PRIMASK save/restore rather than chSysLock(): a BASEPRI-based kernel lock
   can still let a high-priority USB ISR through, which is precisely what
   corrupts a WS2812 frame.  The memory clobber keeps the GPIO writes inside
   the critical section. */
static inline uint32_t __attribute__((always_inline)) irq_save(void) {
    uint32_t primask;
    __asm volatile("mrs %0, primask" : "=r"(primask)::"memory");
    __asm volatile("cpsid i" ::: "memory");
    return primask;
}

static inline void __attribute__((always_inline)) irq_restore(uint32_t primask) {
    __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
}

static inline void __attribute__((always_inline)) send_bit(uint32_t hi_cyc, uint32_t lo_cyc) {
    /* Start each deadline AFTER the GPIO write, otherwise the (variable)
       palSetLine/palClearLine call cost eats into the high phase and makes
       the 0/1 decision window compiler- and cache-dependent. */
    ws2812_pin_high();
    const uint32_t start = DWT->CYCCNT;
    dwt_delay_until(start + hi_cyc);
    ws2812_pin_low();
    const uint32_t mid = DWT->CYCCNT;
    dwt_delay_until(mid + lo_cyc);
}

void ws2812_init(void) {
    (void)dwt_check();
    gpio_set_pin_output(WS2812_DI_PIN);
    gpio_write_pin_low(WS2812_DI_PIN);

    for (uint32_t i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_leds[i][0] = 0;
        ws2812_leds[i][1] = 0;
        ws2812_leds[i][2] = 0;
    }
    ws2812_flush();
}

void ws2812_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index < 0 || index >= WS2812_LED_COUNT) {
        return;
    }
    ws2812_leds[index][0] = green;
    ws2812_leds[index][1] = red;
    ws2812_leds[index][2] = blue;
}

void ws2812_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_color(i, red, green, blue);
    }
}

void ws2812_flush(void) {
    if (!dwt_check()) {
        ws2812_dbg_stall_cnt++;
        gpio_write_pin_low(WS2812_DI_PIN);
        return;
    }
    const uint32_t frame_start = DWT->CYCCNT;

    /* ---- timing-critical data phase: mask ALL maskable interrupts ---- */
    const uint32_t primask    = irq_save();
    const uint32_t data_start = DWT->CYCCNT;

    for (uint32_t i = 0; i < WS2812_LED_COUNT; i++) {
        uint8_t grb[3] = {ws2812_leds[i][0], ws2812_leds[i][1], ws2812_leds[i][2]};
        for (uint32_t byte = 0; byte < 3; byte++) {
            uint8_t v = grb[byte];
            for (uint32_t bit = 0; bit < 8; bit++) {
                if (v & 0x80) {
                    send_bit(WS2812_T1H_CYC, WS2812_T1L_CYC);
                } else {
                    send_bit(WS2812_T0H_CYC, WS2812_T0L_CYC);
                }
                v <<= 1;
            }
        }
    }

    gpio_write_pin_low(WS2812_DI_PIN);
    const uint32_t data_end = DWT->CYCCNT;

    irq_restore(primask);
    /* ---- end of critical section; USB interrupts can run again ---- */

    const uint32_t masked = data_end - data_start;
    ws2812_dbg_masked_cyc = masked;
    if (masked > ws2812_dbg_masked_max_cyc) {
        ws2812_dbg_masked_max_cyc = masked;
    }
    if (masked > WS2812_DATA_CYC_BUDGET) {
        ws2812_dbg_overrun_cnt++;
    }
    ws2812_dbg_flush_cnt++;

    /* Reset/latch gap: the line simply stays low here, there are no edges to
       protect, so interrupts stay enabled for USB responsiveness.  If a
       pending ISR runs past the deadline the line has already been low long
       enough for the strip to latch, which is harmless. */
    dwt_delay_until(data_end + WS2812_RESET_CYC);
    ws2812_dbg_frame_cyc = DWT->CYCCNT - frame_start;
}
