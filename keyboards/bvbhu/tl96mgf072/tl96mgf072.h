#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "wait.h"

// tl96mgf072.c
void keyboard_post_init_kb(void);

// tl96mgf072_matrix.c
void calibrate_matrix(void); // 开机校准：采样各键静置读数(top_reading)；触底锚点走持久化
void matrix_init(void);
uint8_t matrix_scan(void);
matrix_row_t matrix_get_row(uint8_t row);
void matrix_print(void);
__attribute__((weak)) void bootmagic_scan(void);
