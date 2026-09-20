VPATH += $(TOP_DIR)/keyboards/keymagichorse

# Vial analog（0xF0-0xF5 行程调节）：声明 ANALOG_MODEL = linear 启用模拟子系统
# (analog_core.c + analog_model_linear.c + 平台 ADC 驱动)。
ANALOG_MODEL = linear

# EC 静电容矩阵：kb 提供 matrix_init_custom / matrix_scan_custom，
# matrix_init/scan/get_row/print/debounce 由 quantum/matrix_common.c 提供
CUSTOM_MATRIX = lite

SRC += kbd67ble_ec_matrix.c
