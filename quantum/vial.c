/* Copyright 2020 Ilya Zhuravlev
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

#include "vial.h"

#include <string.h>

#include "dynamic_keymap.h"
#include "quantum.h"
#include "vial_generated_keyboard_definition.h"

#include "vial_ensure_keycode.h"

#ifdef ANALOG_MODEL
#    include "analog/analog_core.h"
#endif

#define VIAL_UNLOCK_COUNTER_MAX 50

#ifdef VIAL_INSECURE
#pragma message "Building Vial-enabled firmware in insecure mode."
int vial_unlocked = 1;
#else
int vial_unlocked = 0;
#endif
int vial_unlock_in_progress = 0;
static int vial_unlock_counter = 0;
static uint16_t vial_unlock_timer;

#ifndef VIAL_INSECURE
static uint8_t vial_unlock_combo_rows[] = VIAL_UNLOCK_COMBO_ROWS;
static uint8_t vial_unlock_combo_cols[] = VIAL_UNLOCK_COMBO_COLS;
#define VIAL_UNLOCK_NUM_KEYS (sizeof(vial_unlock_combo_rows)/sizeof(vial_unlock_combo_rows[0]))
_Static_assert(VIAL_UNLOCK_NUM_KEYS < 15, "Max 15 unlock keys");
_Static_assert(sizeof(vial_unlock_combo_rows) == sizeof(vial_unlock_combo_cols), "The number of unlock cols and rows should be the same");
#endif

#include "qmk_settings.h"

#ifdef VIAL_TAP_DANCE_ENABLE
static void reload_tap_dance(void);
#endif

#ifdef VIAL_COMBO_ENABLE
static void reload_combo(void);
#endif

#ifdef VIAL_KEY_OVERRIDE_ENABLE
static void reload_key_override(void);
#endif

#ifdef VIAL_ALT_REPEAT_KEY_ENABLE
static void reload_alt_repeat_key(void);
#endif

void vial_init(void) {
#ifdef VIAL_TAP_DANCE_ENABLE
    reload_tap_dance();
#endif
#ifdef VIAL_COMBO_ENABLE
    reload_combo();
#endif
#ifdef VIAL_KEY_OVERRIDE_ENABLE
    reload_key_override();
#endif
#ifdef VIAL_ALT_REPEAT_KEY_ENABLE
    reload_alt_repeat_key();
#endif
}

__attribute__((unused)) static uint16_t vial_keycode_firewall(uint16_t in) {
    if (in == QK_BOOT && !vial_unlocked)
        return 0;
    return in;
}

#ifdef ANALOG_MODEL
/* ==== Vial Analog 协议扩展(0xF0-0xF6)：线格式翻译层 ====
 * 命令语义见 docs/vial-analog-protocol.md。行程域 0..ANALOG_MAX_TRAVEL，轴体无关。
 * 线上 config ↔ 推送式状态机核心(analog_core.h) 的字段映射：
 *   actuation_point/release_point -> actuation/release_threshold(行程域阈值)
 *   rt_down/rt_up                 -> actuation/release_offset(RT 触发/释放距离)
 *   flags bit0 RT_ENABLED         -> ANALOG_FLAG_RT_ENABLED(同义)
 *   flags bit1 ACTUATION_OVERRIDE -> !FOLLOW_GLOBAL(线上 1=已自定义，与核心位相反)
 *   flags bit2 CONTINUOUS         -> ANALOG_FLAG_CONTINUOUS(随 flags 落盘)
 *   raw_rest/raw_full             -> top/bottom_reading(原始 ADC 域校准端点)
 * ki=0xFFFF 是全局默认槽：只有 5 项阈值+RT 有意义，校准端点/CONTINUOUS 对全局无意义；
 * 写全局经 analog_set_global 级联刷新所有跟随键，GUI 无需(也不应)逐键补写。 */

/* 协议版本基线号：定义于 analog_core.h(线格式与 EEPROM 落盘布局共用同一编号，
 * 见该处的说明)。改动须 bump：本处、analog_core.h 的同名宏与 GUI constants.py
 * 同步；旧 EEPROM 会随之整区作废回出厂值。 */


/* 协议层轴类型(仅供 GUI 显示)：核心层不持轴概念，按所选模型宏推导，kb 可覆盖 */
#ifndef ANALOG_PROTOCOL_AXIS_TYPE
#    if defined(ANALOG_MODEL_EC)
#        define ANALOG_PROTOCOL_AXIS_TYPE 2 /* 静电容 */
#    else
#        define ANALOG_PROTOCOL_AXIS_TYPE 1 /* 磁轴(Hall) */
#    endif
#endif

enum {
    VIAL_ANALOG_CAP_PER_KEY_ACTUATION = (1 << 0),
    VIAL_ANALOG_CAP_RAPID_TRIGGER     = (1 << 1),
    VIAL_ANALOG_CAP_CALIBRATION       = (1 << 2),
    VIAL_ANALOG_CAP_LIVE_READINGS     = (1 << 3),
    VIAL_ANALOG_CAP_PER_KEY_RELEASE   = (1 << 4),
    /* 触底校准开关(0xF4 mode 4/5)：开启时全部键等效 KC_NO，逐个按满即采集触底校准读数 */
    VIAL_ANALOG_CAP_BOTTOM_OUT_CAL    = (1 << 5),
    /* bit6 预留(AUTO_CAL：AUTO_PEAK 未实现，0xF4 mode3 返回错误码) */
    VIAL_ANALOG_CAPS_FLAGS = (VIAL_ANALOG_CAP_PER_KEY_ACTUATION | VIAL_ANALOG_CAP_RAPID_TRIGGER | VIAL_ANALOG_CAP_CALIBRATION | VIAL_ANALOG_CAP_LIVE_READINGS | VIAL_ANALOG_CAP_PER_KEY_RELEASE | VIAL_ANALOG_CAP_BOTTOM_OUT_CAL),
};

enum {
    VIAL_ANALOG_FLAG_RT_ENABLED         = (1 << 0),
    VIAL_ANALOG_FLAG_ACTUATION_OVERRIDE = (1 << 1),
    VIAL_ANALOG_FLAG_CONTINUOUS         = (1 << 2),
};

enum {
    VIAL_ANALOG_CAL_SAMPLE_REST    = 0,
    VIAL_ANALOG_CAL_SAMPLE_FULL    = 1,
    VIAL_ANALOG_CAL_RESET          = 2,
    VIAL_ANALOG_CAL_AUTO_PEAK      = 3, /* 未实现 */
    VIAL_ANALOG_CAL_BOTTOM_OUT_ON  = 4, /* 触底校准模式开：全部键等效 KC_NO */
    VIAL_ANALOG_CAL_BOTTOM_OUT_OFF = 5, /* 触底校准模式关 */
};

/* 0xF3 单包读数上限：每包 32 字节，msg[0] 放条数，余 31 字节装条目。
 * 条目 = 行程(sizeof(analog_travel_t)) + 原始读数 2 字节，故窄域 3 字节/条、
 * 宽域 4 字节/条。GUI 直接采用 caps 回报的条数上限，不自行推算。 */
#define VIAL_ANALOG_READING_ENTRY_BYTES (ANALOG_TRAVEL_WIDE ? 4u : 3u)
#define VIAL_ANALOG_MAX_READINGS        ((31u) / VIAL_ANALOG_READING_ENTRY_BYTES) /* 10 / 7 */

/* 每键配置线格式。4 项阈值宽度 = analog_travel_t(行程域宽度)，其余定长：
 *   行程域 uint8  -> 12 字节，raw_rest 在 [6..7]
 *   行程域 uint16 -> 16 字节，raw_rest 在 [10..11]
 * 宽度由 0xF0 的 msg[6](= 本结构 sizeof)与 msg[8..9](最大键程值)共同声明；
 * GUI 必须先读 caps 再解析，不得写死字段偏移。 */
typedef struct __attribute__((packed)) {
    analog_travel_t actuation_point;
    analog_travel_t release_point;
    analog_travel_t rt_down;
    analog_travel_t rt_up;
    uint8_t  flags;
    uint8_t  reserved;
    uint16_t raw_rest;
    uint16_t raw_full;
    uint16_t reserved2;
} vial_analog_wire_config_t;

_Static_assert(sizeof(vial_analog_wire_config_t) == (ANALOG_TRAVEL_WIDE ? 16u : 12u), "wire config 尺寸随行程域宽度变化：uint8 域 12 字节 / uint16 域 16 字节，且必须无填充");

static uint8_t vial_analog_wire_flags(const analog_key_t *k) {
    uint8_t f = 0;
    if (k->flags & ANALOG_FLAG_RT_ENABLED) f |= VIAL_ANALOG_FLAG_RT_ENABLED;
    if (!(k->flags & ANALOG_FLAG_FOLLOW_GLOBAL)) f |= VIAL_ANALOG_FLAG_ACTUATION_OVERRIDE;
    if (k->flags & ANALOG_FLAG_CONTINUOUS) f |= VIAL_ANALOG_FLAG_CONTINUOUS;
    return f;
}

/* 核心 -> 线格式(0xF1)。ki 无效由调用方先行拦截。 */
static void vial_analog_get_wire_config(uint16_t ki, vial_analog_wire_config_t *c) {
    memset(c, 0, sizeof(*c));
    if (ki == 0xFFFF) {
        c->actuation_point = g_analog_global.actuation_threshold;
        c->release_point   = g_analog_global.release_threshold;
        c->rt_down         = g_analog_global.actuation_offset;
        c->rt_up           = g_analog_global.release_offset;
        if (g_analog_global.rt_enabled) c->flags = VIAL_ANALOG_FLAG_RT_ENABLED;
        c->raw_rest = ANALOG_TOPREADING_MAX;    /* 全局槽不持初始校准读数：回默认校准值 */
        c->raw_full = ANALOG_BOTTOMREADING_MIN;
        return;
    }
    const analog_key_t *k = &g_analog_key[ki];
    c->actuation_point = k->actuation_threshold;
    c->release_point   = k->release_threshold;
    c->rt_down         = k->actuation_offset;
    c->rt_up           = k->release_offset;
    c->flags           = vial_analog_wire_flags(k);
    c->raw_rest        = k->top_reading;
    c->raw_full        = k->bottom_reading;
}

/* 线格式 -> 核心(0xF2 的实际处理)。返回错误码(0=成功)。
 * 0xF2 仅改 RAM 不落盘：由外层 vial_analog_set_wire_config 包 suppress。 */
static uint8_t vial_analog_set_wire_config_impl(uint16_t ki, const vial_analog_wire_config_t *c) {
    if (ki == 0xFFFF) {
        analog_global_t g;
        g.actuation_threshold = c->actuation_point;
        g.release_threshold   = c->release_point;
        g.actuation_offset    = c->rt_down;
        g.release_offset      = c->rt_up;
        g.rt_enabled          = (c->flags & VIAL_ANALOG_FLAG_RT_ENABLED) ? 1 : 0;
        analog_set_global(&g);
        return 0;
    }
    if (ki >= ANALOG_NUM_KEYS) return 1;

    /* ACTUATION_OVERRIDE=0 表示"该键归全局管"：先按 0xF5 单键复位的语义回到跟随态
     * (取全局阈值+RT 位、置 FOLLOW_GLOBAL)，校准端点不动——校准端点是本键物理量。
     * 否则走自定义路径：set_key_config 无条件清 FOLLOW_GLOBAL 即是"转自定义"。 */
    if (!(c->flags & VIAL_ANALOG_FLAG_ACTUATION_OVERRIDE)) {
        analog_reset_key(ki);
    } else {
        const analog_travel_t params[4] = { c->actuation_point, c->release_point, c->rt_down, c->rt_up };
        analog_set_key_config(ki, params, (c->flags & VIAL_ANALOG_FLAG_RT_ENABLED) != 0);
    }

    /* CONTINUOUS 位写进核心 flags(bit2)，随持久化记录一起落盘、0xF1 原样回报。
     * 必须在上面两条分支之后写：它们都会整体改写 flags 而覆盖本位。
     * 另注意仅在位值真变化时标脏，避免 GUI 轮询式重写刷爆 FEE 写日志。 */
    uint8_t want = (c->flags & VIAL_ANALOG_FLAG_CONTINUOUS) ? (uint8_t)ANALOG_FLAG_CONTINUOUS : (uint8_t)0;
    if ((g_analog_key[ki].flags & ANALOG_FLAG_CONTINUOUS) != want) {
        g_analog_key[ki].flags = (uint8_t)((g_analog_key[ki].flags & (uint8_t)~ANALOG_FLAG_CONTINUOUS) | want);
        analog_mark_dirty(ki);
    }
    /* 校准端点随配置一起传：0xF4 之外显式写校准端点的合法途径(值未变时 update-block 不磨损) */
    analog_set_top_reading(ki, c->raw_rest);
    analog_set_bottom_reading(ki, c->raw_full);
    return 0;
}

/* 0xF2 对外入口：包一层 suppress，使本次 set 全程只改 RAM、不标脏落盘。
 * 调参(拖滑块)反复发 0xF2 也不会磨损 EEPROM；用户点 GUI "保存"才发 0xF6 落盘。
 * 校准(0xF4)/复位(0xF5)/kb 扫描不经过本层，仍各自即时标脏落盘。 */
static uint8_t vial_analog_set_wire_config(uint16_t ki, const vial_analog_wire_config_t *c) {
    analog_set_persist_suppress(true);
    uint8_t r = vial_analog_set_wire_config_impl(ki, c);
    analog_set_persist_suppress(false);
    return r;
}
#endif /* ANALOG_MODEL */

void vial_handle_cmd(uint8_t *msg, uint8_t length) {
    /* All packets must be fixed 32 bytes */
    if (length != VIAL_RAW_EPSIZE)
        return;

    /* msg[0] is 0xFE -- prefix vial magic */
    switch (msg[1]) {
        /* Get keyboard ID and Vial protocol version */
        case vial_get_keyboard_id: {
            uint8_t keyboard_uid[] = VIAL_KEYBOARD_UID;

            memset(msg, 0, length);
            msg[0] = VIAL_PROTOCOL_VERSION & 0xFF;
            msg[1] = (VIAL_PROTOCOL_VERSION >> 8) & 0xFF;
            msg[2] = (VIAL_PROTOCOL_VERSION >> 16) & 0xFF;
            msg[3] = (VIAL_PROTOCOL_VERSION >> 24) & 0xFF;
            memcpy(&msg[4], keyboard_uid, 8);
#ifdef VIALRGB_ENABLE
            msg[12] = 1; /* bit flag to indicate vialrgb is supported - so third-party apps don't have to query json */
#endif
            break;
        }
        /* Retrieve keyboard definition size */
        case vial_get_size: {
            uint32_t sz = sizeof(keyboard_definition);
            msg[0] = sz & 0xFF;
            msg[1] = (sz >> 8) & 0xFF;
            msg[2] = (sz >> 16) & 0xFF;
            msg[3] = (sz >> 24) & 0xFF;
            break;
        }
        /* Retrieve 32-bytes block of the definition, page ID encoded within 2 bytes */
        case vial_get_def: {
            uint32_t page = msg[2] + (msg[3] << 8);
            uint32_t start = page * VIAL_RAW_EPSIZE;
            uint32_t end = start + VIAL_RAW_EPSIZE;
            if (end < start || start >= sizeof(keyboard_definition))
                return;
            if (end > sizeof(keyboard_definition))
                end = sizeof(keyboard_definition);
            memcpy_P(msg, &keyboard_definition[start], end - start);
            break;
        }
#ifdef ENCODER_MAP_ENABLE
        case vial_get_encoder: {
            uint8_t layer = msg[2];
            uint8_t idx = msg[3];
            uint16_t keycode = dynamic_keymap_get_encoder(layer, idx, 0);
            msg[0]  = keycode >> 8;
            msg[1]  = keycode & 0xFF;
            keycode = dynamic_keymap_get_encoder(layer, idx, 1);
            msg[2] = keycode >> 8;
            msg[3] = keycode & 0xFF;
            break;
        }
        case vial_set_encoder: {
            dynamic_keymap_set_encoder(msg[2], msg[3], msg[4], vial_keycode_firewall((msg[5] << 8) | msg[6]));
            break;
        }
#endif
        case vial_get_unlock_status: {
            /* Reset message to all FF's */
            memset(msg, 0xFF, length);
            /* First byte of message contains the status: whether board is unlocked */
            msg[0] = vial_unlocked;
            /* Second byte is whether unlock is in progress */
            msg[1] = vial_unlock_in_progress;
#ifndef VIAL_INSECURE
            /* Rest of the message are keys in the matrix that should be held to unlock the board */
            for (size_t i = 0; i < VIAL_UNLOCK_NUM_KEYS; ++i) {
                msg[2 + i * 2] = vial_unlock_combo_rows[i];
                msg[2 + i * 2 + 1] = vial_unlock_combo_cols[i];
            }
#endif
            break;
        }
        case vial_unlock_start: {
            vial_unlock_in_progress = 1;
            vial_unlock_counter = VIAL_UNLOCK_COUNTER_MAX;
            vial_unlock_timer = timer_read();
            break;
        }
        case vial_unlock_poll: {
#ifndef VIAL_INSECURE
            if (vial_unlock_in_progress) {
                int holding = 1;
                for (size_t i = 0; i < VIAL_UNLOCK_NUM_KEYS; ++i)
                    holding &= matrix_is_on(vial_unlock_combo_rows[i], vial_unlock_combo_cols[i]);

                if (timer_elapsed(vial_unlock_timer) > 100 && holding) {
                    vial_unlock_timer = timer_read();

                    vial_unlock_counter--;
                    if (vial_unlock_counter == 0) {
                        /* ok unlock succeeded */
                        vial_unlock_in_progress = 0;
                        vial_unlocked = 1;
                    }
                } else {
                    vial_unlock_counter = VIAL_UNLOCK_COUNTER_MAX;
                }
            }
#endif
            msg[0] = vial_unlocked;
            msg[1] = vial_unlock_in_progress;
            msg[2] = vial_unlock_counter;
            break;
        }
        case vial_lock: {
#ifndef VIAL_INSECURE
            vial_unlocked = 0;
#endif
            break;
        }
        case vial_qmk_settings_query: {
#ifdef QMK_SETTINGS
            uint16_t qsid_greater_than = msg[2] | (msg[3] << 8);
            qmk_settings_query(qsid_greater_than, msg, length);
#else
            memset(msg, 0xFF, length); /* indicate that we don't support any qsid */
#endif
            break;
        }
#ifdef QMK_SETTINGS
        case vial_qmk_settings_get: {
            uint16_t qsid = msg[2] | (msg[3] << 8);
            msg[0] = qmk_settings_get(qsid, &msg[1], length - 1);

            break;
        }
        case vial_qmk_settings_set: {
            uint16_t qsid = msg[2] | (msg[3] << 8);
            msg[0] = qmk_settings_set(qsid, &msg[4], length - 4);

            break;
        }
        case vial_qmk_settings_reset: {
            qmk_settings_reset();
            break;
        }
#endif
        case vial_dynamic_entry_op: {
            switch (msg[2]) {
            case dynamic_vial_get_number_of_entries: {
                memset(msg, 0, length);
                msg[0] = VIAL_TAP_DANCE_ENTRIES;
                msg[1] = VIAL_COMBO_ENTRIES;
                msg[2] = VIAL_KEY_OVERRIDE_ENTRIES;
                msg[3] = VIAL_ALT_REPEAT_KEY_ENTRIES;

                // The last byte of msg indicates optionally supported features.
                msg[length - 1] = (0
#ifdef CAPS_WORD_ENABLE
                        | (1 << 0)  // Bit 0: Caps Word.
#endif
#ifdef LAYER_LOCK_ENABLE
                        | (1 << 1)  // Bit 1: Layer Lock.
#endif
#ifdef BLUETOOTH_ENABLE
                        | (1 << 2)  // Bit 2: Wireless (BHQ/BLE).
#endif
                        );
                break;
            }
#ifdef VIAL_TAP_DANCE_ENABLE
            case dynamic_vial_tap_dance_get: {
                uint8_t idx = msg[3];
                vial_tap_dance_entry_t td = { 0 };
                msg[0] = dynamic_keymap_get_tap_dance(idx, &td);
                memcpy(&msg[1], &td, sizeof(td));
                break;
            }
            case dynamic_vial_tap_dance_set: {
                uint8_t idx = msg[3];
                vial_tap_dance_entry_t td;
                memcpy(&td, &msg[4], sizeof(td));
                td.on_tap = vial_keycode_firewall(td.on_tap);
                td.on_hold = vial_keycode_firewall(td.on_hold);
                td.on_double_tap = vial_keycode_firewall(td.on_double_tap);
                td.on_tap_hold = vial_keycode_firewall(td.on_tap_hold);
                msg[0] = dynamic_keymap_set_tap_dance(idx, &td);
                reload_tap_dance();
                break;
            }
#endif
#ifdef VIAL_COMBO_ENABLE
            case dynamic_vial_combo_get: {
                uint8_t idx = msg[3];
                vial_combo_entry_t entry = { 0 };
                msg[0] = dynamic_keymap_get_combo(idx, &entry);
                memcpy(&msg[1], &entry, sizeof(entry));
                break;
            }
            case dynamic_vial_combo_set: {
                uint8_t idx = msg[3];
                vial_combo_entry_t entry;
                memcpy(&entry, &msg[4], sizeof(entry));
                entry.output = vial_keycode_firewall(entry.output);
                msg[0] = dynamic_keymap_set_combo(idx, &entry);
                reload_combo();
                break;
            }
#endif
#ifdef VIAL_KEY_OVERRIDE_ENABLE
            case dynamic_vial_key_override_get: {
                uint8_t idx = msg[3];
                vial_key_override_entry_t entry = { 0 };
                msg[0] = dynamic_keymap_get_key_override(idx, &entry);
                memcpy(&msg[1], &entry, sizeof(entry));
                break;
            }
            case dynamic_vial_key_override_set: {
                uint8_t idx = msg[3];
                vial_key_override_entry_t entry;
                memcpy(&entry, &msg[4], sizeof(entry));
                entry.replacement = vial_keycode_firewall(entry.replacement);
                msg[0] = dynamic_keymap_set_key_override(idx, &entry);
                reload_key_override();
                break;
            }
#endif
#ifdef VIAL_ALT_REPEAT_KEY_ENABLE
            case dynamic_vial_alt_repeat_key_get: {
                uint8_t idx = msg[3];
                vial_alt_repeat_key_entry_t entry = { 0 };
                msg[0] = dynamic_keymap_get_alt_repeat_key(idx, &entry);
                memcpy(&msg[1], &entry, sizeof(entry));
                break;
            }
            case dynamic_vial_alt_repeat_key_set: {
                uint8_t idx = msg[3];
                vial_alt_repeat_key_entry_t entry;
                memcpy(&entry, &msg[4], sizeof(entry));
                entry.keycode = vial_keycode_firewall(entry.keycode);
                entry.alt_keycode = vial_keycode_firewall(entry.alt_keycode);
                msg[0] = dynamic_keymap_set_alt_repeat_key(idx, &entry);
                reload_alt_repeat_key();
                break;
            }
#endif
            }

            break;
        }
#ifdef ANALOG_MODEL
        /* ---- Vial Analog 协议扩展(0xF0-0xF5)，翻译层见文件头部说明 ---- */
        case vial_analog_get_caps: {
            memset(msg, 0, length);
            msg[0] = VIAL_ANALOG_PROTOCOL_VERSION;
            msg[1] = ANALOG_NUM_KEYS & 0xFF;
            msg[2] = 0; /* ANALOG_NUM_KEYS <= 255(analog_core.c 静态断言) */
            msg[3] = ANALOG_PROTOCOL_AXIS_TYPE;
            msg[4] = VIAL_ANALOG_CAPS_FLAGS;
            msg[5] = VIAL_ANALOG_MAX_READINGS;
            msg[6] = sizeof(vial_analog_wire_config_t);
            msg[7] = analog_get_bottom_out_mode() ? 1 : 0; /* 触底校准开关状态：GUI 重启后能对上(旧固件恒 0) */
            /* 最大键程值(小端)：GUI 据此对齐量程与推导字段宽度。编译期常量，只此分发一次。 */
            msg[8] = (uint8_t)((uint16_t)ANALOG_MAX_TRAVEL & 0xFF);
            msg[9] = (uint8_t)((uint16_t)ANALOG_MAX_TRAVEL >> 8);
            break;
        }
        case vial_analog_get_key_config: {
            uint16_t ki = msg[2] | ((uint16_t)msg[3] << 8);
            if (ki != 0xFFFF && ki >= ANALOG_NUM_KEYS) {
                msg[0] = 1;
                break;
            }
            vial_analog_wire_config_t c;
            vial_analog_get_wire_config(ki, &c);
            memcpy(msg, &c, sizeof(c));
            break;
        }
        case vial_analog_set_key_config: {
            uint16_t ki = msg[2] | ((uint16_t)msg[3] << 8);
            vial_analog_wire_config_t c;
            memcpy(&c, &msg[4], sizeof(c));
            msg[0] = vial_analog_set_wire_config(ki, &c);
            break;
        }
        case vial_analog_get_key_readings: {
            uint16_t start = msg[2] | ((uint16_t)msg[3] << 8);
            /* 清包必须在读完入参 start(msg[2..3]) 之后：n 较小时 msg[1..] 会残留上一包字节。 */
            memset(msg, 0, length);
            uint8_t  n     = 0;
            if (start < ANALOG_NUM_KEYS) {
                n = (uint8_t)(ANALOG_NUM_KEYS - start);
                if (n > VIAL_ANALOG_MAX_READINGS) n = VIAL_ANALOG_MAX_READINGS;
                for (uint8_t i = 0; i < n; i++) {
                    uint16_t k    = start + i;
                    int16_t  raw  = analog_backend_get_raw_adc(k);
                    uint16_t rawu = (raw < 0) ? 0 : (uint16_t)raw;
                    /* 条目 = 行程(小端, 1/2 字节) + raw(小端 16 位)；窄 3/宽 4 字节 */
                    uint8_t        *e  = &msg[1 + (uint16_t)i * VIAL_ANALOG_READING_ENTRY_BYTES];
                    analog_travel_t sw = (raw < 0) ? 0 : analog_model_sw(k, rawu);
                    e[0] = (uint8_t)(sw & 0xFFu);
#if ANALOG_TRAVEL_WIDE
                    e[1] = (uint8_t)(sw >> 8);
                    e[2] = (uint8_t)(rawu & 0xFFu);
                    e[3] = (uint8_t)(rawu >> 8);
#else
                    e[1] = (uint8_t)(rawu & 0xFFu);
                    e[2] = (uint8_t)(rawu >> 8);
#endif
                }
            }
            msg[0] = n;
            break;
        }
        case vial_analog_calibrate: {
            uint8_t  mode = msg[2];
            uint16_t ki   = msg[3] | ((uint16_t)msg[4] << 8);
            msg[0] = 1; /* 未知模式/参数越界默认报错 */

            /* 收到任何 0xF4 都续期触底校准的超时心跳(analog_core.h §5.5 兜底②)：
             * 只要 GUI 还在轮询/操作，模式就不会被自动关掉；GUI 消失则超时自关。 */
            analog_bottom_out_heartbeat();

            /* 触底校准开关先判：只看 mode、忽略 ki(见协议文档 §0xF4)，
             * 故越界 ki 不影响这两个 mode。 */
            if (mode == VIAL_ANALOG_CAL_BOTTOM_OUT_ON || mode == VIAL_ANALOG_CAL_BOTTOM_OUT_OFF) {
                /* 纯运行态、不落盘(上次没关绝不能带到下次启动变砖)。
                 * 期间扫描侧抑制输出并只推高各键 bottom_reading，见 tl96mgf072_matrix.c */
                analog_set_bottom_out_mode(mode == VIAL_ANALOG_CAL_BOTTOM_OUT_ON);
                msg[0] = 0;
                break;
            }

            uint16_t lo = (ki == 0xFFFF) ? 0 : ki;
            uint16_t hi = (ki == 0xFFFF) ? (ANALOG_NUM_KEYS - 1) : ki;
            if (lo >= ANALOG_NUM_KEYS || lo > hi) break;
            switch (mode) {
                case VIAL_ANALOG_CAL_SAMPLE_REST:
                case VIAL_ANALOG_CAL_SAMPLE_FULL: {
                    int16_t first = -1;
                    for (uint16_t i = lo; i <= hi; i++) {
                        int16_t raw = analog_backend_get_raw_adc(i);
                        if (raw < 0) continue;
                        if (mode == VIAL_ANALOG_CAL_SAMPLE_REST) {
                            analog_set_top_reading(i, (uint16_t)raw);
                        } else {
                            analog_set_bottom_reading(i, (uint16_t)raw);
                        }
                        if (first < 0) first = raw;
                    }
                    if (first >= 0) {
                        msg[0] = 0;
                        msg[1] = (uint8_t)(first & 0xFF);
                        msg[2] = (uint8_t)((uint16_t)first >> 8);
                    } else {
                        msg[0] = 2; /* kb 后端不可用 */
                    }
                    break;
                }
                case VIAL_ANALOG_CAL_RESET:
                    /* 恢复默认校准值(非 0/255：校准端点是原始 ADC 域物理量，0 会让模型失效到重开机) */
                    for (uint16_t i = lo; i <= hi; i++) {
                        analog_set_top_reading(i, ANALOG_TOPREADING_MAX);
                        analog_set_bottom_reading(i, ANALOG_BOTTOMREADING_MIN);
                    }
                    msg[0] = 0;
                    break;
                default: /* AUTO_PEAK 未实现：保持错误码 1，caps 不报 AUTO_CAL 位 */
                    break;
            }
            break;
        }
        case vial_analog_reset_key: {
            uint16_t ki = msg[2] | ((uint16_t)msg[3] << 8);
            msg[0] = analog_reset_key(ki) ? 0 : 1;
            break;
        }
        case vial_analog_persist_commit: {
            /* 0xF6：显式保存——把当前 RAM 全量落盘 EEPROM。
             * 0xF2 调参只改 RAM(suppress)，用户点 GUI "保存"才经此命令写 EEPROM。
             * 校准/复位仍各自即时落盘，不经此命令。 */
            analog_persist_commit();
            msg[0] = 0;
            break;
        }
#endif
    }
}

uint16_t g_vial_magic_keycode_override;

void vial_keycode_down(uint16_t keycode) {
    g_vial_magic_keycode_override = keycode;

    if (keycode <= QK_MODS_MAX) {
        register_code16(keycode);
    } else {
        action_exec((keyevent_t){
            .type = KEY_EVENT,
            .key = (keypos_t){.row = VIAL_MATRIX_MAGIC, .col = VIAL_MATRIX_MAGIC}, .pressed = 1, .time = (timer_read() | 1) /* time should not be 0 */
        });
    }
}

void vial_keycode_up(uint16_t keycode) {
    g_vial_magic_keycode_override = keycode;

    if (keycode <= QK_MODS_MAX) {
        unregister_code16(keycode);
    } else {
        action_exec((keyevent_t){
            .type = KEY_EVENT,
            .key = (keypos_t){.row = VIAL_MATRIX_MAGIC, .col = VIAL_MATRIX_MAGIC}, .pressed = 0, .time = (timer_read() | 1) /* time should not be 0 */
        });
    }
}

void vial_keycode_tap(uint16_t keycode) {
    vial_keycode_down(keycode);
    qs_wait_ms(QS_tap_code_delay);
    vial_keycode_up(keycode);
}

#ifdef VIAL_TAP_DANCE_ENABLE
#include "process_tap_dance.h"

/* based on ZSA configurator generated code */

enum {
    SINGLE_TAP = 1,
    SINGLE_HOLD,
    DOUBLE_TAP,
    DOUBLE_HOLD,
    DOUBLE_SINGLE_TAP,
    MORE_TAPS
};

static uint8_t dance_state[VIAL_TAP_DANCE_ENTRIES];
static vial_tap_dance_entry_t td_entry;

static uint8_t dance_step(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return SINGLE_TAP;
        else return SINGLE_HOLD;
    } else if (state->count == 2) {
        if (state->interrupted) return DOUBLE_SINGLE_TAP;
        else if (state->pressed) return DOUBLE_HOLD;
        else return DOUBLE_TAP;
    }
    return MORE_TAPS;
}

static void on_dance(tap_dance_state_t *state, void *user_data) {
    uint8_t index = (uintptr_t)user_data;
    if (dynamic_keymap_get_tap_dance(index, &td_entry) != 0)
        return;
    uint16_t kc = td_entry.on_tap;
    if (kc) {
        if (state->count == 3) {
            vial_keycode_tap(kc);
            vial_keycode_tap(kc);
            vial_keycode_tap(kc);
        } else if (state->count > 3) {
            vial_keycode_tap(kc);
        }
    }
}

static void on_dance_finished(tap_dance_state_t *state, void *user_data) {
    uint8_t index = (uintptr_t)user_data;
    if (dynamic_keymap_get_tap_dance(index, &td_entry) != 0)
        return;
    dance_state[index] = dance_step(state);
    switch (dance_state[index]) {
        case SINGLE_TAP: {
            if (td_entry.on_tap)
                vial_keycode_down(td_entry.on_tap);
            break;
        }
        case SINGLE_HOLD: {
            if (td_entry.on_hold)
                vial_keycode_down(td_entry.on_hold);
            else if (td_entry.on_tap)
                vial_keycode_down(td_entry.on_tap);
            break;
        }
        case DOUBLE_TAP: {
            if (td_entry.on_double_tap) {
                vial_keycode_down(td_entry.on_double_tap);
            } else if (td_entry.on_tap) {
                vial_keycode_tap(td_entry.on_tap);
                vial_keycode_down(td_entry.on_tap);
            }
            break;
        }
        case DOUBLE_HOLD: {
            if (td_entry.on_tap_hold) {
                vial_keycode_down(td_entry.on_tap_hold);
            } else {
                if (td_entry.on_tap) {
                    vial_keycode_tap(td_entry.on_tap);
                    if (td_entry.on_hold)
                        vial_keycode_down(td_entry.on_hold);
                    else
                        vial_keycode_down(td_entry.on_tap);
                } else if (td_entry.on_hold) {
                    vial_keycode_down(td_entry.on_hold);
                }
            }
            break;
        }
        case DOUBLE_SINGLE_TAP: {
            if (td_entry.on_tap) {
                vial_keycode_tap(td_entry.on_tap);
                vial_keycode_down(td_entry.on_tap);
            }
            break;
        }
    }
}

static void on_dance_reset(tap_dance_state_t *state, void *user_data) {
    uint8_t index = (uintptr_t)user_data;
    if (dynamic_keymap_get_tap_dance(index, &td_entry) != 0)
        return;
    qs_wait_ms(QS_tap_code_delay);
    uint8_t st = dance_state[index];
    state->count = 0;
    dance_state[index] = 0;
    switch (st) {
        case SINGLE_TAP: {
            if (td_entry.on_tap)
                vial_keycode_up(td_entry.on_tap);
            break;
        }
        case SINGLE_HOLD: {
            if (td_entry.on_hold)
                vial_keycode_up(td_entry.on_hold);
            else if (td_entry.on_tap)
                vial_keycode_up(td_entry.on_tap);
            break;
        }
        case DOUBLE_TAP: {
            if (td_entry.on_double_tap) {
                vial_keycode_up(td_entry.on_double_tap);
            } else if (td_entry.on_tap) {
                vial_keycode_up(td_entry.on_tap);
            }
            break;
        }
        case DOUBLE_HOLD: {
            if (td_entry.on_tap_hold) {
                vial_keycode_up(td_entry.on_tap_hold);
            } else {
                if (td_entry.on_tap) {
                    if (td_entry.on_hold)
                        vial_keycode_up(td_entry.on_hold);
                    else
                        vial_keycode_up(td_entry.on_tap);
                } else if (td_entry.on_hold) {
                    vial_keycode_up(td_entry.on_hold);
                }
            }
            break;
        }
        case DOUBLE_SINGLE_TAP: {
            if (td_entry.on_tap) {
                vial_keycode_up(td_entry.on_tap);
            }
            break;
        }
    }
}

tap_dance_action_t tap_dance_actions[VIAL_TAP_DANCE_ENTRIES] = { };

/* Load timings from eeprom into custom_tapping_term */
static void reload_tap_dance(void) {
    for (size_t i = 0; i < VIAL_TAP_DANCE_ENTRIES; ++i) {
        tap_dance_actions[i].fn.on_each_tap = on_dance;
        tap_dance_actions[i].fn.on_dance_finished = on_dance_finished;
        tap_dance_actions[i].fn.on_reset = on_dance_reset;
        tap_dance_actions[i].user_data = (void*)i;
    }
}
#endif

#ifdef TAPPING_TERM_PER_KEY
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
#ifdef VIAL_TAP_DANCE_ENABLE
    if (keycode >= QK_TAP_DANCE && keycode <= QK_TAP_DANCE_MAX) {
        vial_tap_dance_entry_t td;
        if (dynamic_keymap_get_tap_dance(keycode & 0xFF, &td) == 0)
            return td.custom_tapping_term;
    }
#endif
#ifdef QMK_SETTINGS
    return qs_get_tapping_term(keycode, record);
#else
    return TAPPING_TERM;
#endif
}

uint16_t tap_dance_count(void) {
    return VIAL_TAP_DANCE_ENTRIES;
}

tap_dance_action_t* tap_dance_get(uint16_t tap_dance_idx) {
    if (tap_dance_idx >= VIAL_TAP_DANCE_ENTRIES)
        return NULL;
    return &tap_dance_actions[tap_dance_idx];
}
#endif

#ifdef VIAL_COMBO_ENABLE
combo_t key_combos[VIAL_COMBO_ENTRIES] = { };
uint16_t key_combos_keys[VIAL_COMBO_ENTRIES][5];

static void reload_combo(void) {
    /* initialize with all keys = COMBO_END */
    memset(key_combos_keys, 0, sizeof(key_combos_keys));
    memset(key_combos, 0, sizeof(key_combos));

    /* reload from eeprom */
    for (size_t i = 0; i < VIAL_COMBO_ENTRIES; ++i) {
        uint16_t *seq = key_combos_keys[i];
        key_combos[i].keys = seq;

        vial_combo_entry_t entry;
        if (dynamic_keymap_get_combo(i, &entry) == 0) {
            memcpy(seq, entry.input, sizeof(entry.input));
            key_combos[i].keycode = entry.output;
        }
    }
}
#endif

#ifdef VIAL_TAP_DANCE_ENABLE
void process_tap_dance_action_on_dance_finished(tap_dance_action_t *action);
#endif

bool process_record_vial(uint16_t keycode, keyrecord_t *record) {
#ifdef VIAL_TAP_DANCE_ENABLE
    /* process releases before tap-dance timeout arrives */
    if (!record->event.pressed && keycode >= QK_TAP_DANCE && keycode <= QK_TAP_DANCE_MAX) {
        uint16_t idx = keycode - QK_TAP_DANCE;
        if (dynamic_keymap_get_tap_dance(idx, &td_entry) != 0)
            return true;

        tap_dance_action_t *action = &tap_dance_actions[idx];

        /* only care about 2 possibilities here
           - tap and hold set, everything else unset: process first release early (count == 1)
           - double tap set: process second release early (count == 2)
         */
        if ((action->state.count == 1 && td_entry.on_tap && td_entry.on_hold && !td_entry.on_double_tap && !td_entry.on_tap_hold)
            || (action->state.count == 2 && td_entry.on_double_tap)) {
                action->state.pressed = false;
                process_tap_dance_action_on_dance_finished(action);
                /* reset_tap_dance() will get called in process_tap_dance() */
            }
    }
#endif

    return true;
}

#ifdef VIAL_KEY_OVERRIDE_ENABLE
static bool vial_key_override_disabled = 0;
static key_override_t vial_key_overrides[VIAL_KEY_OVERRIDE_ENTRIES] = { 0 };

static int vial_get_key_override(uint8_t index, key_override_t *out) {
    vial_key_override_entry_t entry;
    int ret;
    if ((ret = dynamic_keymap_get_key_override(index, &entry)) != 0)
        return ret;

    memset(out, 0, sizeof(*out));
    out->trigger = entry.trigger;
    out->trigger_mods = entry.trigger_mods;
    out->layers = entry.layers;
    out->negative_mod_mask = entry.negative_mod_mask;
    out->suppressed_mods = entry.suppressed_mods;
    out->replacement = entry.replacement;
    out->options = 0;
    uint8_t opt = entry.options;
    if (opt & vial_ko_enabled)
        out->enabled = NULL;
    else
        out->enabled = &vial_key_override_disabled;
    /* right now these options match one-to-one so this isn't strictly necessary,
       nevertheless future-proof the code by parsing them out to ensure "stable" abi */
    if (opt & vial_ko_option_activation_trigger_down) out->options |= ko_option_activation_trigger_down;
    if (opt & vial_ko_option_activation_required_mod_down) out->options |= ko_option_activation_required_mod_down;
    if (opt & vial_ko_option_activation_negative_mod_up) out->options |= ko_option_activation_negative_mod_up;
    if (opt & vial_ko_option_one_mod) out->options |= ko_option_one_mod;
    if (opt & vial_ko_option_no_reregister_trigger) out->options |= ko_option_no_reregister_trigger;
    if (opt & vial_ko_option_no_unregister_on_other_key_down) out->options |= ko_option_no_unregister_on_other_key_down;

    return 0;
}

static void reload_key_override(void) {
    for (size_t i = 0; i < VIAL_KEY_OVERRIDE_ENTRIES; ++i)
        vial_get_key_override(i, &vial_key_overrides[i]);
}

uint16_t key_override_count(void) {
    return VIAL_KEY_OVERRIDE_ENTRIES;
}

const key_override_t* key_override_get(uint16_t key_override_idx) {
    if (key_override_idx >= VIAL_KEY_OVERRIDE_ENTRIES)
        return NULL;
    return &vial_key_overrides[key_override_idx];
}
#endif

#ifdef VIAL_ALT_REPEAT_KEY_ENABLE
typedef struct {
    uint16_t keycode;
    uint16_t alt_keycode;
    uint8_t required_mods;
    uint8_t alt_required_mods;
    uint8_t allowed_mods;
    uint8_t options;
} alt_repeat_key_t;

static alt_repeat_key_t vial_alt_repeat_key[VIAL_ALT_REPEAT_KEY_ENTRIES] = { 0 };

static uint8_t unpack_mods5(uint8_t mods5) {
  return (mods5 & 0x10) != 0 ? (mods5 << 4) : mods5;
}

static uint16_t alt_repeat_key_normalize_keycode(uint16_t keycode, uint8_t *mods) {
    switch (keycode) {
        case QK_MODS ... QK_MODS_MAX: // Unpack modifier + basic key.
            *mods |= unpack_mods5(QK_MODS_GET_MODS(keycode));
            keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
            break;
        case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
            break;
        case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
            keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
            break;
    }
    return keycode;
}

static int vial_get_alt_repeat_key(uint8_t index, alt_repeat_key_t *out) {
    vial_alt_repeat_key_entry_t entry;
    int ret;
    if ((ret = dynamic_keymap_get_alt_repeat_key(index, &entry)) != 0) {
        return ret;
    }

    memset(out, 0, sizeof(*out));
    out->keycode = alt_repeat_key_normalize_keycode(entry.keycode, &out->required_mods);
    out->alt_keycode = alt_repeat_key_normalize_keycode(entry.alt_keycode, &out->alt_required_mods);
    out->allowed_mods = entry.allowed_mods;
    out->options = entry.options;

    return 0;
}

static void reload_alt_repeat_key(void) {
    for (size_t i = 0; i < VIAL_ALT_REPEAT_KEY_ENTRIES; ++i) {
        vial_get_alt_repeat_key(i, &vial_alt_repeat_key[i]);
    }
}

uint16_t alt_repeat_key_count(void) {
    return VIAL_ALT_REPEAT_KEY_ENTRIES;
}

static bool alt_repeat_key_mods_match(uint8_t mods, uint8_t required_mods, uint8_t allowed_mods, uint8_t options) {
    allowed_mods |= required_mods; // Required mods, if any, are allowed.

    // If ignoring mod handedness, bitwise-or low (lhs) 4 bits with upper (rhs) 4 bits.
    if ((options & vial_arep_option_ignore_mod_handedness)) {
        mods = (mods & 0xf) | (mods >> 4);
        required_mods = (required_mods & 0xf) | (required_mods >> 4);
        allowed_mods = (allowed_mods & 0xf) | (allowed_mods >> 4);
    }

    // Check that all required mods are set and all disallowed mods are unset.
    return (mods & required_mods) == required_mods && (mods & ~allowed_mods) == 0;
}

uint16_t get_alt_repeat_key_keycode_user(uint16_t keycode, uint8_t mods) {
    uint16_t alt_keycode = KC_TRNS;
    int8_t best_fit = -1;

    keycode = alt_repeat_key_normalize_keycode(keycode, &mods);

    for (size_t i = 0; i < VIAL_ALT_REPEAT_KEY_ENTRIES; ++i) {
        const alt_repeat_key_t* entry = &vial_alt_repeat_key[i];
        const uint8_t options = entry->options;
        if (!(options & vial_arep_enabled)) { // Skip disabled entries.
            continue;
        }

        // Search for an entry with matching keycode and mods. If there is more
        // than one match, the entry with the most mods wins.
        if (entry->keycode == keycode &&
                alt_repeat_key_mods_match(mods, entry->required_mods, entry->allowed_mods, options)) {
            const int8_t fit = bitpop(entry->required_mods);
            if (fit > best_fit) {
                alt_keycode = (entry->alt_required_mods << 8) | entry->alt_keycode;
                best_fit = fit;
            }
        }

        // If the entry is bidirectional, check for match with the alt keycode.
        if (entry->alt_keycode == keycode &&
                (options & vial_arep_option_bidirectional) != 0 &&
                alt_repeat_key_mods_match(mods, entry->alt_required_mods, entry->allowed_mods, options)) {
            const int8_t fit = bitpop(entry->alt_required_mods);
            if (fit > best_fit) {
                alt_keycode = (entry->required_mods << 8) | entry->keycode;
                best_fit = fit;
            }
        }

        // If this entry is the default alt key and allowed mods are satisfied,
        // use it if no there is no other match.
        if ((options & vial_arep_option_default_to_this_alt_key) != 0 &&
                best_fit == -1 && alt_keycode == KC_TRNS &&
                alt_repeat_key_mods_match(mods, 0, entry->allowed_mods, options)) {
            alt_keycode = (entry->alt_required_mods << 8) | entry->alt_keycode;
        }
    }

    return alt_keycode;
}
#endif
