/* Copyright 2026 Talo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

/* N32H481REL7K 引脚虚焊测试板 —— 引脚清单与注意事项
 *
 * 板子把 LQFP64 的引脚通过左右两排 2.54mm 排针引出（丝印见 _pcb.png）。
 * 测试固件把每个可用的 IO 配成"接地按键"（内部上拉，短接到 GND 即按下），
 * 键码从 26 个字母 + 10 个数字里循环挑选。
 *
 * 排除（不参与按键矩阵）：
 *   H0 / H1    —— 32.768kHz 低速晶振 OSC_IN / OSC_OUT（丝印 "H0/OSCI"、"H1/OSCO"）
 *   B14 / B15  —— 用户指定排除
 *   A11 / A12  —— USB DM / DP（键盘必需）
 *   A13 / A14  —— SWDIO / SWCLK；焊了按键就放弃 SWD 调试，只能 J-Link / 串口烧录
 *   NRST / VDD / VSS / VCAP / BOOT0 —— 非 IO
 *
 * 注意：GPIOG/GPIOH 在本板未用，板级 board.h 已把 N32_HAS_GPIOG/H 置 FALSE。
 */

#include_next <config.h>
