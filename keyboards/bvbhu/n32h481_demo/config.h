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
 * n32h481_demo: verify a mechanical switch, a hall sensor and three
 * lamps on the N32H481REL7K development board.
 *
 * The 6-pin U3 header (RGB / KEY1 / KEY2 / LED+ / LED- / HALLO) is NOT
 * wired back to the MCU on the schematic, so it must be jumpered to the
 * MCU pins below (user decision):
 *
 *   KEY1  -> PA0  (U2 slide switch one end, input pull-up)
 *   KEY2  -> PA1  (U2 slide switch other end, driven LOW)
 *   LED+  -> PA2  (plain board LED anode, HIGH = on)
 *   LED-  -> PA3  (plain board LED cathode, LOW = on)
 *   HALLO -> PA4  (hall sensor OUT, input pull-up, polarity auto-detected)
 *   RGB   -> PC3  (lamp 0, active LOW, common-anode to 5V)
 *   RGB1  -> PA15 (lamp 1, active LOW, header H5)
 *   RGB2  -> PB3  (lamp 2, active LOW, header H5)
 *
 * Lamp 0 (PC3) follows the mechanical switch, lamp 1 (PA15) follows the
 * hall sensor, lamp 2 (PB3) lights when either input is active. The
 * plain LED on LED+/LED- is a steady power-on indicator.
 */
#pragma once

#define DEMO_KEY1_PIN A0
#define DEMO_KEY2_PIN A1
#define DEMO_LEDP_PIN A2
#define DEMO_LEDM_PIN A3
#define DEMO_HALL_PIN A4
#define DEMO_LAMP0_PIN C3
#define DEMO_LAMP1_PIN A15
#define DEMO_LAMP2_PIN B3

/* Number of samples used to auto-detect the hall idle level at boot. */
#define DEMO_HALL_SAMPLES 32

/* EEPROM: on-chip Flash dual-page mirror (drivers/eeprom/eeprom_custom.c),
 * EEPROM_SIZE partitions the dynamic-keymap area. */
#define EEPROM_SIZE 2048

#include_next <config.h>

