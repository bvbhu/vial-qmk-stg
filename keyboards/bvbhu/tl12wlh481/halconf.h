/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

/* kb_common (lpm_core.c / matrix_sleep_*.c) uses the PAL event API
 * (palEnableLineEvent), which hal_pal.h hides behind
 * PAL_USE_CALLBACKS || PAL_USE_WAIT.  LPM is disabled on this board for now,
 * but keep the switches on so the shared framework compiles unchanged if
 * a N32 LPM driver is added later.  The N32 GPIOv1 LLD implements them. */
#define PAL_USE_CALLBACKS TRUE
#define PAL_USE_WAIT      TRUE

/* BHQ bridge UART: USART3 / SD3 (N32 USARTv1 SERIAL LLD).  common_features.mk
 * also adds -DHAL_USE_SERIAL=TRUE because BLUETOOTH_DRIVER=bhq sets
 * UART_DRIVER_REQUIRED; undef/redefine keeps the template from redefining it. */
#undef HAL_USE_SERIAL
#define HAL_USE_SERIAL TRUE

#undef HAL_USE_USB
#define HAL_USE_USB TRUE

#include_next <halconf.h>
