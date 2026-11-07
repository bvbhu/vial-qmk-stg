/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

// The N32H48x SYSTICK driver supports only OSAL_ST_MODE_PERIODIC.
// Forcing CH_CFG_ST_TIMEDELTA to 0 selects PERIODIC mode in the OSAL
// (see lib/chibios/os/hal/osal/rt-nil/osal.h). QMK does not use the
// free-running alarm feature, so PERIODIC is safe here.
// Same as bvbhu/n32h481_demo and bvbhu/n32h481_iotest.
#define CH_CFG_ST_TIMEDELTA 0

#include_next <chconf.h>
