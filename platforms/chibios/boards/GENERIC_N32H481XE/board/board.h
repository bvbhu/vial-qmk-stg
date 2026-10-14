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

#ifndef _BOARD_H_
#define _BOARD_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Setup for a Generic N32H481RE board.
 */

/*
 * Board identifier.
 */
#define BOARD_GENERIC_N32H481XE
#define BOARD_NAME                  "GENERIC N32H481RE board"

/*
 * Board oscillators-related settings.
 */
#if !defined(N32_LEXTCLK)
#define N32_LEXTCLK                 32768
#endif

#if !defined(N32_HEXTCLK)
#define N32_HEXTCLK                 16000000
#endif

/*
 * MCU type, supported types are defined in ./os/hal/platforms/hal_lld.h.
 */
#define N32H481RE

/*
 * GPIO settings, allow unused GPIO for smaller chip packages.
 * The N32H481REL7K (LQFP64) exposes ports A..F; all register ports
 * exist on the silicon and are enabled unconditionally here.
 */
#define N32_HAS_GPIOA              TRUE
#define N32_HAS_GPIOB              TRUE
#define N32_HAS_GPIOC              TRUE
#define N32_HAS_GPIOD              TRUE
#define N32_HAS_GPIOE              TRUE
#define N32_HAS_GPIOF              TRUE
#define N32_HAS_GPIOG              FALSE
#define N32_HAS_GPIOH              FALSE

/*
 * IO pins assignments.
 */
#define GPIOA_PIN0                  0U
#define GPIOA_PIN1                  1U
#define GPIOA_PIN2                  2U
#define GPIOA_PIN3                  3U
#define GPIOA_PIN4                  4U
#define GPIOA_PIN5                  5U
#define GPIOA_PIN6                  6U
#define GPIOA_PIN7                  7U
#define GPIOA_PIN8                  8U
#define GPIOA_PIN9                  9U
#define GPIOA_PIN10                 10U
#define GPIOA_PIN11                 11U
#define GPIOA_PIN12                 12U
#define GPIOA_PIN13                 13U
#define GPIOA_PIN14                 14U
#define GPIOA_PIN15                 15U

#define GPIOB_PIN0                  0U
#define GPIOB_PIN1                  1U
#define GPIOB_PIN2                  2U
#define GPIOB_PIN3                  3U
#define GPIOB_PIN4                  4U
#define GPIOB_PIN5                  5U
#define GPIOB_PIN6                  6U
#define GPIOB_PIN7                  7U
#define GPIOB_PIN8                  8U
#define GPIOB_PIN9                  9U
#define GPIOB_PIN10                 10U
#define GPIOB_PIN11                 11U
#define GPIOB_PIN12                 12U
#define GPIOB_PIN13                 13U
#define GPIOB_PIN14                 14U
#define GPIOB_PIN15                 15U

#define GPIOC_PIN0                  0U
#define GPIOC_PIN1                  1U
#define GPIOC_PIN2                  2U
#define GPIOC_PIN3                  3U
#define GPIOC_PIN4                  4U
#define GPIOC_PIN5                  5U
#define GPIOC_PIN6                  6U
#define GPIOC_PIN7                  7U
#define GPIOC_PIN8                  8U
#define GPIOC_PIN9                  9U
#define GPIOC_PIN10                 10U
#define GPIOC_PIN11                 11U
#define GPIOC_PIN12                 12U
#define GPIOC_PIN13                 13U
#define GPIOC_PIN14                 14U
#define GPIOC_PIN15                 15U

#define GPIOD_PIN0                  0U
#define GPIOD_PIN1                  1U
#define GPIOD_PIN2                  2U
#define GPIOD_PIN3                  3U
#define GPIOD_PIN4                  4U
#define GPIOD_PIN5                  5U
#define GPIOD_PIN6                  6U
#define GPIOD_PIN7                  7U
#define GPIOD_PIN8                  8U
#define GPIOD_PIN9                  9U
#define GPIOD_PIN10                 10U
#define GPIOD_PIN11                 11U
#define GPIOD_PIN12                 12U
#define GPIOD_PIN13                 13U
#define GPIOD_PIN14                 14U
#define GPIOD_PIN15                 15U

#define GPIOE_PIN0                  0U
#define GPIOE_PIN1                  1U
#define GPIOE_PIN2                  2U
#define GPIOE_PIN3                  3U
#define GPIOE_PIN4                  4U
#define GPIOE_PIN5                  5U
#define GPIOE_PIN6                  6U
#define GPIOE_PIN7                  7U
#define GPIOE_PIN8                  8U
#define GPIOE_PIN9                  9U
#define GPIOE_PIN10                 10U
#define GPIOE_PIN11                 11U
#define GPIOE_PIN12                 12U
#define GPIOE_PIN13                 13U
#define GPIOE_PIN14                 14U
#define GPIOE_PIN15                 15U

#define GPIOF_PIN0                  0U
#define GPIOF_PIN1                  1U
#define GPIOF_PIN2                  2U
#define GPIOF_PIN3                  3U
#define GPIOF_PIN4                  4U
#define GPIOF_PIN5                  5U
#define GPIOF_PIN6                  6U
#define GPIOF_PIN7                  7U
#define GPIOF_PIN8                  8U
#define GPIOF_PIN9                  9U
#define GPIOF_PIN10                 10U
#define GPIOF_PIN11                 11U
#define GPIOF_PIN12                 12U
#define GPIOF_PIN13                 13U
#define GPIOF_PIN14                 14U
#define GPIOF_PIN15                 15U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 *
 * The digits have the following meaning:
 *   PMODE:  0 - Input.                    1 - Output.
 *           2 - Alternate function.       3 - Analog.
 *   PUPD:   0 - No pull-up/pull-down.     1 - Pull-up.
 *           2 - Pull-down.
 *   POTYPE: 0 - Push-pull.                1 - Open-drain.
 *   DS:     0 - Lowest.                   1 - Mid1.
 *           2 - Mid2.                     3 - Highest.
 * Please refer to the N32H48x Reference Manual for details.
 *
 * All ports are left as floating inputs by default; the keyboard
 * firmware programs the pads it actually uses at run time.
 */

/*
 * Port A setup.
 */
#define VAL_GPIOAPOD            0x00000000
#define VAL_GPIOAPMODE          0x00000000      /*  PA15...PA0 all inputs */
#define VAL_GPIOAPUPD           0x00000000
#define VAL_GPIOAPOTYPE         0x00000000
#define VAL_GPIOADS             0x00000000

/*
 * Port B setup.
 */
#define VAL_GPIOBPOD            0x00000000
#define VAL_GPIOBPMODE          0x00000000      /*  PB15...PB0 all inputs */
#define VAL_GPIOBPUPD           0x00000000
#define VAL_GPIOBPOTYPE         0x00000000
#define VAL_GPIOBDS             0x00000000

/*
 * Port C setup.
 */
#define VAL_GPIOCODP            0x00000000
#define VAL_GPIOCOPMODE         0x00000000      /*  PC15...PC0 all inputs */
#define VAL_GPIOCOPUPD          0x00000000
#define VAL_GPIOCOPOTYPE        0x00000000
#define VAL_GPIOCODS            0x00000000

/*
 * Port D setup.
 */
#define VAL_GPIODPOD            0x00000000
#define VAL_GPIODPMODE          0x00000000      /*  PD15...PD0 all inputs */
#define VAL_GPIODPUPD           0x00000000
#define VAL_GPIODPOTYPE         0x00000000
#define VAL_GPIODDS             0x00000000

/*
 * Port E setup.
 */
#define VAL_GPIOEPOD            0x00000000
#define VAL_GPIOEPMODE          0x00000000      /*  PE15...PE0 all inputs */
#define VAL_GPIOEPUPD           0x00000000
#define VAL_GPIOEPOTYPE         0x00000000
#define VAL_GPIOEDS             0x00000000

/*
 * Port F setup.
 */
#define VAL_GPIOFPOD            0x00000000
#define VAL_GPIOFPMODE          0x00000000      /*  PF15...PF0 all inputs */
#define VAL_GPIOFPUPD           0x00000000
#define VAL_GPIOFPOTYPE         0x00000000
#define VAL_GPIOFDS             0x00000000

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
