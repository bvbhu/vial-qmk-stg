DEFERRED_EXEC_ENABLE = yes
KEYCODE_STRING_ENABLE = yes

ANALOG_DRIVER_REQUIRED = yes
ANALOG_ENABLE = yes

# 按功能拆分的源文件（模拟矩阵；键程映射模型在 quantum/analog/analog_model_*.h）
SRC += tl96mgf072_matrix.c
