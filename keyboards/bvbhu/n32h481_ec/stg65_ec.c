/* Copyright 2024 keymagichorse
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include "quantum.h"

#if defined(BLUETOOTH_BHQ)
#   include "bhq.h"
#   include "bhq_common.h"
#endif

#if defined(KB_LPM_ENABLED)
#   include "lpm.h"
#endif

#include "timer.h" /* timer_read32 / timer_elapsed32 */

void calibrate_matrix(void); /* stg65_ec_matrix.c：开机采样初始校准读数 */

/* ============================================================================
 *  0x11 SET_CONFIG 发送端（参考 keyboards/bvbhu/tl12blef103/tl12blef103.c）
 *
 *  一次性下发 蓝牙连接参数/发送功率/休眠/四组名称/VID-PID 给 BHQ 桥(CH582)。
 *  延迟 STG_CFG_DELAY_MS 再发：桥启动慢于主控，发早了会丢；
 *  STG_CFG_RETRY_COUNT 次重试（0x11 只发一次不可靠时兜底）。
 * ========================================================================= */
#if defined(BLUETOOTH_BHQ)

#    include "km_printf.h" /* km_printf: KB_DEBUG 未开时为空实现 */

#    ifndef STG_CFG_DELAY_MS
#        define STG_CFG_DELAY_MS 500
#    endif
#    ifndef STG_CFG_RETRY_COUNT
#        define STG_CFG_RETRY_COUNT 3
#    endif
#    ifndef STG_CFG_RETRY_INTERVAL_MS
#        define STG_CFG_RETRY_INTERVAL_MS 400
#    endif

/* BLE 广播名称设为 stg65_ec；Swift Pair 名称与 USB 产品名同 */
#    ifndef BLE_NAME
#        define BLE_NAME "stg65_ec"
#    endif
#    ifndef BLE_SWIFT_PAIR_NAME
#        define BLE_SWIFT_PAIR_NAME BLE_NAME
#    endif
#    ifndef USB_DOG_NAME
#        define USB_DOG_NAME BLE_NAME "_DOG"
#    endif
#    ifndef USB_DOG_VENDOR_NAME
#        define USB_DOG_VENDOR_NAME MANUFACTURER
#    endif

/* 连接参数：与 tl12blef103 相同（上游示例同值） */
#    ifndef STG_LE_INTERVAL_MIN
#        define STG_LE_INTERVAL_MIN 6 /* 单位 1.25ms */
#    endif
#    ifndef STG_LE_INTERVAL_MAX
#        define STG_LE_INTERVAL_MAX 35
#    endif
#    ifndef STG_LE_TIMEOUT
#        define STG_LE_TIMEOUT 500
#    endif
#    ifndef STG_TX_POWER
#        define STG_TX_POWER 0x3D /* 最强 */
#    endif
#    ifndef STG_SLEEP_1_S
#        define STG_SLEEP_1_S 10 /* 一级休眠(秒) */
#    endif
#    ifndef STG_SLEEP_2_S
#        define STG_SLEEP_2_S 300 /* 二级休眠/关机(秒) */
#    endif

static uint8_t stg_clamp_len(uint16_t len, uint16_t max, const char *what) {
    if (len > max) {
        km_printf("[cfg] %s too long: %d > %d, truncated\n", what, (int)len, (int)max);
        return (uint8_t)max;
    }
    return (uint8_t)len;
}

static void stg_build_and_send_config(void) {
    bhqDevConfigInfo_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    /* 蓝牙的配置信息 */
    cfg.le_connection_interval_min     = STG_LE_INTERVAL_MIN;
    cfg.le_connection_interval_max     = STG_LE_INTERVAL_MAX;
    cfg.le_connection_interval_timeout = STG_LE_TIMEOUT;
    cfg.tx_poweer                      = STG_TX_POWER;

    /* 电池相关字段已废弃, 全部填 0 (上游注释明确要求) */
    cfg.mk_is_read_battery_voltage = 0;
    cfg.mk_adc_pga                 = 0;
    cfg.mk_rvd_r1                  = 0;
    cfg.mk_rvd_r2                  = 0;

    cfg.sleep_1_s = STG_SLEEP_1_S;
    cfg.sleep_2_s = STG_SLEEP_2_S;

    /* 四组名称: 长度先夹到上游上限, 再拷贝内容 */
    cfg.bleNameStrLength = stg_clamp_len(strlen(BLE_NAME), BLE_ADVERT_NAME_MAX, "BLE_NAME");
    memcpy(cfg.bleNameStr, BLE_NAME, cfg.bleNameStrLength);

    cfg.bleSwiftPairNameStrLength = stg_clamp_len(strlen(BLE_SWIFT_PAIR_NAME), BLE_SWIFT_PAIR_NAME_MAX, "BLE_SWIFT_PAIR_NAME");
    memcpy(cfg.bleSwiftPairNameStr, BLE_SWIFT_PAIR_NAME, cfg.bleSwiftPairNameStrLength);

    cfg.usbNameStrLength = stg_clamp_len(strlen(USB_DOG_NAME), USB_NAME_MAX, "USB_DOG_NAME");
    memcpy(cfg.usbNameStr, USB_DOG_NAME, cfg.usbNameStrLength);

    cfg.usbVendorNameStrLength = stg_clamp_len(strlen(USB_DOG_VENDOR_NAME), USB_VENDOR_NAME_MAX, "USB_DOG_VENDOR_NAME");
    memcpy(cfg.usbVendorNameStr, USB_DOG_VENDOR_NAME, cfg.usbVendorNameStrLength);

    /* 通用的配置信息: VID/PID 同本键盘 USB 模式 (keyboard.json) */
    cfg.vendor_id_source = 1; /* 1 = USB-IF */
    cfg.verndor_id       = VENDOR_ID;
    cfg.product_id       = PRODUCT_ID;

    bhq_ConfigParam(cfg);
}

static void stg_config_task(void) {
    static uint8_t  sent      = 0;
    static uint32_t start_at  = 0;
    static uint32_t last_sent = 0;

    if (sent >= STG_CFG_RETRY_COUNT) {
        return;
    }

    uint32_t now = timer_read32();

    if (sent == 0) {
        /* 首次: 记录起点, 等 STG_CFG_DELAY_MS 让桥稳定后再发 */
        if (start_at == 0) {
            start_at = now ? now : 1; /* 避免 0 被当成"未初始化" */
            return;
        }
        if (timer_elapsed32(start_at) < STG_CFG_DELAY_MS) {
            return;
        }
    } else if (timer_elapsed32(last_sent) < STG_CFG_RETRY_INTERVAL_MS) {
        return;
    }

    last_sent = now;
    stg_build_and_send_config();
    sent++;
}
#endif /* BLUETOOTH_BHQ */

void board_init(void) {
#if defined(BLUETOOTH_BHQ)
#   if defined(KB_LPM_ENABLED)
    lpm_init();
#   endif
#endif
}

void keyboard_post_init_kb(void) {
    /* analog_init 在 keyboard_post_init_quantum 链尾之前已完成，
     * 此时初始校准读数采样写入 top_reading 才会被核心层正确接受 */
    calibrate_matrix();
    keyboard_post_init_user(); /* 覆盖了弱默认实现，必须手动回调 keymap 层 */
}

void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_wireless_task();
#   if defined(KB_LPM_ENABLED)
    lpm_task();
#   endif
    stg_config_task(); /* 0x11 配置下发(延迟 + 重试) */
#endif
}