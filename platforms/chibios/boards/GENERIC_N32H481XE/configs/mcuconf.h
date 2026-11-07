/*
    ChibiOS - Copyright (C) 2006..2020 Giovanni Di Sirio
    ChibiOS - Copyright (C) 2026 bvbhu (vial-qmk-stg-n32h481 fork)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef MCUCONF_H
#define MCUCONF_H

/*
 * N32H48x drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 *
 * IRQ priorities:
 * 15...0       Lowest...Highest.
 *
 * DMA priorities:
 * 0...3        Lowest...Highest.
 */

#define N32H48x_MCUCONF

/*
 * General settings.
 */
#define N32_NO_INIT                        FALSE

/*
 * HAL driver system settings.
 */
#define N32_HICK_ENABLED                   TRUE
#define N32_LICK_ENABLED                   FALSE
#define N32_HEXT_ENABLED                   FALSE
#define N32_SCLKSEL                        N32_SCLKSEL_PLL
#define N32_PLLSRC                         N32_PLLSRC_HSI
#define N32_PLLMULFCT_VALUE                30
#define N32_PLLINPRES_VALUE                1
#define N32_PLLOUTPRES_VALUE               1
#define N32_AHBDIV                         N32_AHBDIV_DIV1
#define N32_APB1DIV                        N32_APB1DIV_DIV2
#define N32_APB2DIV                        N32_APB2DIV_DIV2

/*
 * ST driver system settings.
 */
#define N32_ST_IRQ_PRIORITY                8

/*
 * ADC driver system settings.
 */
#define N32_ADC_USE_ADC1                   TRUE
#define N32_ADC_ADC1_IRQ_PRIORITY          5

/*
 * USB driver system settings.
 */
#define N32_USB_USE_OTG1                   TRUE
#define N32_USB_OTG1_IRQ_PRIORITY          5

/*
 * SERIAL driver system settings (ChibiOS HAL_USE_SERIAL).
 * The driver only gets compiled when the keyboard halconf.h sets
 * HAL_USE_SERIAL to TRUE.  USART1 is already occupied by the
 * n32h481_demo uart_dbg.c polling console, so USART2 (SD2, PA2/PA3)
 * is the default instance here; enable any other instance as needed.
 * IRQ priority 12 follows the other N32 LLD defaults.
 */
#define N32_SERIAL_USE_USART1              FALSE
#define N32_SERIAL_USE_USART2              TRUE
#define N32_SERIAL_USE_USART3              FALSE
#define N32_SERIAL_USE_USART4              FALSE
#define N32_SERIAL_USE_UART5               FALSE
#define N32_SERIAL_USE_UART6               FALSE
#define N32_SERIAL_USE_UART7               FALSE
#define N32_SERIAL_USE_UART8               FALSE

#define N32_SERIAL_USART1_IRQ_PRIORITY     12
#define N32_SERIAL_USART2_IRQ_PRIORITY     12
#define N32_SERIAL_USART3_IRQ_PRIORITY     12
#define N32_SERIAL_USART4_IRQ_PRIORITY     12
#define N32_SERIAL_UART5_IRQ_PRIORITY      12
#define N32_SERIAL_UART6_IRQ_PRIORITY      12
#define N32_SERIAL_UART7_IRQ_PRIORITY      12
#define N32_SERIAL_UART8_IRQ_PRIORITY      12

/*
 * SPI driver system settings (ChibiOS HAL_USE_SPI / QMK spi_master).
 * The LLD is interrupt driven (TXE/RXNE), 8-bit frames, master + software
 * NSS, and needs no DMA.  Only the instances enabled here get an SPIDx
 * object and an ISR vector; HAL_USE_SPI must additionally be TRUE in the
 * keyboard halconf.h (SPI_DRIVER_REQUIRED=yes does that automatically).
 *
 * SPI1 is the QMK default on this port (PA5/PA6/PA7, AF1).  Override
 * SPI_DRIVER / SPI_*_PIN / SPI_*_PAL_MODE in config.h when those pins are
 * already used by the matrix, e.g. on bvbhu/tl12wlh481.
 */
#define N32_SPI_USE_SPI1                   TRUE
#define N32_SPI_USE_SPI2                   FALSE
#define N32_SPI_USE_SPI3                   FALSE
#define N32_SPI_USE_SPI4                   FALSE
#define N32_SPI_USE_SPI5                   FALSE
#define N32_SPI_USE_SPI6                   FALSE

#define N32_SPI_SPI1_IRQ_PRIORITY          12
#define N32_SPI_SPI2_IRQ_PRIORITY          12
#define N32_SPI_SPI3_IRQ_PRIORITY          12
#define N32_SPI_SPI4_IRQ_PRIORITY          12
#define N32_SPI_SPI5_IRQ_PRIORITY          12
#define N32_SPI_SPI6_IRQ_PRIORITY          12

#endif /* MCUCONF_H */
