/* Copyright 2026 <bvbhu>
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Minimal polling UART1 debug output for the N32H481.
 *   PA9 = USART1_TX, AF5; APB2 = 120 MHz after PLL switch.
 *   115200 8N1, TX-only, no interrupts, no FIFO.
 * Register-level on purpose: no ChibiOS SERIAL driver needed.
 */
#include "uart_dbg.h"

#include "hal.h"                     /* RCC, GPIOA, USART1 macros from n32h47x_48x.h */
#include "n32h47x_48x.h"             /* RCC_APB2PCLKEN_USART1EN etc. */

#ifndef DBG_UART_BAUD
#define DBG_UART_BAUD 115200U
#endif

static void dbg_tx(char c) {
    /* Wait TX data register empty (TXDE = STS bit7). */
    while ((USART1->STS & USART_STS_TXDE) == 0U) {
    }
    USART1->DAT = (uint32_t)(uint8_t)c;
}

void dbg_uart_init(void) {
    /* 1. Clocks: GPIOA (AHB1) + USART1 (APB2). */
    RCC->AHB1PCLKEN |= RCC_AHB1PCLKEN_IOPAEN;
    RCC->APB2PCLKEN |= RCC_APB2PCLKEN_USART1EN;

    /* 2. PA9 in AF mode (PMODE[19:18] = 2'b10), AF5, push-pull, no pull.
     *    N32 AFSEL layout (SDK GPIO_InitPeripheral): each AFSEL[n] covers 4
     *    pins, 8-bit slot each (5 bits used):
     *      RegNum = pos >> 2; BitPos = (pos & 3) * 8;
     *    PA9 -> AFSEL[9>>2]=AFSEL[2], BitPos=(9&3)*8=8 -> AF5<<8 = 0x500. */
    GPIOA->PMODE &= ~(0x3U << 18U);
    GPIOA->PMODE |= (0x2U << 18U);
    GPIOA->PUPD &= ~(0x3U << 18U);
    GPIOA->POTYPE &= ~(0x1U << 9U);
    GPIOA->AFSEL[2] &= ~(0x1FU << 8U);  /* PA9 = AFSEL[2] bits[12:8] */
    GPIOA->AFSEL[2] |= (0x5U << 8U);    /* AF5 = USART1_TX */

    /* 3. USART1: 8N1.
     * BRCF is 12.4 fixed-point USARTDIV*16 = fCK/(16*baud)*16 = fCK/baud.
     * 120e6 / 115200 = 1041.67 -> 1042 (0x412). */
    USART1->CTRL1 &= ~USART_CTRL1_UEN;   /* disable while configuring */
    USART1->BRCF = ((120000000U / DBG_UART_BAUD) + 1U) & 0xFFFFU;
    USART1->CTRL1 = USART_CTRL1_UEN | USART_CTRL1_TXEN;  /* UE + TE, no RX */
    USART1->CTRL3 = 0U;
}

void dbg_putc(char c) {
    if (c == '\n') {
        dbg_tx('\r');
    }
    dbg_tx(c);
}

void dbg_puts(const char *s) {
    while (*s != '\0') {
        dbg_putc(*s++);
    }
}

static void dbg_nibble(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    dbg_putc(hex[v & 0xFU]);
}

void dbg_hex8(uint32_t v) {
    dbg_nibble(v >> 4U);
    dbg_nibble(v);
}

void dbg_hex32(uint32_t v) {
    dbg_nibble(v >> 28U); dbg_nibble(v >> 24U);
    dbg_nibble(v >> 20U); dbg_nibble(v >> 16U);
    dbg_nibble(v >> 12U); dbg_nibble(v >> 8U);
    dbg_nibble(v >> 4U);  dbg_nibble(v);
}

void dbg_reg32(const char *label, uint32_t addr) {
    dbg_puts(label);
    dbg_putc('=');
    dbg_hex32(*(volatile uint32_t *)addr);
    dbg_puts("\r\n");
}