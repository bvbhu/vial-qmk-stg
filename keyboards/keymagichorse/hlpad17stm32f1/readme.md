# HLPAD17

A PAD multi-layout keyboard (there are 17 keys in total) with 1 RGB in capslock key.
This keyboard use 16mhz HSE and STM32F103CB as MCU.

- Keyboard Maintainer: https://github.com/KeyMagicHorse/qmk_firmware
- Hardware Supported: HLPAD17_STM32F103CBT6
- Hardware Availability:
  Make example for this keyboard (after setting up your build environment):

  ```
  make keymagichorse/hlpad17stm32f1:default
  ```

  ```
  make keymagichorse/hlpad17stm32f1:ble
  ```

  See [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) then the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.

## Bootloader

Enter the bootloader in 2 ways:

- **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key which is Escape in this keyboard) and plug in the keyboard
- **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available.
