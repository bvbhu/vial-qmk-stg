/* Copyright 2024 bvbhu
 *
 * Vial keymap config.
 *
 * NOTE: VIAL_ENABLE must be set in the keymap rules.mk, not the keyboard
 * rules.mk -- data/mappings/info_rules.hjson:59 marks keyboard-level
 * VIAL_ENABLE as invalid ("no longer a valid option").
 */
#pragma once

/* Unique 8-byte ID; Vial refuses to open the keyboard if this changes. */
#define VIAL_KEYBOARD_UID {0x7D, 0xBA, 0xF4, 0xA3, 0x6F, 0x12, 0x89, 0x13}

/* Vial dynamic keymap: 4 layers (keymap.c defines all four). */
#define DYNAMIC_KEYMAP_LAYER_COUNT 4

/* Room for Vial's combo / tap-dance / key-override editors. */
#define VIAL_COMBO_ENTRIES 16
#define VIAL_TAP_DANCE_ENTRIES 16
#define VIAL_KEY_OVERRIDE_ENTRIES 16
