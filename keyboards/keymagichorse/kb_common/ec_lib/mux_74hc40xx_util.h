/* Copyright 2025 keymagichorse
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

// 生成 MUX 通道选择值：高4位为 MUX 编号，低4位为通道编号
#define MUX_ENCODE(mux_index, channel)   (((uint8_t)(mux_index) & 0x0F) << 4 | ((uint8_t)(channel) & 0x0F))
// 从组合值中提取 MUX 编号（高4位）
#define MUX_GET_INDEX(value)             (((value) >> 4) & 0x0F)
// 从组合值中提取通道编号（低4位）
#define MUX_GET_CHANNEL(value)           ((value) & 0x0F)