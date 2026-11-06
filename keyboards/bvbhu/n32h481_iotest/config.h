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
 * 引脚表按 N32H481 数据手册 Table 3-1（LQFP64）重建：本封装只邦定了 56 个 GPIO，
 * 去掉板载必需功能后剩 47 个可测脚（见 keymap.c 顶部注释）。
 *
 * 排除（不参与按键矩阵）：
 *   H0 / H1    —— 8MHz 高速晶振 HSE（OSC_IN / OSC_OUT），不是 32.768kHz
 *   B12 / B13  —— USB HS VBUS / OTG ID
 *   B14 / B15  —— USB HS D- / D+（键盘通信必需）
 *   A13 / A14  —— SWDIO / SWCLK（保留 SWD 以便反复烧录）
 *   B7         —— BOOT0（硬件启动模式脚）
 *   NRST / VDD / VSS / VDDA / VSSA / VREF+ —— 非 IO
 *
 * 本封装未邦定（这些脚物理上不存在，之前的 86 脚表错在把 16 位端口当全邦定）：
 *   A10 A11 A12、B6 B10 B11、C6..C15、D0..D7 D15、E0..E6 E15、F0 F1 F2 F6 F11..F15、
 *   整个 GPIOG
 *
 * 注意：A0(RGB) / A1,A2(KEY) / A3(LED+) / A4(LED-) 挂着板载外设网络，
 *   单独测试时读数可能受板载电路影响，但短接 GND 依然能可靠触发。
 */

#include_next <config.h>
