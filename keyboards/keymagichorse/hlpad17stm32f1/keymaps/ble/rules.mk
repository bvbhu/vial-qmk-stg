
# 是否使能低功耗
KB_LPM_ENABLED = yes
KB_LPM_DRIVER = lpm_stm32f1
# 是否使能QMK端读取电池电压
KB_CHECK_BATTERY_ENABLED = yes
# 开启键盘层DEBUG  这里是用RTT输出日记的
KB_DEBUG = no

# 打开矩阵rgb闪烁 功能	
RGB_MATRIX_CUSTOM_BLINK_EFFECT = yes
# 是否打开矩阵rgb显示电量 功能
RGB_MATRIX_CUSTOM_BATTERY_EFFECT = yes

include keyboards/keymagichorse/kb_common/kb_common.mk

VIA_ENABLE = yes
