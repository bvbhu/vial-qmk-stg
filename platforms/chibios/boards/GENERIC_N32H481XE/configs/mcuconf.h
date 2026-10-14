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
 * N32H49x drivers configuration.
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

#define N32H49x_MCUCONF

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

#endif /* MCUCONF_H */
