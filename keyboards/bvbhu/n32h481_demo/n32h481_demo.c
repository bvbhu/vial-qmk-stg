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
 * n32h481_demo — mechanical switch + analogue Hall (磁轴) + WS2812 strip.
 *
 * Matrix (CUSTOM_MATRIX = lite), 1 row x 2 columns:
 *   [0,0] = KEY1/KEY2 slide switch  -> binary key travel (0 / ANALOG_MAX_TRAVEL)
 *   [0,1] = HALLO linear Hall OUT   -> real analogue key travel (ADC2_IN17, PA4)
 *
 * The Hall device on U3 pin 6 is a 3-pin VCC/OUT/GND sensor.  Its OUT is an
 * analogue voltage, so it is sampled with the on-chip 12-bit ADC (ADC2_IN17),
 * NOT with gpio_read_pin().  Measured on this board: zero field ~2150, switch
 * resting ~2010, full press ~1530 (1 count = VDDA/4096 = 0.806 mV).  The
 * zero-field point is 2150 rather than the nominal 2048 = VCC/2, so the raw
 * domain reported to the GUI is the deviation from that MEASURED point:
 *
 *     absv = |adc - 2150|      0 / 140 / 620  (no field / rest / full press)
 *
 * which is strictly increasing over the whole working range and therefore
 * fits analog_core's standard increasing-endpoint model.  Boot calibration
 * measures the standing offset and uses it as top_reading.  bottom_reading
 * starts at the CONSERVATIVE factory preset (600, below the real ~620 full
 * press) and is automatically widened at runtime towards the deepest press
 * actually seen, so the physical range always has room to exceed the preset.
 * See config.h for why 2048 does not work on this PCB.
 * ADC2 (channel 17, the one PA4 is bonded to) is driven with the Nations SDK
 * single-read sequence (software-trigger EXTRSEL=0x2A, 239.5-cycle sampling,
 * official calibration) using bounded polling instead of the ChibiOS ADC
 * driver, whose ENDC ISR never fires here.
 *
 * Both keys are driven through the same push state machine
 * (analog_model_sw -> analog_step_key), so the vial-stg "Analog / 键程" page
 * can show live travel, per-key actuation/release points, Rapid Trigger and
 * calibration for either key.  The mechanical key is deliberately clamped to
 * the two endpoints 0 / M because a slide switch has no intermediate travel.
 *
 * Lighting (RGB_MATRIX_DRIVER = custom):
 *   WS2812 strip on PC3, 3 LEDs, GRB, 800 kHz, DWT-timed bit-banging.
 *   The strip is owned exclusively by RGB Matrix and the VialRGB page.
 *
 * The plain board LED on LED+/LED- is a hard-wired power indicator and is
 * not part of the lighting system.
 */

#include "quantum.h"

#include "gpio.h"
#include "hal.h"                  /* ADC2 / RCC / PAL_MODE_INPUT_ANALOG */
#include "matrix.h"               /* MATRIX_ROWS/COLS must precede analog_core.h */
#include "analog/analog_core.h"   /* Vial Analog core (0xF0-0xF6 backing store) */
#include "rgb_matrix.h"
#include "drivers/ws2812_n32.h"
#include "uart_dbg.h"

#define DEMO_MECH_KI 0u
#define DEMO_HALL_KI 1u

/* DEMO_HALL_ZERO (config.h) is this board's MEASURED zero-field ADC code,
   2150.  The stock 2048 = VCC/2 is a nominal value that this sensor/VDDA
   combination misses by 102 counts; using it makes absv non-monotonic. */

static uint16_t hall_rest_center = DEMO_HALL_ZERO; /* rest ADC code, sampled at boot */
static uint16_t hall_rest_abs;                     /* |rest - ZERO|, the standing offset */
static uint16_t last_raw[ANALOG_NUM_KEYS];         /* absv domain value reported by 0xF3 */

/* SWD-readable diagnostics: let us prove the ADC path is live without a GUI. */
volatile uint32_t demo_analog_dbg_scan_cnt;
volatile uint16_t demo_analog_dbg_rest_center;
volatile uint16_t demo_analog_dbg_rest_abs;        /* |rest - 2150|, the standing field */
volatile uint16_t demo_analog_dbg_top;
volatile uint16_t demo_analog_dbg_hall_adc;
volatile uint16_t demo_analog_dbg_hall_raw;        /* == 0xF3 raw for the Hall key */
volatile uint16_t demo_analog_dbg_hall_sw;
volatile uint16_t demo_analog_dbg_mech_sw;
volatile uint16_t demo_analog_dbg_hall_bottom;
volatile uint8_t  demo_analog_dbg_adc_cfg;         /* 0xFF = no working config */
volatile uint32_t demo_analog_dbg_adc_ok;
volatile uint32_t demo_analog_dbg_adc_fail;

/* Boot phase marker (SWD-readable): 0=before matrix_init, 1..6=matrix_init,
   7=post_init entered, 0x10..0x2F=rest-calibration sample #, 8=post_init done,
   0x50=main loop reached.  Lets us see exactly where init blocks without UART. */
volatile uint32_t demo_boot_phase;

/* ------------------------------------------------------------------ */
/* Direct, interrupt-free ADC2 polling  (PA4 = ADC2_IN17)             */
/*                                                                    */
/* PA4 is NOT an ADC1 pin.  The Nations channel map                   */
/* (n32h47x_48x_adc.h) is explicit:                                   */
/*     ADC1_IN2=PA3  ADC1_IN3=PA0  ADC1_IN4=PA1  ADC1_IN5=PA2         */
/*     ADC2_IN17=PA4                                                  */
/* An earlier build programmed ADC1 channel 4 while the Hall OUT was   */
/* on PA4, so it was really digitising PA1 - the KEY2 line that this   */
/* firmware drives LOW.  That is why the travel bar sat at 0 with      */
/* +-1 LSB of noise: not a timing bug, a wrong-channel bug.            */
/*                                                                    */
/* Channel 17 is a "high" channel (>=16): its sampling time lives in  */
/* SAMPT3 (4 bits per channel, ch17 -> bits[7:4]) and RSEQ1 takes the  */
/* channel number directly.                                           */
/*                                                                    */
/* The ChibiOS N32 ADCv1 driver still cannot be used: it leaves       */
/* CTRL2.EXTRSEL at 0 and waits forever for the ENDC ISR, so          */
/* adc_read() deadlocked keyboard_init().  The Nations SDK             */
/* ADC_SingleRead example shows software triggering needs             */
/* CTRL2 |= ADC_EXT_TRIG_REG_CONV_SOFTWARE (0x002A0000).  We own       */
/* ADC2 here: SDK init/calibration sequence, bounded polling, no ISR.  */
/* A failed probe degrades to "no travel" instead of bricking the KB.  */
/* ------------------------------------------------------------------ */

#define DEMO_ADC            ADC2
#define DEMO_ADC_CH         17U           /* PA4 = ADC2_IN17 */
#define DEMO_ADC_TIMEOUT    20000U        /* bounded: never hang the scan loop */
#define DEMO_ADC_SW_TRIGGER 0x002A0000U   /* SDK: ADC_EXT_TRIG_REG_CONV_SOFTWARE */
#define DEMO_ADC_SAMPT3     0x000000C0U   /* SAMP17 = 239.5 cycles (SDK example) */

static uint8_t adc_cfg_index = 0xFFU;

static void demo_adc_apply(void) {
    /* SDK RCC_Configuration: ADC2 clock, ADCHCLK = HCLK /16. */
    RCC->AHB1PCLKEN |= RCC_AHB1PCLKEN_ADC2EN;
    RCC->CFG2 &= ~(RCC_CFG2_ADCHPRES | RCC_CFG2_ADC1MPRES | RCC_CFG2_ADC1MSEL);
    RCC->CFG2 |= RCC_CFG2_ADCHPRES_DIV16 | RCC_CFG2_ADC1MPRES_DIV8;

    /* SDK ADC_Init: power support, 12-bit, software trigger, one-channel
       regular sequence. */
    DEMO_ADC->PCTRL  = ADC_PCTRL_EXTPEN | ADC_PCTRL_VREFLDOEN;
    DEMO_ADC->INTEN  = 0U;
    DEMO_ADC->CTRL1  = 0U;                     /* independent, no scan */
    DEMO_ADC->CTRL2  = DEMO_ADC_SW_TRIGGER;    /* <-- the missing piece */
    DEMO_ADC->CTRL3  = ADC_CTRL3_RES;          /* RES[1:0] = 11 -> 12-bit */
    DEMO_ADC->SAMPT1 = 0U;
    DEMO_ADC->SAMPT2 = 0U;
    DEMO_ADC->SAMPT3 = DEMO_ADC_SAMPT3;        /* only ch16..18 live here */
    DEMO_ADC->RSEQ1  = DEMO_ADC_CH;
    DEMO_ADC->RSEQ2  = 0U;
    DEMO_ADC->RSEQ3  = 0U;

    /* SDK ADC_Enable + wait RDY. */
    DEMO_ADC->CTRL2 |= ADC_CTRL2_ON;
    for (volatile uint32_t i = 0; i < 200000U; i++) {
        if (DEMO_ADC->STS & ADC_STS_RDY) break;
    }

    /* SDK ADC_CalibrationOperation / ADC_GetCalibrationStatus: done when the
       CAL bit clears OR the single-ended calibration factor becomes valid. */
    DEMO_ADC->CTRL2 |= ADC_CTRL2_ENCAL;
    for (volatile uint32_t i = 0; i < 200000U; i++) {
        if ((DEMO_ADC->CALFACT & ADC_CALFACT_CALFACTS) != 0U) break;
        if ((DEMO_ADC->CTRL2 & ADC_CTRL2_ENCAL) == 0U) break;
    }
    DEMO_ADC->STS = ADC_STS_ENDC | ADC_STS_STR;
}

static bool demo_adc_convert(uint16_t *out) {
    /* Re-assert our channel every time: no other code may own ADC2's regular
       sequence, and a stray RSEQ1 write must not silently become "Hall data". */
    DEMO_ADC->RSEQ1 = DEMO_ADC_CH;
    /* SDK ADC_EnableSoftwareStartConv uses |= : never clobber EXTRSEL. */
    DEMO_ADC->STS = ADC_STS_ENDC | ADC_STS_STR;
    DEMO_ADC->CTRL2 &= ~ADC_CTRL2_SWSTRRCH;
    DEMO_ADC->CTRL2 |= ADC_CTRL2_SWSTRRCH;
    for (uint32_t i = 0; i < DEMO_ADC_TIMEOUT; i++) {
        if (DEMO_ADC->STS & ADC_STS_ENDC) {
            *out = (uint16_t)(DEMO_ADC->DAT & 0xFFFU);
            DEMO_ADC->STS = ADC_STS_ENDC | ADC_STS_STR;
            return true;
        }
    }
    return false;
}

static void demo_adc_settle(void) {
    /* Pure busy loop: wait_us() uses a GPT timer that is not guaranteed to be
       up during early keyboard_init(). */
    for (volatile uint32_t d = 0; d < 40000U; d++) {
    }
}

static void demo_adc_probe(void) {
    uint16_t v;
    demo_adc_apply();
    demo_adc_settle();
    if (demo_adc_convert(&v)) {
        adc_cfg_index           = 0U;
        demo_analog_dbg_adc_cfg = 0U;
        demo_analog_dbg_adc_ok++;
        return;
    }
    adc_cfg_index           = 0xFFU;
    demo_analog_dbg_adc_cfg = 0xFFU;
    demo_analog_dbg_adc_fail++;
}

static uint16_t demo_hall_read_adc(void) {
    uint16_t v;
    if (adc_cfg_index == 0xFFU) {
        demo_analog_dbg_adc_fail++;
        /* Retry occasionally, but never block the matrix scan. */
        if ((demo_analog_dbg_scan_cnt & 0x3FFU) == 0U) demo_adc_probe();
        return DEMO_HALL_ZERO; /* no reading -> zero field -> absv 0 -> travel 0 */
    }
    if (!demo_adc_convert(&v)) {
        demo_analog_dbg_adc_fail++;
        return DEMO_HALL_ZERO;
    }
    demo_analog_dbg_adc_ok++;
    return v;
}

/* ------------------------------------------------------------------ */
/* Boot calibration (runs from keyboard_post_init_kb, AFTER analog_init) */
/* ------------------------------------------------------------------ */

static void demo_analog_calibrate_rest(void) {
    uint16_t samples[DEMO_HALL_SAMPLES];
    uint32_t sum = 0;

    for (uint8_t i = 0; i < DEMO_HALL_SAMPLES; i++) {
        demo_boot_phase = 0x10U + i;
        samples[i] = demo_hall_read_adc();
        sum += samples[i];
        demo_adc_settle();
    }

    uint16_t centre = (uint16_t)(sum / DEMO_HALL_SAMPLES);
    hall_rest_center            = centre;
    demo_analog_dbg_rest_center = centre;

    /* Standing field offset in the raw absv domain: |rest - ZERO|.  This is
       where travel 0 lives, so it becomes top_reading (plus a noise margin). */
    uint16_t rest_abs = (centre > DEMO_HALL_ZERO) ? (uint16_t)(centre - DEMO_HALL_ZERO)
                                                  : (uint16_t)(DEMO_HALL_ZERO - centre);
    hall_rest_abs            = rest_abs;
    demo_analog_dbg_rest_abs = rest_abs;

    /* Noise floor = mean absolute deviation of the samples about the mean. */
    uint32_t dev = 0;
    for (uint8_t i = 0; i < DEMO_HALL_SAMPLES; i++) {
        dev += (samples[i] > centre) ? (uint32_t)(samples[i] - centre)
                                     : (uint32_t)(centre - samples[i]);
    }
    uint16_t noise = (uint16_t)(dev / DEMO_HALL_SAMPLES);

    /* If the sensor saw no field at all at boot, the switch was not fitted
       yet; adopt the nominal standing offset instead of 0, otherwise fitting
       the switch later would show a permanent partial press.  top_reading is
       RAM only (re-derived every boot), so this costs no flash write. */
    uint16_t top;
    if (rest_abs < (uint16_t)CALIBRATION_THRESHOLD) {
        top = (uint16_t)DEMO_HALL_REST_ABS;
    } else {
        uint32_t top32 = (uint32_t)rest_abs + (uint32_t)noise * 3U + 8U;
        top = (top32 > (uint32_t)ANALOG_TOPREADING_MAX)
                  ? (uint16_t)ANALOG_TOPREADING_MAX : (uint16_t)top32;
    }
    if (top < (uint16_t)ANALOG_TOPREADING_MIN) top = (uint16_t)ANALOG_TOPREADING_MIN;
    if (top > (uint16_t)ANALOG_TOPREADING_MAX) top = (uint16_t)ANALOG_TOPREADING_MAX;
    analog_set_top_reading(DEMO_HALL_KI, top);

    /* bottom_reading is persisted.  Only reset it when it is clearly unusable
       (inverted, or leftover from an earlier raw-code domain such as 1669),
       and always reset to the CONSERVATIVE factory preset -- never to the
       measured full press: runtime auto-calibration needs the preset to sit
       BELOW the real range so it has something to learn. */
    uint32_t cur_bottom = (uint32_t)ANALOG_BOTTOM_READING(0, DEMO_HALL_KI);
    if (cur_bottom < (uint32_t)top + 200U ||
        cur_bottom > (uint32_t)DEMO_HALL_BOTTOM_SANE_MAX) {
        analog_set_bottom_reading(DEMO_HALL_KI, (uint16_t)ANALOG_BOTTOMREADING_MIN);
    }
    demo_analog_dbg_top = ANALOG_TOP_READING(0, DEMO_HALL_KI);

    dbg_puts("analog: hall adc=");
    dbg_hex32(centre);
    dbg_puts(" absv=");
    dbg_hex32(rest_abs);
    dbg_puts(" noise=");
    dbg_hex32(noise);
    dbg_puts(" top=");
    dbg_hex32(top);
    dbg_puts(" bottom=");
    dbg_hex32(ANALOG_BOTTOM_READING(0, DEMO_HALL_KI));
    dbg_puts("\r\n");
    demo_boot_phase = 0x30U;
}

void keyboard_post_init_kb(void) {
    /* analog_init() has already filled defaults / loaded EEPROM by now, so
       writing top_reading here is accepted instead of being overwritten. */
    demo_boot_phase = 7U;
    demo_analog_calibrate_rest();
    demo_boot_phase = 8U;
    keyboard_post_init_user();
}

/* ------------------------------------------------------------------ */
/* Matrix                                                             */
/* ------------------------------------------------------------------ */

void matrix_init_custom(void) {
    demo_boot_phase = 1U;
    dbg_uart_init();
    dbg_puts("\r\n== N32H481 DEMO BOOT ==\r\n");

    /* Slide switch: drive KEY2 low, read KEY1 with pull-up. */
    gpio_set_pin_output(DEMO_KEY2_PIN);
    gpio_write_pin_low(DEMO_KEY2_PIN);
    gpio_set_pin_input_high(DEMO_KEY1_PIN);

    /* PA4 = ADC2_IN17.  PAL_MODE_INPUT_ANALOG is mandatory: reading an
       analogue Hall OUT as a digital GPIO would throw away the whole travel
       curve, which is what the old firmware did. */
    demo_boot_phase = 2U;
    palSetLineMode(DEMO_HALL_PIN, PAL_MODE_INPUT_ANALOG);
    demo_boot_phase = 3U;
    hall_rest_center = DEMO_HALL_ZERO;
    demo_boot_phase = 4U;
    demo_adc_probe(); /* ADC2/ch17 init + first conversion; always returns */
    demo_boot_phase = 5U;

    /* Plain board LED (LED+ -> diode -> 1k -> LED-): steady on = powered.
       PC3 is the WS2812 data line and must not be toggled here. */
    gpio_set_pin_output(DEMO_LEDP_PIN);
    gpio_write_pin_high(DEMO_LEDP_PIN);
    gpio_set_pin_output(DEMO_LEDM_PIN);
    gpio_write_pin_low(DEMO_LEDM_PIN);
    demo_boot_phase = 6U;
}

/* Convert one ADC reading into the raw domain reported through 0xF3:
 *     absv = |adc - DEMO_HALL_ZERO|
 * i.e. the field-induced deviation in ADC counts.  0 = no field at all, a
 * fitted switch resting on the sensor reads ~140, full press ~620.  The
 * absolute value also covers the opposite magnet polarity (output rising
 * above 2150) without any extra work. */
static uint16_t demo_hall_read_absv(void) {
    uint16_t v = demo_hall_read_adc();
    demo_analog_dbg_hall_adc = v;

    uint32_t d = (v > DEMO_HALL_ZERO) ? (uint32_t)(v - DEMO_HALL_ZERO)
                                      : (uint32_t)(DEMO_HALL_ZERO - v);
    if (d > 0xFFFFU) d = 0xFFFFU;
    demo_analog_dbg_hall_raw = (uint16_t)d;
    return (uint16_t)d;
}

/* GUI 0xF3 hook: most recent absv-domain reading for each key. */
int16_t analog_backend_get_raw_adc(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) return -1;
    return (int16_t)last_raw[ki];
}

/* Strong override of the weak hook in analog_model_linear.c.  Keeping both
   keys' mappings here makes the "mechanical = endpoints only" contract
   explicit and immune to GUI calibration endpoints.  The Hall uses the
   standard INCREASING mapping (top < bottom) because absv grows with travel. */
analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv) {
    if (ki == DEMO_MECH_KI) {
        return (absv >= DEMO_MECH_RAW_THRESHOLD) ? (analog_travel_t)ANALOG_MAX_TRAVEL
                                                 : (analog_travel_t)0;
    }
    if (ki >= ANALOG_NUM_KEYS) return 0;

    const analog_key_t *k = &g_analog_key[ki];
    uint32_t top    = k->top_reading;    /* rest deviation, travel 0 */
    uint32_t bottom = k->bottom_reading; /* bottom-out deviation, travel M */

    /* 0xF4 mode 2 (RESET_CAL) parks top at ANALOG_TOPREADING_MAX (400), which
       would make the first ~400 counts feel dead.  Substitute the standing
       offset measured at boot whenever top is implausibly large. */
    if (top > (uint32_t)DEMO_HALL_REST_ABS * 2U) {
        top = hall_rest_abs ? (uint32_t)hall_rest_abs : (uint32_t)DEMO_HALL_REST_ABS;
    }

    /* Only reject a truly inverted interval.  A wider-than-nominal interval
       is EXPECTED: the factory endpoints are conservative and the automatic
       bottom-out calibration deliberately extends bottom towards the real
       physical range.  Clamping it back would undo that calibration. */
    if (bottom <= top) return 0;

    if ((uint32_t)absv <= top) return 0;
    if ((uint32_t)absv >= bottom) return (analog_travel_t)ANALOG_MAX_TRAVEL;

    uint32_t sw = ((uint32_t)absv - top) * ANALOG_MAX_TRAVEL / (bottom - top);
    return (sw > (uint32_t)ANALOG_MAX_TRAVEL) ? (analog_travel_t)ANALOG_MAX_TRAVEL
                                              : (analog_travel_t)sw;
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    matrix_row_t row = 0;
    demo_boot_phase = 0x50U;
    demo_analog_dbg_scan_cnt++;

    for (uint16_t ki = 0; ki < ANALOG_NUM_KEYS; ki++) {
        uint16_t raw;
        if (ki == DEMO_MECH_KI) {
            /* Binary switch: 0 = released, full-scale absv = pressed. */
            raw = (gpio_read_pin(DEMO_KEY1_PIN) == 0) ? (uint16_t)DEMO_MECH_RAW_FULL : 0U;
        } else {
            raw = demo_hall_read_absv();
        }
        last_raw[ki] = raw;

        analog_travel_t sw = analog_model_sw(ki, raw);
        if (ki == DEMO_MECH_KI) {
            demo_analog_dbg_mech_sw = sw;
        } else {
            demo_analog_dbg_hall_sw     = sw;
            demo_analog_dbg_hall_bottom = ANALOG_BOTTOM_READING(0, ki);
        }

        if (analog_get_bottom_out_mode()) {
            /* 0xF4 mode 4: suppress every key, and for the Hall key adopt the
               deepest deviation seen so far -- in the increasing absv domain
               that means pushing bottom_reading UP.  The cap rejects a
               floating input or an ADC spike. */
            if (ki == DEMO_HALL_KI && raw <= (uint16_t)DEMO_HALL_ABS_CAP &&
                (int)raw > (int)ANALOG_BOTTOM_READING(0, ki)) {
                analog_set_bottom_reading(ki, raw);
            }
            analog_force_release(ki);
            continue;
        }

        /* Automatic bottom-out learning.  The factory bottom is deliberately
           conservative (600) and a real full press (~620) exceeds it.  Widen
           the endpoint by ONE count per scan towards the deepest deviation
           seen: a single ADC spike can move it by at most 1 LSB, while a
           sustained press converges in a few tens of scans.  analog_task
           debounces the resulting flash write; 0xF6 / GUI save persists it
           explicitly.  The cap rejects a floating input. */
        if (ki == DEMO_HALL_KI && raw <= (uint16_t)DEMO_HALL_ABS_CAP) {
            uint16_t cur_bottom = (uint16_t)ANALOG_BOTTOM_READING(0, ki);
            if (raw > cur_bottom) {
                analog_set_bottom_reading(ki, (uint16_t)(cur_bottom + 1U));
            }
        }

        /* No runtime top_reading tracking on purpose: a smaller absv cannot
           be told apart from "switch removed" or rest drift without a press
           gate, and silently pulling top down would create a false partial
           press.  DEMO_HALL_ZERO is a fixed measured constant, so the raw
           domain does not drift; top is re-sampled at every boot and the GUI
           "Initial calibration" button (0xF4) handles mid-session re-zeroing. */

        (void)analog_step_key(ki, sw);
        if (analog_get_pressed(ki)) {
            row |= (matrix_row_t)(1u << ki);
        }
    }

    bool changed      = (current_matrix[0] != row);
    current_matrix[0] = row;
    return changed;
}

/* ------------------------------------------------------------------ */
/* rgb_matrix driver shim: WS2812 strip on PC3 (3 LEDs, GRB).          */
/* ------------------------------------------------------------------ */

static void demo_rgb_init(void) {
    ws2812_init();
}

static void demo_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    ws2812_set_color(index, red, green, blue);
}

static void demo_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    ws2812_set_color_all(red, green, blue);
}

static void demo_flush(void) {
    ws2812_flush();
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = demo_rgb_init,
    .flush         = demo_flush,
    .set_color     = demo_set_color,
    .set_color_all = demo_set_color_all,
};
