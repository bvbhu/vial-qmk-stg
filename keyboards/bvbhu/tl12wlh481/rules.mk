# tl12wlh481 -- TL12 wireless matrix board on N32H481RE.
# Layout / RGB layout / Vial layout are copied verbatim from bvbhu/tl12blef103.
# The MCU-side differences (ChibiOS SERIAL, WS2812 bit-bang, Flash EEPROM,
# EP0 SET_REPORT RAW HID) come from bvbhu/n32h481_demo.

# --- Wireless: BHQ bridge over USART2 (128000 8N1) -------------------------
BLUETOOTH_ENABLE = yes
BLUETOOTH_DRIVER = bhq

# --- Vial -------------------------------------------------------------------
# VIA_ENABLE and VIAL_ENABLE both live in keymaps/*/rules.mk: QMK's linter
# treats "via" as a keymap-only feature and rejects it at keyboard level.
DYNAMIC_KEYMAP_ENABLE = yes
VIAL_INSECURE         = yes
VIALRGB_ENABLE        = yes

# tap_dance / combo / key_override / repeat_key are all off by default and are
# left unset on purpose: writing "TAP_DANCE_ENABLE = no" still surfaces them in
# the generated info.json, which the linter rejects at keyboard level.

# kb_common/wireless.c forwards 0x97 ACKs and 0x27 VIA traffic through
# raw_hid_send()/raw_hid_receive(); without RAW_ENABLE those are implicit
# declarations and the build fails.
RAW_ENABLE = yes

# --- Low power / battery ----------------------------------------------------
# kb_common only ships LPM chip backends for STM32F0/F1/F4 and AT32F4
# (lpm_chip_*.h / matrix_sleep_default_stm32.c), and battery.c is written
# against the STM32 ADC API.  The N32 port has neither, so both stay OFF until
# a N32 backend exists.  With them off, bhq_common.c/wireless.c compile their
# guarded branches out and the keyboard never sleeps; USB mode is unaffected.
KB_LPM_ENABLED           = no
KB_CHECK_BATTERY_ENABLED = no
KB_DEBUG                 = no

# --- N32H481 platform -------------------------------------------------------
# On-chip Flash dual-page mirror for dynamic keymap / VialRGB settings.
EEPROM_DRIVER = custom
SRC += drivers/eeprom/eeprom_custom.c

# N32 port has no PWM/GPT LLD: WS2812 is bit-banged with DWT timing.
# ws2812_n32.c lives on the platform VPATH, hence the bare filename.
SRC += ws2812_n32.c

# Generous stacks, matching bvbhu/n32h481_demo (Vial + RAW HID + RGB).
USE_PROCESS_STACKSIZE    = 0x8000
USE_EXCEPTIONS_STACKSIZE = 0x2000

# --- Shared BHQ / wireless / transport framework ----------------------------
VPATH += $(TOP_DIR)/keyboards/keymagichorse
include keyboards/keymagichorse/kb_common/kb_common.mk
