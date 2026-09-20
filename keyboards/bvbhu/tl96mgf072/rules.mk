DEFERRED_EXEC_ENABLE = yes
KEYCODE_STRING_ENABLE = yes

# 键程映射模型选编：声明 ANALOG_MODEL 即启用整个模拟子系统，并编入
# analog_model_<name>.c + 平台 ADC 驱动(ANALOG_DRIVER_REQUIRED 由它派生)。
#   isf         平方反比-快速(磁轴)   linear_fast 线性(乘+移)   linear 线性(纯除法)
# ISF 还会在构建期生成查表常量(util/analog_isf_gen.py -> analog_isf_table.inc)。
ANALOG_MODEL = isf

# 按功能拆分的源文件（模拟矩阵；键程映射模型在 quantum/analog/analog_model_*.c）
SRC += tl96mgf072_matrix.c
