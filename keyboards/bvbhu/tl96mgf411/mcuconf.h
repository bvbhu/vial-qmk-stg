/* Copyright 2023 Cipulot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include_next <mcuconf.h>

/* 模拟矩阵：行引脚逐个读 ADC1，故必须打开 ADC1。 */
#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE

/* 通用 F411 板的 mcuconf.h 按 Nucleo-F411RE 的 HSE = 8 MHz 配 PLL
 * (PLLM=4, PLLN=96, PLLP=2, PLLQ=4)。本板 HSE 见 board.h，故按 STM32_HSECLK 重算：
 *   VCO in  = HSE / PLLM  = 1 MHz
 *   VCO out = 1 MHz * 384 = 384 MHz   (规格 100..432 MHz)
 *   SYSCLK  = VCO / PLLP  = 96 MHz    (F411 上限 100 MHz)
 *   USB     = VCO / PLLQ  = 48 MHz
 * ADC 时钟仍走 ADC_CCR_ADCPRE_DIV4：PCLK2 = 96 MHz → 24 MHz（F411 上限 36 MHz），无需改。 */
#undef STM32_PLLM_VALUE
#undef STM32_PLLN_VALUE
#undef STM32_PLLP_VALUE
#undef STM32_PLLQ_VALUE

#define STM32_PLLM_VALUE (STM32_HSECLK / 1000000)
#define STM32_PLLN_VALUE 384
#define STM32_PLLP_VALUE 4
#define STM32_PLLQ_VALUE 8
