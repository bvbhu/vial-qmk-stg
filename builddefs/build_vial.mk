# Copyright 2023 Ilya Zhuravlev
# SPDX-License-Identifier: GPL-2.0-or-later

QMK_SETTINGS ?= yes
TAP_DANCE_ENABLE ?= yes
ifeq ($(strip $(TAP_DANCE_ENABLE)), yes)
    OPT_DEFS += -DTAPPING_TERM_PER_KEY
endif
CAPS_WORD_ENABLE ?= yes
COMBO_ENABLE ?= yes
KEY_OVERRIDE_ENABLE ?= yes
LAYER_LOCK_ENABLE ?= yes
REPEAT_KEY_ENABLE ?= yes
SRC += $(QUANTUM_DIR)/vial.c
OPT_DEFS += -DVIAL_ENABLE -DNO_DEBUG -DSERIAL_NUMBER=\"vial:f64c2b3c\" -DCAPS_WORD_INVERT_ON_SHIFT

ifeq ($(strip $(VIAL_INSECURE)), yes)
    OPT_DEFS += -DVIAL_INSECURE
endif

ifeq ($(strip $(VIALRGB_ENABLE)), yes)
    SRC += $(QUANTUM_DIR)/vialrgb.c
    OPT_DEFS += -DVIALRGB_ENABLE
endif

# Analog (静电容/磁轴) — 行程 0-255，协议命令详见 docs/vial-analog-protocol.md
#
# kb 只需在 rules.mk 声明 ANALOG_MODEL = <name>，即启用整个模拟子系统：
#   isf         平方反比-快速(磁轴)，构建期另生成查表常量(见 build_keyboard.mk)
#   linear_fast 线性(乘+移替代除法)
#   linear      线性(纯除法，EC 兜底)
# 编入 analog_core.c + 对应 analog_model_<name>.c；-DANALOG_MODEL(无值宏)供
# vial.c/keyboard.c/nvm 的 #ifdef 门控(协议命令/EEPROM 区)。
#
# ANALOG_DRIVER_REQUIRED 是上游 QMK 变量(common_features.mk:1010 据此编入平台
# ADC 驱动 analog.c + HAL_USE_ADC)，此处由 ANALOG_MODEL 派生，板级无需另写。
ifneq ($(strip $(ANALOG_MODEL)),)
    ANALOG_DRIVER_REQUIRED = yes
    SRC += $(QUANTUM_DIR)/analog/analog_core.c
    SRC += $(QUANTUM_DIR)/analog/analog_model_$(ANALOG_MODEL).c
    COMMON_VPATH += $(QUANTUM_DIR)/analog
    OPT_DEFS += -DANALOG_MODEL
endif

ifeq ($(strip $(QMK_SETTINGS)), yes)
    AUTO_SHIFT_ENABLE := yes
    SRC += $(QUANTUM_DIR)/qmk_settings.c
    OPT_DEFS += -DQMK_SETTINGS \
        -DAUTO_SHIFT_NO_SETUP -DAUTO_SHIFT_REPEAT_PER_KEY -DAUTO_SHIFT_NO_AUTO_REPEAT_PER_KEY \
        -DPERMISSIVE_HOLD_PER_KEY -DHOLD_ON_OTHER_KEY_PRESS_PER_KEY -DQUICK_TAP_TERM_PER_KEY -DRETRO_TAPPING_PER_KEY \
        -DCOMBO_TERM_PER_COMBO -DCHORDAL_HOLD -DFLOW_TAP_TERM=321
endif

# Generate Vial layout definition header from JSON
$(QUANTUM_DIR)/vial.c: $(INTERMEDIATE_OUTPUT)/src/vial_generated_keyboard_definition.h

$(INTERMEDIATE_OUTPUT)/src/vial_generated_keyboard_definition.h: $(KEYMAP_PATH)/vial.json
	python3 util/vial_generate_definition.py $(KEYMAP_PATH)/vial.json $(INTERMEDIATE_OUTPUT)/src/vial_generated_keyboard_definition.h
