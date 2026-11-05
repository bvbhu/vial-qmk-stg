/* Copyright 2026 Talo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * n32h481_demo: two sensor keys and a WS2812 strip on the N32H481REL7K
 * development board.
 *
 * The 6-pin U3 header (RGB / KEY1 / KEY2 / LED+ / LED- / HALLO) is NOT
 * wired back to the MCU on the schematic, so it must be jumpered to the
 * MCU pins below (user decision):
 *
 *   KEY1  -> PA0  (U2 slide switch one end, input pull-up)
 *   KEY2  -> PA1  (U2 slide switch other end, driven LOW)
 *   LED+  -> PA2  (plain board LED anode, HIGH = on)
 *   LED-  -> PA3  (plain board LED cathode, LOW = on)
 *   HALLO -> PA4  (linear Hall OUT, ADC2_IN17, 12-bit analogue sampling)
 *   RGB   -> PC3  (WS2812 data line, 3 LEDs in series, GRB)
 *
 * All colour/animation is produced by RGB Matrix (VialRGB in the Vial
 * GUI).  The firmware never paints fixed test colours on the strip, and
 * no GPIO other than PC3 is used for lighting.  The plain LED on LED+/
 * LED- is just a steady power-on indicator.
 *
 * Both keys feed the Vial Analog core behind the vial-stg "Analog / 键程"
 * page: the Hall key has a real analogue travel curve, the mechanical slide
 * switch is clamped to the two endpoints 0 / ANALOG_MAX_TRAVEL.
 */
#pragma once

#define DEMO_KEY1_PIN A0
#define DEMO_KEY2_PIN A1
#define DEMO_LEDP_PIN A2
#define DEMO_LEDM_PIN A3
#define DEMO_HALL_PIN A4
/* PC3 is the WS2812 data line (WS2812_DI_PIN in drivers/ws2812_n32.h).
   It is deliberately not exposed as a GPIO lamp here: driving it as a
   plain output fights the LED strip protocol. */

/* Number of ADC samples used for the Hall rest-centre / noise-floor
   calibration at boot (keyboard_post_init_kb, after analog_init). */
#define DEMO_HALL_SAMPLES 32

/* ---- Vial Analog (vial-stg 键程页) ----------------------------------
 * PA4 is a linear Hall OUT sampled by ADC2_IN17, 12-bit over VDDA = 3.3 V
 * (1 count = 0.806 mV).  Measured on THIS board:
 *     no magnet / zero field        adc ~= 2150
 *     switch fitted, resting        adc ~= 2010
 *     full press / bottom-out       adc ~= 1530
 * The zero-field point is 2150, NOT the nominal 2048 = VCC/2: the sensor's
 * quiescent offset plus the actual VDDA put it 102 counts high.  The output
 * falls monotonically as the magnet approaches, so the raw domain reported
 * through 0xF3 is the deviation from the MEASURED zero-field point:
 *
 *     absv = |adc - 2150|
 *     no field 0 / rest 140 / full press 620   (stroke = 480 counts)
 *
 * Using 2150 keeps absv strictly increasing over the whole working range.
 * The stock 2048 (as used by tl96mgf072) would give no-field 102, rest 38,
 * full press 518: the rest point sits on the OTHER side of the reference and
 * absv first falls to 0 then rises, i.e. non-monotonic and hypersensitive to
 * zero-point drift.  That board only gets away with it because its standing
 * offset is 200-350 counts, well clear of 2048; this PCB is not.
 *
 * The absolute value also handles the opposite magnet polarity (output rising
 * above 2150) without any extra work.  analog_core's standard increasing
 * endpoint invariant applies, so no core patch is needed.
 *
 * M = 255 keeps the narrow wire format (12-byte 0xF1/0xF2 config, 3-byte
 * 0xF3 readings, 8-byte EEPROM records).
 */
#define ADC_RESOLUTION 12 /* QMK ADC driver default (this kb polls ADC2 directly) */
#define ANALOG_MAX_TRAVEL 255

/* Measured zero-field ADC code of this board's Hall sensor. */
#define DEMO_HALL_ZERO 2150U

/* Nominal resting deviation, used only when the boot calibration sees no
   field at all (switch not fitted yet / fitted after power-up).  Adopting 0
   there would turn the magnet's 140-count standing offset into a permanent
   partial press the moment the switch is installed. */
#define DEMO_HALL_REST_ABS 140U

/* Nominal physical stroke in absv counts: 620 (bottom) - 140 (rest) = 480. */
#define DEMO_HALL_STROKE 480U

#define DEMO_HALL_BOTTOM_ABS (DEMO_HALL_REST_ABS + DEMO_HALL_STROKE) /* 620 */

/* ---- Factory endpoints are deliberately CONSERVATIVE ------------------
 * The real physical range is EXPECTED to exceed the factory presets: that is
 * exactly what gives automatic calibration something to learn.  bottom starts
 * at 600 while a real full press reaches ~620, and the scan loop widens
 * bottom_reading towards the deepest deviation actually seen (one count per
 * scan, bounded by DEMO_HALL_ABS_CAP); analog_task debounces the flash write.
 * GUI bottom-out calibration (0xF4 mode 4/5) can also widen it directly.
 * Do NOT "tune" these presets up to the measured values -- that removes the
 * headroom the auto-calibration needs.
 */

/* A stored bottom_reading outside [top + 200, this] is treated as leftover
   from an earlier raw-code domain (or a bogus calibration) and reset to the
   conservative factory preset.  Old-firmware EEPROM values (e.g. 1669) land
   here.  1600 leaves ~2.6x headroom over the measured 620 full press. */
#define DEMO_HALL_BOTTOM_SANE_MAX 1600U

/* Runtime automatic bottom-out learning cap: ignore deviations above this so
   a floating input or ADC spike cannot blow the stroke up to the full domain.
   This is a noise guard, NOT a limit on how far calibration may extend. */
#define DEMO_HALL_ABS_CAP 1600U

/* These macros are BOTH the factory defaults and the clamp bounds (see
   analog_core.h §6): top <= TOPREADING_MAX, bottom >= BOTTOMREADING_MIN,
   and the core statically asserts BOTTOMREADING_MIN > TOPREADING_MAX. */
#define ANALOG_TOPREADING_MIN    1
#define ANALOG_TOPREADING_MAX    400
/* 600 is intentionally BELOW the measured ~620 full press: a real press
   exceeds the preset and drives automatic calibration.  Also the 0xF4
   RESET_CAL target and the clamp lower bound. */
#define ANALOG_BOTTOMREADING_MIN 600
#define ANALOG_BOTTOMREADING_MAX 2047
#define CALIBRATION_THRESHOLD    24

/* Trigger at ~24% of full travel so a moderate magnet approach still fires,
   with a 20-unit hysteresis dead band. */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 60
#define ANALOG_DEFAULT_RELEASE_THRESHOLD   40

/* Raw-domain sentinel for the binary mechanical switch, in the SAME absv
   domain: 0 = released, 620 = full press.  It is interpreted only by our
   analog_model_sw() override, never by the endpoint formula, so GUI
   calibration commands can never disable the mechanical key. */
#define DEMO_MECH_RAW_FULL      DEMO_HALL_BOTTOM_ABS
#define DEMO_MECH_RAW_THRESHOLD (DEMO_HALL_BOTTOM_ABS / 2U)

/* EEPROM: on-chip Flash dual-page mirror (drivers/eeprom/eeprom_custom.c),
 * EEPROM_SIZE partitions the dynamic-keymap area. */
#define EEPROM_SIZE 2048

/* USBHS on this part implements OUT endpoints 0 and 9..15 only; OUT EP1..EP8
 * have no control/interrupt registers at all.  Placing the raw HID OUT
 * endpoint outside that range made it silently dead (DOEPINT stayed 0, PKTCNT
 * never decremented, the host's WriteFile hung forever).
 *
 * Two transports are therefore available:
 *
 *   A) Interrupt OUT on EP9  - "stock QMK" path.  Keep RAW_OUT_EP_IN_DESCRIPTOR
 *      defined and RAW_OUT_EPNUM_FIXED at 9.  Requires N32_USBHS_ENDPOINTS >= 15
 *      so epc[9] exists and the DAEPINT bit25 dispatch is compiled in.
 *
 *   B) SET_REPORT on EP0     - the proven fallback.  With the endpoint left out
 *      of the descriptor the Windows HID class driver routes hid_write()
 *      through a control transfer, which works on this silicon (verified: the
 *      Vial GUI enumerated the keyboard and key remaps went through).  Stock
 *      Vial clients need no changes.
 *
 * Transport B is active: it is the one that is known to work end to end here.
 */
/* #define RAW_OUT_EPNUM_FIXED 9 */
/* #define RAW_OUT_EP_IN_DESCRIPTOR 1 */

#include_next <config.h>

