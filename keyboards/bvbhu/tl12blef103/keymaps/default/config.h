/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * tl12blef103 default keymap —— Vial 相关配置
 */
#pragma once

/* Vial 键盘唯一标识 (8 字节 CRC64 派生值)。
 * 每把键盘/每个变体必须唯一, 否则 Vial 会认成同一把键盘而写错动态键位。 */
#define VIAL_KEYBOARD_UID {0x54, 0x4C, 0x31, 0x32, 0x42, 0x4C, 0x45, 0x01}

#define DYNAMIC_KEYMAP_LAYER_COUNT 4

/* 不用定义 VIAL_COMBO/TAP_DANCE/KEY_OVERRIDE_ENTRIES: 这三个特性已在键盘级
 * rules.mk 整体关闭, quantum/vial.h 会把条目数定为 0。重新启用时先开 *_ENABLE,
 * 再按需定义条目数即可。 */
