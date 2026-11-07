# tl12wlh481 default keymap rules.mk
#
# Vial must be enabled: quantum/via.c refuses a pure-VIA build with #error.
# Keep it (and VIA_ENABLE) at keymap level because QMK's linter treats "via"
# as a keymap-only feature and rejects it at keyboard level.
VIA_ENABLE  = yes
VIAL_ENABLE = yes

# Trim the Vial features this 12-key board does not use.  These must be
# explicit: with VIAL_ENABLE on they default to enabled, which pulls in the
# keymap_introspection arrays and costs ~6K of flash.  They live here rather
# than in the keyboard rules.mk because an explicit "= no" still shows up in
# the keyboard-level info.json features, which the linter rejects.
TAP_DANCE_ENABLE    = no
COMBO_ENABLE        = no
KEY_OVERRIDE_ENABLE = no
REPEAT_KEY_ENABLE   = no
