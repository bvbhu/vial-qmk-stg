# STG65_EC_F072

A 65% multi-layout EC (electrocapacitive) keyboard (74 keys in total) with 1 RGB LED in the capslock key,
**STM32F072xB** + BHQ Bluetooth (BLE) + Vial analog adjustable actuation.

This keyboard is the STM32F072 variant of [`stg/stg65_ec`](../stg65_ec/readme.md); the switch matrix,
pin assignment, layout, keymaps and Vial USB identity are unchanged, only the MCU-specific configuration differs.

- Keyboard Maintainer: [STG](https://github.com/KeyMagicHorse/qmk_firmware)
- Hardware Supported: STG65_EC (STM32F072 version)
- Hardware Availability: STG

Make example for this keyboard (after setting up your build environment):

    ```
    make stg/stg65_ec_f072:vial       # USB keymap
    make stg/stg65_ec_f072:vial_ble   # BLE keymap
    ```

See [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) then the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information.

## What differs from `stg65_ec` (STM32F411)

| Item | `stg65_ec` (F411) | `stg65_ec_f072` (F072) |
|---|---|---|
| Processor | `STM32F411` | `STM32F072` |
| Device identity | VID `0x6829` / PID `0x6501` / product name `STG65_EC` / Vial UID | **identical** — same PCB, only the MCU differs, so host & Vial see the same keyboard |
| SYSCLK | HSE 16MHz → PLL (M16/N192/P4) = **48MHz** | HSI/2 (4MHz) × 12 = **48MHz** (F0 max) |
| USB clock | PLL Q | **HSI48** (`STM32_HSI48_ENABLED TRUE`, required on F0) |
| UART (BHQ bridge) | USART2 on PA2/PA3, AF **7** | USART2 on PA2/PA3, AF **1** |
| LPM driver | `lpm_stm32f4_rtc_ec_v1` | `lpm_stm32f0_rtc_ec_v1` (`lpm_chip_stm32f0.h`) |
| RTC wakeup tick | LSI 32kHz → 2048Hz | LSI 40kHz → 2500Hz |
| HSE pins (LPM) | `H0`/`H1` | `F0`/`F1` (PF0/PF1 = OSC_IN/OSC_OUT) |
| EEPROM emulation | wear-leveling (F4) | legacy emulated flash (F0: 4 × 2KB pages, 4 KB cached in RAM) |
| BLE report queue | `REPORT_BUFFER_QUEUE_SIZE 68` | `32` (largest depth that links on 16 KB SRAM) |
| Battery VREFINT | channel 17, 1.21V | channel 17, 1.22V, `ADC->CCR.VREFEN` |
| ADC | ADCv2, ADC clock 48MHz/4 = **12MHz** | ADCv1, ADC clock HSI14/4 = **3.5MHz** (both scaled to 10-bit by `analog.c`) |
| EC scan rate | ~1.0µs per conversion | ~3.4µs per conversion — full 70-key scan ≈1–2ms instead of ≈0.3–0.7ms; `debounce: 10` (scan-count based) therefore ≈10ms instead of ≈3ms |

Unchanged: EC matrix rows/cols, 74HC4051 mux pins, discharge/sense/op-amp pins, BHQ pins,
battery pin, WS2812 data/power pins, layouts, both keymap contents, USB identity (VID/PID/product
name) and the Vial UID, and every `ANALOG_*` factory calibration value (both ADCs are presented as
10-bit, so the `top≈600 / bottom≈900` ranges carry over).

## Resource usage (measured)

```
$ arm-none-eabi-size -A .build/stg_stg65_ec_f072_vial.elf
.data     1392    0x20000800
.bss     11864    0x20000D70
.heap       56    0x20003FC8      <- 0x20004000 = end of the 16 KB SRAM
```

| Item | STM32F072 (this board) | STM32F411 (`stg65_ec`) |
|---|---|---|
| Flash (firmware image) | 61536 B / 131072 B (47 %) | ~64 KB / 512 KB |
| `.data` + `.bss` | 1392 + 11864 = 13256 B | 1500 + 13968 = 15468 B |
| ChibiOS stacks (main 0x400 + process 0x800) | 3072 B | 3072 B |
| Heap headroom | **56 B** | 112528 B |

The F072's 16 KB SRAM is therefore fully committed: the `WordBuf` of the F0 EEPROM emulation
alone takes 4096 B (the legacy emulated-flash driver caches the whole 4 KB EEPROM in RAM; the
STM32F4 boards use the wear-leveling driver instead, which needs ~20 B), plus 1280 B of Vial
analog per-key state, 1184 B of Vial tap-dance/combo/key-override tables, 1536 B of BLE report
queue and the 3072 B stack reserve.

Consequences and guidance:

- The fit is enforced by the linker: adding statics that exceed 16 KB fails the **build**
  (`cannot move location counter backwards`), it does not silently corrupt RAM at runtime.
- The 56 B heap is nominal — nothing in the image allocates (`malloc` is only present because
  newlib's stdio object files are pulled in; no object file references it), and QMK/ChibiOS do
  not use the heap.
- If you need RAM headroom for new features, reduce `REPORT_BUFFER_QUEUE_SIZE` (48 B per entry,
  `config.h`) and/or `VIAL_COMBO/TAP_DANCE/KEY_OVERRIDE_ENTRIES` (~74 B per entry, keymap
  `config.h`). Do **not** shrink the EEPROM emulation: Vial's layout needs ~2.3 KB of the 4 KB
  (dynamic keymaps 640 B + Vial tables 480 B + analog persistence 978 B + ≥100 B of macros), so
  `FEE_DENSITY_BYTES 2048` is rejected by a `nvm_dynamic_keymap.c` static assert.

## Clock source

By default this build runs entirely from the internal HSI (no crystal needed):

    HSI 8MHz / 2 = 4MHz --(PLLMUL × 12)--> 48MHz = SYSCLK = HCLK = PCLK
    USB 48MHz from HSI48 (independent of the PLL)

If the board is fitted with a 16MHz crystal, HSE can be enabled in `mcuconf.h`
(`STM32_HSE_ENABLED TRUE`, `STM32_PLLSRC_HSE`, `STM32_PREDIV_VALUE 2`, `STM32_PLLMUL_VALUE 6`
→ 16MHz/2 × 6 = 48MHz); see the comment block at the top of `mcuconf.h`. Do not enable HSE
unless the crystal is actually present — `stm32_clock_init()` waits for `HSERDY` forever
otherwise, and the board will not boot.

## Low power

`keymaps/*/rules.mk` select `KB_LPM_DRIVER = lpm_stm32f0_rtc_ec_v1`, which uses
`keyboards/keymagichorse/kb_common/lpm_chip_stm32f0.h`:

- STOP mode via `PWR_CR.LPDS` (F0 has no `MRLVDS/LPLVDS/FPDS`, and `PDDS` must stay clear so the
  MCU does not enter Standby, which would reset the part);
- RTC periodic wakeup (`RTCv2`, same LLD as the F4 boards) via LSI 40kHz;
- USB D+/D- (PA11/PA12, AF4 on F0) set to analog input while sleeping and restored on wakeup.

## Bootloader

Enter the bootloader in 2 ways:

- **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key which is Escape in this keyboard) and plug in the keyboard
- **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available.
