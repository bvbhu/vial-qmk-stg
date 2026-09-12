MAKEFLAGS += -j10

VPATH += $(TOP_DIR)/keyboards/keymagichorse

# Vial analog（0xF0-0xF5 行程调节）：启用 analog 核心层 + ADC 驱动
ANALOG_ENABLE = yes
ANALOG_DRIVER_REQUIRED = yes

# EC 静电容矩阵：板级提供 matrix_init_custom / matrix_scan_custom，
# matrix_init/scan/get_row/print/debounce 由 quantum/matrix_common.c 提供
CUSTOM_MATRIX = lite

SRC += kbd67ble_ec_matrix.c
