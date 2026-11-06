# n32h481_demo

Mechanical-switch + analogue Hall (磁轴) + WS2812 RGB firmware for the
N32H481REL7K development board (QMK / Vial).  Both keys are exposed on the
vial-stg exclusive **Analog / 键程** page.

## What it verifies

| Input | Pin | What you should see |
|---|---|---|
| Mechanical slide switch (U2) | `KEY1/KEY2` | types `m`; travel is exactly `0` or `M` |
| Hall sensor (U3 pin 6) | `HALLO / PA4` | types `h` on magnet approach; continuous travel `0..M` from ADC |
| Plain board LED | `LED+/LED-` | steady on = firmware alive |
| WS2812 strip (3 LEDs) | `PC3` | RGB Matrix animation, chosen in the Vial GUI |

The strip is controlled only by RGB Matrix / the VialRGB page. There are no
fixed red/green/blue status colours; the selected effect owns every LED.

## Analog / 键程 page (vial-stg)

`ANALOG_MODEL = linear` enables the `0xF0`-`0xF6` Vial Analog extension, so the
vial-stg GUI shows its Analog tab with 2 keys, `ANALOG_MAX_TRAVEL = 255`,
per-key actuation/release points, Rapid Trigger, live travel (`0xF3`),
calibration (`0xF4`) and explicit save (`0xF6`).

- **Hall key** (`ki=1`): `PA4 = ADC2_IN17`, 12-bit, full scale `0..4095` over
  `VDDA = 3.3 V` (`1 count = 0.806 mV`). Measured on this board:

  | state | ADC code | `0xF3` raw (`absv`) |
  |---|---|---|
  | no magnet / zero field | ~2150 | 0 |
  | switch fitted, resting | ~2010 | 140 |
  | full press | ~1530 | 620 |

  The zero-field point is **2150, not the nominal 2048 = VCC/2**. The reported
  raw domain is the deviation from that measured point,
  `absv = |adc - 2150|`, which is strictly increasing over the whole working
  range (`0 / 140 / 620`, stroke = 480 counts) and therefore fits the stock
  increasing-endpoint model. Using 2048 (as `tl96mgf072` does) gives
  `102 / 38 / 518`: the rest point lands on the other side of the reference,
  so absv first falls to 0 then rises — non-monotonic and hypersensitive to
  zero-point drift. That board gets away with it only because its standing
  offset is 200–350 counts, well clear of 2048; this PCB is not. The absolute
  value also handles the opposite magnet polarity for free.
- **Mechanical key** (`ki=0`): a slide switch has no intermediate travel, so
  its `analog_model_sw()` override returns only `0` or `255`; the raw column
  shows `0` / `620` (released / pressed) in the same `absv` domain.
- Calibration endpoints are `absv` counts, i.e. the same numbers `0xF3`
  reports. `top_reading` is re-measured on every boot (RAM only, no flash
  wear).
- **Factory endpoints are deliberately conservative.** `bottom_reading`
  starts at `600`, intentionally *below* the measured full press `~620`, so
  the real physical range exceeds the preset. Every scan widens it by one
  count towards the deepest deviation actually seen (capped at 1600 to reject
  ADC spikes); `analog_task` debounces the flash commit. A sustained press
  therefore auto-calibrates the endpoint in a few tens of scans. The GUI's
  bottom-out calibration (`0xF4` mode 4/5) widens it directly, and **Save**
  (`0xF6`) persists it. `0xF4` reset / `0xF1`-`0xF2` endpoints are clamped to
  `[1, 400]` for top and `[600, 2047]` for bottom. Do not "tune" the presets
  up to the measured values — that removes the headroom auto-calibration needs.
  A stored bottom that is inverted or clearly from an older raw-code domain
  (e.g. 1669) is reset to 600 at boot.

Do not power the board on with the magnet fully pressed against the sensor —
the boot calibration would then take the pressed deviation (~620) as the rest
point. Power up at the normal rest position. If the switch is not fitted at
boot (zero field, `absv ~ 0`), the firmware falls back to the nominal 140
standing offset, so fitting the switch later does not create a partial press;
reset afterwards to sample the real rest point.

### ADC implementation note

ADC2 is driven directly with the Nations SDK `ADC_SingleRead` sequence, not
with the ChibiOS ADC driver. The stock `platforms/chibios/drivers/analog.c`
path leaves `CTRL2.EXTRSEL` at zero and waits forever for the ENDC interrupt;
on this silicon the software-start bit then never begins a conversion and
`adc_read()` deadlocks `keyboard_init()`. This board therefore configures
`CTRL2 = ADC_EXT_TRIG_REG_CONV_SOFTWARE (0x002A0000)`, sets channel 17 to
239.5 sample cycles (`SAMPT3`, since channels >= 16 are not in `SAMPT1`),
performs the official single-ended calibration, and polls `STS.ENDC` with a
bounded timeout. A failed conversion degrades to zero travel instead of
hanging the keyboard.

Important: **PA4 is not an ADC1 pin**. Per the Nations channel map
(`n32h47x_48x_adc.h`), `ADC1_IN4 = PA1` while `ADC2_IN17 = PA4`. Sampling
`ADC1_IN4` therefore digitises the KEY2 line (held low by this firmware) and
produces a travel bar pinned to 0 with +-1 LSB of noise.

A constant `0` is the correct reading for "no field at all", so it is not by
itself a fault. To check the wiring, read the pre-`absv` code over SWD
(`demo_analog_dbg_hall_adc`): a wired, powered linear Hall sits at ~2150 with
no magnet, ~2010 with the switch resting, and barely moves when PA4's internal
pull-up/pull-down is toggled (the sensor output is low impedance). A floating
or unjumped `PA4` looks similar at rest but will not change when the magnet
moves, so "raw never moves" still means "check the `HALLO -> PA4` jumper and
the sensor's VCC/GND".

## Wiring (the U3 header is not wired back to the MCU on the schematic)

Jumper the U3 6-pin header to the MCU pins below (user decision, all
available on headers H1/H2/H5 — see `readme` of `n32h481_iotest` for
the header pin maps):

| U3 pin | Net | MCU pin | Direction |
|---|---|---|---|
| 1 | RGB | `PC3` | output, WS2812 data line (3 LEDs) |
| 2 | KEY1 | `PA0` | input pull-up |
| 3 | KEY2 | `PA1` | output low |
| 4 | LED+ | `PA2` | output high |
| 5 | LED- | `PA3` | output low |
| 6 | HALLO | `PA4` | analogue input, `ADC2_IN17` (12-bit) |

The 3-pin Hall device (`VCC/OUT/GND`) outputs an analogue voltage, so `PA4`
must be in `PAL_MODE_INPUT_ANALOG`; do not configure a pull-up or read it with
`gpio_read_pin()`. Its measured zero-field output on this board is ~2150 counts
(slightly above the nominal `VCC/2` = 2048, from sensor offset + actual VDDA),
falling to ~2010 with the switch fitted and ~1530 at full press. `0xF3` reports
`|adc - 2150|`, not the converter code. The firmware still boots and every
other page works if the Hall path is missing.

## Building

```sh
qmk compile -kb bvbhu/n32h481_demo -km default
qmk compile -kb bvbhu/n32h481_demo -km vial
```

Output: `bvbhu_n32h481_demo_<keymap>.bin`.

## Flashing

See [`docs/n32h481_jlink_flashing.md`](../../../docs/n32h481_jlink_flashing.md)
for J-Link wiring and commands (SWDIO=PA13, SWCLK=PA14, NRST, VTref=3V3).
