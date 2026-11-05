# n32h481_demo uses a custom matrix scan (CUSTOM_MATRIX = lite):
# the slide switch on KEY1/KEY2 is read as a 1-key matrix (KEY2 driven
# low, KEY1 pulled up) and the hall sensor as a second key.
# The keyboard-named source n32h481_demo.c (matrix + rgb_matrix driver)
# is picked up automatically by the build; uart_dbg.c must be listed.
CUSTOM_MATRIX = lite
SRC += uart_dbg.c

USE_PROCESS_STACKSIZE = 0x8000
USE_EXCEPTIONS_STACKSIZE = 0x2000

# WS2812 strip on PC3 (bit-banged; the N32 port has no PWM/GPT LLD).
SRC += drivers/ws2812_n32.c

# Vial Analog / vial-stg "Analog (键程)" page.  Declaring ANALOG_MODEL pulls in
# analog_core.c + analog_model_linear.c, the 0xF0-0xF6 protocol extension in
# vial.c, the EEPROM persistence region, and the platform ADC driver
# (ANALOG_DRIVER_REQUIRED -> -DHAL_USE_ADC=TRUE + analog.c).  N32_ADC_USE_ADC1
# is already TRUE in GENERIC_N32H481XE/configs/mcuconf.h.
# The kb overrides the weak analog_model_sw() hook so the mechanical switch is
# clamped to the 0 / ANALOG_MAX_TRAVEL endpoints while the Hall key uses the
# real 12-bit ADC travel curve.
ANALOG_MODEL = linear
