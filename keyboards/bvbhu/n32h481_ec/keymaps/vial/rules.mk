VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes

BACKLIGHT_ENABLE = no

# 片内 Flash EEPROM(drivers/eeprom/eeprom_custom.c,双页镜像磨损均衡)
EEPROM_DRIVER = custom
SRC += drivers/eeprom/eeprom_custom.c
