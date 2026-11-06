# Vial protocol for the N32H481 Sensor Demo (2 keys / 3-LED WS2812 strip).
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes

BACKLIGHT_ENABLE = no

# On-chip Flash EEPROM (drivers/eeprom/eeprom_custom.c, dual-page
# mirrored wear levelling) so Vial config survives power loss.
EEPROM_DRIVER = custom
SRC += drivers/eeprom/eeprom_custom.c

# Vial RGB page: exposes per-LED colour control in the Vial GUI,
# which requires both this flag and "lighting": "vialrgb" in vial.json.
VIALRGB_ENABLE = yes
