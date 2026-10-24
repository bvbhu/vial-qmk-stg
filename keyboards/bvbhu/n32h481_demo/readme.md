# n32h481_demo

Mechanical-switch + hall-sensor + 3-lamp verification firmware for the
N32H481REL7K development board (QMK).

## What it verifies

| Input | Pin | What you should see |
|---|---|---|
| Mechanical slide switch (U2) | `KEY1/KEY2` | typing `m`; lamp 0 (PC3) lights red |
| Hall sensor (U3 pin 6) | `HALLO` | typing `h` when a magnet approaches; lamp 1 (PA15) lights green |
| Plain board LED | `LED+/LED-` | steady on = firmware alive |
| Lamp 2 (PB3) | — | lights blue when either input is active |

## Wiring (the U3 header is not wired back to the MCU on the schematic)

Jumper the U3 6-pin header to the MCU pins below (user decision, all
available on headers H1/H2/H5 — see `readme` of `n32h481_iotest` for
the header pin maps):

| U3 pin | Net | MCU pin | Direction |
|---|---|---|---|
| 1 | RGB | `PC3` | output, active-low (lamp 0) |
| 2 | KEY1 | `PA0` | input pull-up |
| 3 | KEY2 | `PA1` | output low |
| 4 | LED+ | `PA2` | output high |
| 5 | LED- | `PA3` | output low |
| 6 | HALLO | `PA4` | input pull-up, polarity auto-detected at boot |

Solder the board's RGB1/RGB2 test pads to `PA15` (lamp 1) and `PB3`
(lamp 2), both on header H5.

The hall polarity (which level means "magnet present") is sampled
automatically for 32 reads at boot, so either open-collector or
push-pull sensor types work.

## Building

```sh
qmk compile -kb bvbhu/n32h481_demo -km default
```

Output: `bvbhu_n32h481_demo_default.bin`.

## Flashing

See [`docs/n32h481_jlink_flashing.md`](../../../docs/n32h481_jlink_flashing.md)
for J-Link wiring and commands (SWDIO=PA13, SWCLK=PA14, NRST, VTref=3V3).
