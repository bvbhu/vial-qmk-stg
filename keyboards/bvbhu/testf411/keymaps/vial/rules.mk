# Copyright 2024 bvbhu
#
# testf411 Vial keymap.
#
# The Vial fork rejects a plain VIA build (quantum/via.c:25 #errors), so
# VIAL_ENABLE is mandatory.  VIAL_INSECURE skips the 5 s unlock combo --
# handy on a bare pin-test board, drop it if you want the usual protection.
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes
VIALRGB_ENABLE = yes
