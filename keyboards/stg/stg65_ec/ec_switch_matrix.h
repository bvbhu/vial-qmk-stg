/* Copyright 2026 vial-qmk-wireless contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* stg65_ec 的 EC 钩子声明，供 kb_common/lpm_core.c 的 LPM_EC_MATRIX 分支调用
 * （RTC 唤醒循环：HAL 重初始化 -> ec_init -> ec_matrix_scan）。
 * 实现在 stg65_ec_matrix.c。 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "matrix.h"

int  ec_init(void);
bool ec_matrix_scan(matrix_row_t current_matrix[]);
void ec_print_matrix(void);
