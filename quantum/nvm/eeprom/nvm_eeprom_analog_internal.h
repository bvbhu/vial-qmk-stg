// Copyright 2026 bvbhu
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Analog(磁轴/EC)持久化区在 EEPROM 里的槽位 —— 分配链与寻址共用本头文件：
 *
 *   quantum/nvm/eeprom/nvm_dynamic_keymap.c  动态宏区尾部按 VIAL_ANALOG_EEPROM_SIZE 缩让(借出动作)
 *   quantum/analog/analog_core.c             用 VIAL_ANALOG_EEPROM_ADDR 定位自己的区首
 *
 * 大小由 analog_core.h 的 ANALOG_PERSIST_SIZE 按矩阵尺寸自动推导，kb config.h
 * 无需参与；ANALOG_MODEL 关闭时为 0，EEPROM 布局与原版完全一致。
 * 区首锚在 EEPROM 末尾(向前数)，不依赖 VIA/动态键位是否启用。
 *
 * 不取偶、不掩码：ANALOG_PERSIST_SIZE 本身已是偶数(头/记录/全局三段皆偶，
 * analog_core.h §9 有静态断言把关)。这里若再套 (SIZE+1)&~1 只是掩盖"尺寸为奇"
 * 这一事实——真正要守的是区首地址为偶，而区首 = TOTAL - SIZE，故守 SIZE 为偶即可。 */

#include "eeprom.h" // TOTAL_EEPROM_BYTE_COUNT

#ifdef ANALOG_MODEL
#    include "analog/analog_core.h" // COMMON_VPATH 由 build_vial.mk 在 ANALOG_MODEL 时提供
#    define VIAL_ANALOG_EEPROM_SIZE (ANALOG_PERSIST_SIZE)
#else
#    define VIAL_ANALOG_EEPROM_SIZE 0
#endif

_Static_assert((VIAL_ANALOG_EEPROM_SIZE % 2) == 0, "analog 区大小必须为偶，否则区首(末尾向前数)落在奇地址，8 字节记录全错位");

#define VIAL_ANALOG_EEPROM_ADDR (TOTAL_EEPROM_BYTE_COUNT - VIAL_ANALOG_EEPROM_SIZE)
