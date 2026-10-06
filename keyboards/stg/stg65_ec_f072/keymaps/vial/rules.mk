VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes
# 是否使能低功耗
KB_LPM_ENABLED = yes
# STM32F072 用 F0 芯片层（HSI/USB HSI48/LPDS + RTCv2 周期唤醒），
# 不能沿用 F411 的 lpm_stm32f4_rtc_ec_v1（PWR_CR_MRLVDS/LPLVDS 等 F0 上不存在）
KB_LPM_DRIVER = lpm_stm32f0_rtc_ec_v1
# 是否使能QMK端读取电池电压
KB_CHECK_BATTERY_ENABLED = yes
# 开启键盘层DEBUG
KB_DEBUG = no

BACKLIGHT_ENABLE = no

include keyboards/keymagichorse/kb_common/kb_common.mk
