VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes
# 是否使能低功耗
KB_LPM_ENABLED = yes
KB_LPM_DRIVER = lpm_stm32f4_rtc_ec_v1
# 是否使能QMK端读取电池电压
KB_CHECK_BATTERY_ENABLED = yes
# 开启键盘层DEBUG
KB_DEBUG = no

BACKLIGHT_ENABLE = no

include keyboards/keymagichorse/kb_common/kb_common.mk
