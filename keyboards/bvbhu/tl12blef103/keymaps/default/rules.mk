# SPDX-License-Identifier: GPL-2.0-or-later
#
# tl12blef103 default keymap rules.mk

# Vial 必须开启: quantum/via.c:25 会 #error 拒绝纯 VIA 构建。
# 放在 keymap 级而非键盘级: data/mappings/info_rules.hjson:59 把键盘级
# VIAL_ENABLE 标为 invalid, 每次构建都会报 "no longer a valid option"。
# 二者共存是 Vial 的正常用法 (via.c:60 起用 #ifdef VIAL_ENABLE 走 Vial 分支)。
VIA_ENABLE = yes
VIAL_ENABLE = yes
