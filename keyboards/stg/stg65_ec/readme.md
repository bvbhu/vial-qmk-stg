# STG65_EC

A 65% multi-layout EC (electrocapacitive) keyboard (74 keys in total) with 1 RGB LED in the capslock key, STM32F411 + BHQ Bluetooth (BLE) + Vial analog adjustable actuation.

- Keyboard Maintainer: [STG](https://github.com/KeyMagicHorse/qmk_firmware)
- Hardware Supported: STG65_EC
- Hardware Availability: STG

Make example for this keyboard (after setting up your build environment):

    ```
    make stg/stg65_ec:vial_ble
    ```

See [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) then the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.

## Bootloader

Enter the bootloader in 2 ways:

- **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key which is Escape in this keyboard) and plug in the keyboard
- **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available.
