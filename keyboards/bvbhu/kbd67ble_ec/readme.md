# kbd67_ec

A 65% multi-layout EC keyboard (there are 74 keys in total) with 0 RGB in capslock key.
As for not to change too much code so just keep the RGB code from KeyMagicHorse.
This keyboard use 25mhz HSE and STM32F411 as MCU.

- Keyboard Maintainer: https://github.com/KeyMagicHorse/qmk_firmware
- Hardware Supported: kbd67_ec
- Hardware Availability:
Make example for this keyboard (after setting up your build environment):
    ```
    make kippper/kbd67_ec:default
    ```
    ```
    make kippper/kbd67_ec:ble
    ```
See [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) then the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.

## Bootloader

Enter the bootloader in 2 ways:

- **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key which is Escape in this keyboard) and plug in the keyboard
- **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available.
