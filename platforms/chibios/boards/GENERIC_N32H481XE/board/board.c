/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio
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

#include "hal.h"

/* N32 AFIO SWJ-disable value = AFIO_RMP_CFG_SWJ_CFG_2 | 0x80000000
   (the 0x80000000 mark bit is defined as GPIO_RMP_SWJ_MARK in the SDK
   std-periph gpio driver, not in the CMSIS device header). */
#define N32_GPIO_RMP_SWJ_DISABLE (AFIO_RMP_CFG_SWJ_CFG_2 | 0x80000000U)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  {VAL_GPIOAPOD, VAL_GPIOAPMODE, VAL_GPIOAPUPD, VAL_GPIOAPOTYPE, VAL_GPIOADS},
  {VAL_GPIOBPOD, VAL_GPIOBPMODE, VAL_GPIOBPUPD, VAL_GPIOBPOTYPE, VAL_GPIOBDS},
#if N32_HAS_GPIOC
  {VAL_GPIOCODP, VAL_GPIOCOPMODE, VAL_GPIOCOPUPD, VAL_GPIOCOPOTYPE, VAL_GPIOCODS},
#endif
  {VAL_GPIODPOD, VAL_GPIODPMODE, VAL_GPIODPUPD, VAL_GPIODPOTYPE, VAL_GPIODDS},
#if N32_HAS_GPIOE
  {VAL_GPIOEPOD, VAL_GPIOEPMODE, VAL_GPIOEPUPD, VAL_GPIOEPOTYPE, VAL_GPIOEDS},
#endif
#if N32_HAS_GPIOF
  {VAL_GPIOFPOD, VAL_GPIOFPMODE, VAL_GPIOFPUPD, VAL_GPIOFPOTYPE, VAL_GPIOFDS},
#endif
};
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Early initialization code.
 * @details System clocks are initialized before everything else.
 */
void __early_init(void) {
  n32_clock_init();
}

/**
 * @brief   Board-specific initialization code.
 * @note    You can add your board-specific code here.
 */
void boardInit(void) {
  /* N32H481 enables SWD/JTAG debug functions after reset, which binds
     PA13 (SWDIO), PA15, PB3 and PB4 to the debug interface. The EC matrix
     and the mux selector lines use these pins as GPIO, so release them:
     set AFIO_RMP_CFG.SWJ_CFG = GPIO_RMP_SWJ_DISABLE
     (AFIO_RMP_CFG_SWJ_CFG_2 | GPIO_RMP_SWJ_MARK).
     NOTE: this also disables SWD debugging after boot. To re-enable SWD,
     hold the chip in reset with PA13 low / use the ROM bootloader to reflash,
     or clear the SWJ_CFG bits via the system memory bootloader. */
  RCC->APB2PCLKEN |= RCC_APB2PCLKEN_AFIOEN;
  AFIO->RMP_CFG |= N32_GPIO_RMP_SWJ_DISABLE;
}
