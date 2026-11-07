/* Copyright 2026 Talo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tl12wlh481 keyboard-level code.
 *
 * This is bvbhu/tl12blef103's wireless/framework logic ported to N32H481RE:
 *   - the STM32F1-only ADC VREFINT shim, the F1 STOP low-power hooks and the
 *     sleep/wake RGB repaint are gone (no N32 LPM backend exists yet);
 *   - RGB Matrix uses the N32 bit-banged WS2812 driver instead of QMK's
 *     stock ws2812 driver (the N32 port has no PWM/GPT LLD);
 *   - the BHQ bridge UART is the newly added N32 USARTv1 SERIAL LLD (SD3,
 *     USART3 on PD8/PD9, AF17/AF18 -- USART3 is fully pin-mappable).
 *
 * 0x11 SET_CONFIG protocol: wch-ble-bridge/uart_protocol.md section 3.2.
 */
#include "tl12wlh481.h"
#include "timer.h"

#if defined(BLUETOOTH_BHQ)
#    include "km_printf.h" /* km_printf is a no-op unless KB_DEBUG is set */
#endif

/* ==========================================================================
 * Board / keyboard hooks
 * ======================================================================== */
void board_init(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_common_init(); /* USB power sense + (guarded) battery init */
#endif
    /* No lpm_init(): kb_common has no N32 LPM chip backend.  The keyboard
     * therefore never enters STOP mode; USB/wireless stay live. */
}

void keyboard_post_init_kb(void) {
#if defined(BLUETOOTH_BHQ)
    tl12wl_config_task(); /* record the start time; first send is delayed */
#endif
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_wireless_task();  /* multi-host switching, bridge state machine */
    tl12wl_config_task(); /* delayed + retried 0x11 SET_CONFIG */
#endif
    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#if defined(BLUETOOTH_BHQ)
    if (!process_record_bhq(keycode, record)) {
        return false;
    }
#endif
    return process_record_user(keycode, record);
}

/* ==========================================================================
 * RGB Matrix driver shim
 *
 * keyboard.json declares "rgb_matrix": { "driver": "custom", "led_count": 12 }.
 * QMK then expects the keyboard to supply rgb_matrix_driver; we forward it to
 * platforms/chibios/drivers/ws2812_n32.c (DWT-timed bit-bang on E7).
 * ======================================================================== */
static void tl12wl_rgb_init(void) {
    ws2812_init();
}

static void tl12wl_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    ws2812_set_color(index, red, green, blue);
}

static void tl12wl_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    ws2812_set_color_all(red, green, blue);
}

static void tl12wl_flush(void) {
    ws2812_flush();
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = tl12wl_rgb_init,
    .flush         = tl12wl_flush,
    .set_color     = tl12wl_set_color,
    .set_color_all = tl12wl_set_color_all,
};

/* ==========================================================================
 * 0x11 SET_CONFIG sender
 *
 * Same payload as tl12blef103: connection parameters, TX power, sleep
 * thresholds, four names and the VID/PID.  Sent 500 ms after boot (the WCH
 * bridge powers up more slowly than the MCU) and retried a few times.
 * ======================================================================== */
#if defined(BLUETOOTH_BHQ)

#    ifndef TL12WL_CFG_DELAY_MS
#        define TL12WL_CFG_DELAY_MS 500
#    endif
#    ifndef TL12WL_CFG_RETRY_COUNT
#        define TL12WL_CFG_RETRY_COUNT 3
#    endif
#    ifndef TL12WL_CFG_RETRY_INTERVAL_MS
#        define TL12WL_CFG_RETRY_INTERVAL_MS 400
#    endif

#    ifndef BLE_NAME
#        define BLE_NAME PRODUCT
#    endif
#    ifndef BLE_SWIFT_PAIR_NAME
#        define BLE_SWIFT_PAIR_NAME PRODUCT
#    endif
#    ifndef USB_DOG_NAME
#        define USB_DOG_NAME PRODUCT "_DOG"
#    endif
#    ifndef USB_DOG_VENDOR_NAME
#        define USB_DOG_VENDOR_NAME MANUFACTURER
#    endif

#    ifndef TL12WL_LE_INTERVAL_MIN
#        define TL12WL_LE_INTERVAL_MIN 6 /* unit: 1.25 ms */
#    endif
#    ifndef TL12WL_LE_INTERVAL_MAX
#        define TL12WL_LE_INTERVAL_MAX 35
#    endif
#    ifndef TL12WL_LE_TIMEOUT
#        define TL12WL_LE_TIMEOUT 500
#    endif
#    ifndef TL12WL_TX_POWER
#        define TL12WL_TX_POWER 0x3D
#    endif
#    ifndef TL12WL_SLEEP_1_S
#        define TL12WL_SLEEP_1_S 10
#    endif
#    ifndef TL12WL_SLEEP_2_S
#        define TL12WL_SLEEP_2_S 300
#    endif

/* bhqDevConfigInfo_t name fields are uint8_t[] WITHOUT a NUL terminator;
 * the length is carried in a separate field.  Copy explicitly (string
 * literals only work as initialisers, not as assignments). */
static uint8_t tl12wl_clamp_len(uint16_t len, uint16_t max, const char *what) {
    if (len > max) {
        km_printf("[cfg] %s too long: %d > %d, truncated\n", what, (int)len, (int)max);
        return (uint8_t)max;
    }
    return (uint8_t)len;
}

static void tl12wl_build_and_send_config(void) {
    bhqDevConfigInfo_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    cfg.le_connection_interval_min     = TL12WL_LE_INTERVAL_MIN;
    cfg.le_connection_interval_max     = TL12WL_LE_INTERVAL_MAX;
    cfg.le_connection_interval_timeout = TL12WL_LE_TIMEOUT;
    cfg.tx_poweer                      = TL12WL_TX_POWER;

    /* Battery-related fields are deprecated upstream: all zero. */
    cfg.mk_is_read_battery_voltage = 0;
    cfg.mk_adc_pga                 = 0;
    cfg.mk_rvd_r1                  = 0;
    cfg.mk_rvd_r2                  = 0;

    cfg.sleep_1_s = TL12WL_SLEEP_1_S;
    cfg.sleep_2_s = TL12WL_SLEEP_2_S;

    cfg.bleNameStrLength = tl12wl_clamp_len(strlen(BLE_NAME), BLE_ADVERT_NAME_MAX, "BLE_NAME");
    memcpy(cfg.bleNameStr, BLE_NAME, cfg.bleNameStrLength);

    cfg.bleSwiftPairNameStrLength = tl12wl_clamp_len(strlen(BLE_SWIFT_PAIR_NAME), BLE_SWIFT_PAIR_NAME_MAX, "BLE_SWIFT_PAIR_NAME");
    memcpy(cfg.bleSwiftPairNameStr, BLE_SWIFT_PAIR_NAME, cfg.bleSwiftPairNameStrLength);

    cfg.usbNameStrLength = tl12wl_clamp_len(strlen(USB_DOG_NAME), USB_NAME_MAX, "USB_DOG_NAME");
    memcpy(cfg.usbNameStr, USB_DOG_NAME, cfg.usbNameStrLength);

    cfg.usbVendorNameStrLength = tl12wl_clamp_len(strlen(USB_DOG_VENDOR_NAME), USB_VENDOR_NAME_MAX, "USB_DOG_VENDOR_NAME");
    memcpy(cfg.usbVendorNameStr, USB_DOG_VENDOR_NAME, cfg.usbVendorNameStrLength);

    cfg.vendor_id_source = 1; /* 1 = USB-IF */
    cfg.verndor_id       = VENDOR_ID;
    cfg.product_id       = PRODUCT_ID;

    bhq_ConfigParam(cfg);
}

void tl12wl_config_task(void) {
    static uint8_t  sent      = 0;
    static uint32_t start_at  = 0;
    static uint32_t last_sent = 0;

    if (sent >= TL12WL_CFG_RETRY_COUNT) {
        return;
    }

    uint32_t now = timer_read32();

    if (sent == 0) {
        if (start_at == 0) {
            start_at = now ? now : 1; /* avoid 0 being read as "unset" */
            return;
        }
        if (timer_elapsed32(start_at) < TL12WL_CFG_DELAY_MS) {
            return;
        }
    } else if (timer_elapsed32(last_sent) < TL12WL_CFG_RETRY_INTERVAL_MS) {
        return;
    }

    last_sent = now;
    tl12wl_build_and_send_config();
    sent++;
}
#endif /* BLUETOOTH_BHQ */
