DEFERRED_EXEC_ENABLE = yes
KEYCODE_STRING_ENABLE = yes

# F411 的 USB 是 OTG1，只有 3 个可用端点(STM32_OTG1_ENDPOINTS=3，见
# lib/chibios/os/hal/ports/STM32/STM32F4xx/stm32_registry.h)，而 F072 的 USBv1 有 7 个。
# 本板同时开了 EXTRAKEY/NKRO(共享端点)、Vial 的 RAW HID 和 CONSOLE，按默认分配会要 4 个，
# 触发 tmk_core/protocol/usb_descriptor.h:282 的 "not enough available endpoints" 编译错误。
# 把键盘上报并到共享端点后正好压到 3 个：RAW=1、SHARED=2、CONSOLE=3。
KEYBOARD_SHARED_EP = yes

# 键程映射模型选编：声明 ANALOG_MODEL 即启用整个模拟子系统，并编入
# analog_model_<name>.c + 平台 ADC 驱动(ANALOG_DRIVER_REQUIRED 由它派生)。
#   isf         平方反比-快速(磁轴)   linear_fast 线性(乘+移)   linear 线性(纯除法)
# ISF 还会在构建期生成查表常量(util/analog_isf_gen.py -> analog_isf_table.inc)。
ANALOG_MODEL = isf

# 按功能拆分的源文件（模拟矩阵；键程映射模型在 quantum/analog/analog_model_*.c）
SRC += tl96mgf411_matrix.c
