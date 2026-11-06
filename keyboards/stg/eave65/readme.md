# EAVE65

A 65% multi-layout EC (electrocapacitive) keyboard (74 keys across the layout options) with 1 RGB LED,
**STM32F411CEU6** + BHQ Bluetooth (BLE, CH582F) + Vial analog adjustable actuation.

This board uses a new PCB ("EAVE65-EC", schematic V1.0 dated 2026-11-03) but the same hardware
design as [`stg/stg65_ec`](../stg65_ec/readme.md): same MCU, same EC sensing front end, same mux
wiring, same 5×16 matrix and the same physical layouts. The firmware-visible differences are the
USB PID, the product/BLE name and the Vial UID, so the source is a copy of `stg65_ec` with those
items updated. The schematic files are kept outside the firmware tree and are not committed here.

- Keyboard Maintainer: [STG](https://github.com/KeyMagicHorse/qmk_firmware)
- Hardware Supported: EAVE65 (EAVE65-EC PCB)
- Hardware Availability: STG

Make example for this keyboard (after setting up your build environment):

    ```
    make stg/eave65:vial       # USB keymap
    make stg/eave65:vial_ble   # BLE keymap
    ```

See [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) then the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.

## What differs from `stg65_ec`

| Item | `stg65_ec` | `eave65` |
|---|---|---|
| PCB | STG65_EC | EAVE65-EC (schematic V1.0, 2026-11-03) |
| USB identity | VID `0x6829` / PID `0x6501` | VID `0x6829` / PID **`0x6502`** |
| USB product name | `STG65_EC` | `EAVE65` |
| BLE advertising name | `stg65_ec` | `eave65` |
| Vial UID | `{0x35,0x27,0x09,0xFF,0x9D,0xE5,0xC4,0x2A}` | **new** `{0x64,0x0F,0xDE,0x45,0x7B,0x27,0x58,0x77}` (own identity; `.vil` layouts are not interchangeable between the two boards) |
| Source files | `stg65_ec.c`, `stg65_ec_matrix.c` | `eave65.c`, `eave65_matrix.c` |

Everything else — MCU, clock tree, EC matrix pins, mux pins and channel order, discharge/sense/op-amp
pins, BHQ bridge pins, battery/USB-sense pins, WS2812 pins, LED config, debounce, analog factory
calibration values, both keymaps and `vial.json` — is unchanged.

## Hardware pin map (verified against the EAVE65-EC schematic)

| Function | Pins |
|---|---|
| Matrix rows | `ROW0..ROW4` = PB6, PA13, PB0, PB1, PA10 (100 Ω series each) |
| Mux (CD74HC4051 ×2) | `EN-PIN-0/1` = PB9/PB8 (active low), `MUX-0/1/2` = PB4/PB3/PA15 |
| Mux channel order | mux0 `{1,0,3,2,4,6,5,7}` (COL0..7), mux1 `{1,2,0,3,4,5,7,6}` (COL8..15) |
| EC analog front end | `ADC` = PA4 (SGM8053XN6 output), `DISCHARGE` = PA5, `OPA_EN` = PA6 (SHDN, active high) |
| Bluetooth bridge (CH582F) | `BHQ_IRQ` = PA1, `BHQ_INT` = PA0, `ST_TX`/`ST_RX` = PA2/PA3 (USART2, AF7) |
| Power sensing | `USB_PW` = PB7 (high = USB present), `BAT_ADC` = PA7 |
| RGB | WS2812 data = PB15, WS2812 power switch = PB14 |
| Crystal | 16 MHz HSE (`board.h` / `mcuconf.h`: PLL M=16, N=192, P=4 → 48 MHz) |
| Matrix geometry | 5 rows × 16 cols, 71 keys per layout; unused positions listed in `config.h` |

## Bootloader

Enter the bootloader in 2 ways:

- **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key which is Escape in this keyboard) and plug in the keyboard
- **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available.
