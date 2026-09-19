# tl12blef103 rules.mk

# 蓝牙: 复用 BHQ 驱动
BLUETOOTH_ENABLE = yes
BLUETOOTH_DRIVER = bhq

# vial-qmk 仓库: VIAL_ENABLE 必须为 yes (quantum/via.c:25 会 #error 拒绝纯 VIA 构建)。
# 注意: VIAL_ENABLE 在 data/mappings/info_rules.hjson 里被标 invalid, 放在键盘级
# rules.mk 会让每次构建都报 "no longer a valid option"(实测 python -m qmk.cli
# generate-rules-mk 即触发)。故放在 **keymap 级** rules.mk, 与 kbd67ble_ec /
# tl96mgf072 的做法一致。VIA_ENABLE 同理一并移过去:
# 两者共存是 Vial 的正常用法 (via.c:60 起用 #ifdef VIAL_ENABLE 走 Vial 分支)。
VIA_ENABLE = yes

# Vial 动态键位表
DYNAMIC_KEYMAP_ENABLE = yes

# 不启用 secure 解锁组合键 (quantum/vial.c:43 的 VIAL_UNLOCK_COMBO_ROWS/COLS
# 只在 #ifndef VIAL_INSECURE 下引用)。与 kbd67ble_ec / tl96mgf072 保持一致。
VIAL_INSECURE = yes

# Vial 灯光控制: 让 Vial 直接读写每颗灯珠(颜色/亮度)并切换灯效。
# build_vial.mk:21 会据此加入 quantum/vialrgb.c 与 -DVIALRGB_ENABLE;
# 同时 keymap 的 vial.json 必须把 "lighting" 写成 "vialrgb" 才会出现灯光页。
VIALRGB_ENABLE = yes

# --- 精简 RAM: 关掉用不到的 Vial 特性 ----------------------------------------
# quantum/vial.h 由下列宏派生 VIAL_*_ENABLE (vial.h:94/124/158/204),
# 关闭后对应条目数自动归 0, 静态数组随之消失:
#   tap_dance_actions    448 B   (TAP_DANCE_ENABLE)
#   key_combos + keys    352 B   (COMBO_ENABLE)
#   vial_key_overrides   384 B   (KEY_OVERRIDE_ENABLE)
#   vial_alt_repeat_key  256 B   (REPEAT_KEY_ENABLE)
# 同时这些特性的实现代码也不再编译, flash 另有可观收益。
#
# 额外好处: 关掉 COMBO/TAP_DANCE/KEY_OVERRIDE 后, quantum/keymap_introspection.c
# 里 `#if defined(COMBO_ENABLE) && !defined(VIAL_COMBO_ENABLE)` 这类分支不再展开,
# 也就不会引用编译期不存在的 key_combos[] / tap_dance_actions[] / key_overrides[]
# (之前正是这个原因才不得不在 keymap 里定义 VIAL_*_ENTRIES)。
TAP_DANCE_ENABLE    = no
COMBO_ENABLE        = no
KEY_OVERRIDE_ENABLE = no
# alt-repeat 本就由 repeat_key 派生 —— quantum/vial.h:204 的条件是
#   #if defined(REPEAT_KEY_ENABLE) && !defined(NO_ALT_REPEAT_KEY)
# 故直接关 REPEAT_KEY_ENABLE 即可让 VIAL_ALT_REPEAT_KEY_ENABLE 消失,
# 无需另加 -DNO_ALT_REPEAT_KEY 旁路。本键盘 keymap 未使用 QK_REP, 关闭无影响。
REPEAT_KEY_ENABLE   = no

# kb_common/wireless.c 在 RAW_ENABLE 下才 include "raw_hid.h" 并调用
# raw_hid_send/raw_hid_receive (0x97 ACK 转发、0x27 VIA 透传都走这里)。
# 不置该宏时 wireless.c 会报 raw_hid_* 隐式声明。
RAW_ENABLE = yes

# 低功耗: F103 STOP, 矩阵 GPIO 唤醒
KB_LPM_ENABLED = yes
KB_LPM_DRIVER  = lpm_stm32f1

# 电池 ADC 采样 + 低电量保护
KB_CHECK_BATTERY_ENABLED = yes

# 链接脚本: 本板为 **STM32F103C8T6 = 64KB flash**。
# stm32duino(Maple) 引导驻留在 0x08000000..0x08001FFF (前 8KB),
# 故应用固件从 0x08002000 开始, 用户区 = 57344 字节。
# (x8 = 64k / xB = 128k, 见 platforms/chibios/boards/STM32_F103_STM32DUINO/ld/)
MCU_LDSCRIPT = STM32F103x8

KB_DEBUG = no

# kb_common 源码查找路径
VPATH += $(TOP_DIR)/keyboards/keymagichorse

# 蓝牙/电池/多主机/低功耗公共框架
include keyboards/keymagichorse/kb_common/kb_common.mk
