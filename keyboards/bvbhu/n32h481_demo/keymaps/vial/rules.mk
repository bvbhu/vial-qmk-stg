# Vial protocol for the N32H481 Sensor Demo (2 keys / 3 lamps).
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes

BACKLIGHT_ENABLE = no

# On-chip Flash EEPROM (drivers/eeprom/eeprom_custom.c, dual-page
# mirrored wear levelling) so Vial config survives power loss.
EEPROM_DRIVER = custom
SRC += drivers/eeprom/eeprom_custom.c
