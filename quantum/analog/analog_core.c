/* Copyright 2026 vial-qmk-wireless contributors
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

/* 注意：MATRIX_ROWS/MATRIX_COLS 必须在包含 analog_core.h 之前就可见，
 * 否则结构尺寸会静默地依赖包含顺序。 */
#include "matrix.h"
#include "analog_core.h"

#include <string.h>

#include "eeprom.h" /* TOTAL_EEPROM_BYTE_COUNT / eeprom_update_* (§9 持久化) */
#include "timer.h"  /* timer_read32：落盘防抖计时 */

#include "analog_model.h" /* 键程映射模型 */

/* =========================================================================
 *  持久化(§9)：区址由 QMK 的 EEPROM 分配链决定，板级 config.h 不参与
 *
 *  nvm_dynamic_keymap.c 把动态宏区尾部让出 VIAL_ANALOG_EEPROM_SIZE
 *  (= ANALOG_PERSIST_SIZE 取偶，随矩阵尺寸自动推导) 字节；分配链的缩让
 *  与本文件的寻址共用 nvm_eeprom_analog_internal.h，没有可漂移的手写常量。
 * ========================================================================= */
#include "../nvm/eeprom/nvm_eeprom_analog_internal.h"

#define ANALOG_EEPROM_BASE VIAL_ANALOG_EEPROM_ADDR

_Static_assert((uint32_t)ANALOG_EEPROM_BASE + (uint32_t)ANALOG_PERSIST_SIZE <= (uint32_t)TOTAL_EEPROM_BYTE_COUNT, "analog 持久化区越界");
_Static_assert(ANALOG_NUM_KEYS <= 255, "持久化头部 num_keys 是单字节");

#define ANALOG_PERSIST_REC_BASE(ki) \
    ((uint32_t)ANALOG_EEPROM_BASE + ANALOG_PERSIST_HEADER_BYTES + (uint32_t)(ki) * ANALOG_PERSIST_RECORD_BYTES)
#define ANALOG_PERSIST_GLOBAL_BASE \
    ((uint32_t)ANALOG_EEPROM_BASE + ANALOG_PERSIST_HEADER_BYTES + (uint32_t)ANALOG_NUM_KEYS * ANALOG_PERSIST_RECORD_BYTES)

#ifndef ANALOG_PERSIST_FLUSH_MS
#    define ANALOG_PERSIST_FLUSH_MS 500u
#endif

/* 校准锚点安全边界(校准不变量) ----
 * 出厂锚点是"可信最坏范围"的边界：静置读数只允许比 ANALOG_DEFAULT_TOP_READING
 * 更小(更松弛)，触底读数只允许比 ANALOG_DEFAULT_BOTTOM_READING 更大(更深)，
 * 边界外再留 ANALOG_CAL_GUARD 计数的噪声带。越界值一律视为噪声、回落到边界。
 * ANALOG_CAL_GUARD 定义在 analog_core.h(模型层要用它推派生参数量级)。
 *
 * 最典型的越界来源是"触底校准时有个别键没按"——那批键读到的是静置值，若收下会把
 * bottom 压到 top 附近甚至倒挂，直接造成误触发。clamp 之后恒有
 *   ANALOG_CAL_GUARD <= top_reading <= DEFAULT_TOP  - GUARD
 *   bottom_reading >= DEFAULT_BOTTOM + GUARD
 *
 * 注意**出厂默认锚点(375/675)不在 clamp 域内**：域是 [GUARD, DEFAULT_TOP-GUARD] 与
 * [DEFAULT_BOTTOM+GUARD, ∞)，默认值由 fill_defaults() 直写、不经 clamp。所以
 * "top < bottom" 由 DEFAULT_BOTTOM > DEFAULT_TOP 保证，而不是由 clamp 保证；
 * 用 clamp 的界去推导依赖锚点跨度的量(如 ISF 的 D)时必须改用 DEFAULT_* 原始值。
 *
 * 另一处易混：**ANALOG_PERSIST_VERSION(本文件 §9，EEPROM 布局版本) 与
 * VIAL_ANALOG_PROTOCOL_VERSION(quantum/vial.c，线路协议版本) 语义无关、取值不同**，
 * 改动其一不必动另一个。 */

/* 噪声带必须比边界窄：否则 clamp_top_reading 的下界会高于上界，两条边界打架。
 * 这是 ANALOG_CAL_GUARD 唯一的结构性约束，编译期抓比运行期出怪值好。 */
_Static_assert((uint32_t)ANALOG_DEFAULT_TOP_READING >= 2u * (uint32_t)ANALOG_CAL_GUARD, "ANALOG_CAL_GUARD 过大：DEFAULT_TOP - GUARD < GUARD，顶部噪声带上下界会倒挂");

static uint16_t clamp_top_reading(uint16_t value) {
    /* 下界：0 或极小值不是"可信的静置读数"，而是空键/未接轴体。收下会让模型层
     * 拿到 top=0 —— analog_backend_calibration_changed 会因 top==0 提前返回，
     * 派生参数 D/K 停在上一次的值，而 mapping 却按新 top 判定，两者失配。
     * 与上界同理：越界值一律视为噪声、回落到边界。 */
    if (value < (uint16_t)ANALOG_CAL_GUARD) value = (uint16_t)ANALOG_CAL_GUARD;
    uint16_t limit = (ANALOG_DEFAULT_TOP_READING > (uint16_t)ANALOG_CAL_GUARD)
                         ? (uint16_t)(ANALOG_DEFAULT_TOP_READING - (uint16_t)ANALOG_CAL_GUARD)
                         : 0;
    return (value <= limit) ? value : limit;
}

static uint16_t clamp_bottom_reading(uint16_t value) {
    uint16_t limit = (uint16_t)((uint32_t)ANALOG_DEFAULT_BOTTOM_READING + (uint32_t)ANALOG_CAL_GUARD);
    return (value >= limit) ? value : limit;
}

/* 行程域饱和：把任意宽度的入参收进 0..ANALOG_MAX_TRAVEL。
 * 必须比较 ANALOG_MAX_TRAVEL 而非 analog_travel_t 的最大值——满量程小于类型
 * 上限时(如满量程 300 而类型是 uint16)，类型本身拦不住越界值，而越界阈值会让
 * 键永远触发不了(sw > act 恒不成立)。EEPROM 里的旧值、GUI 传来的值都过这里。 */
static inline analog_travel_t clamp_travel(uint32_t value) {
    return (value > (uint32_t)ANALOG_MAX_TRAVEL) ? (analog_travel_t)ANALOG_MAX_TRAVEL : (analog_travel_t)value;
}

/* 脏标记：只重写改过的记录，eeprom_update_block 的"读-比-写"才不会白费。
 * 尺寸与按下位图(analog_core.h 的 ANALOG_PRESSED_WORDS)同口径，两处都按 (N+7)/8 取。 */
static uint8_t  g_persist_dirty[(ANALOG_NUM_KEYS + 7) / 8];
static bool     g_persist_dirty_global = false;
static uint32_t g_persist_last_flush   = 0;

/* 0xF2 调参期间设 true：persist_mark_* 变 no-op，故 0xF2 只改 RAM、不落盘 EEPROM。
 * 由 vial_analog_set_wire_config 在处理 0xF2 命令时 set→处理→reset(同步无重入)，
 * 仅作用于 0xF2 路径；0xF4 校准 / 0xF5 复位 / 板级扫描不经过该层，标脏照常。
 *
 * ⚠️ 抑制期被吞掉的标脏记在 g_persist_shadow 里，解除抑制时一次性补标。
 * 这不是优化而是**正确性要求**：头校验和 persist_checksum_ram() 是按 RAM 全量算的，
 * 而 persist_flush_dirty() 只写脏项。若某键 RAM 已变却不标脏，则任何**其它**脏项触发的
 * 增量落盘都会写出一份"校验和按新 RAM、数据段却还是旧值"的区 —— 下次开机校验必然失败，
 * 整区被出厂默认覆盖。抑制期改的值必须最终进脏位图，才能维持
 * "非脏键：EEPROM == RAM" 这个不变量。 */
static bool g_persist_suppress = false;
static uint8_t g_persist_shadow[(ANALOG_NUM_KEYS + 7) / 8];
static bool    g_persist_shadow_global = false;

static void persist_mark_key(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) return;
    if (g_persist_suppress) {
        g_persist_shadow[ki >> 3] |= (uint8_t)(1u << (ki & 7)); /* 记影子，解除抑制时补标 */
        return;
    }
    g_persist_dirty[ki >> 3] |= (uint8_t)(1u << (ki & 7));
}

static void persist_mark_global(void) {
    if (g_persist_suppress) {
        g_persist_shadow_global = true;
        return;
    }
    g_persist_dirty_global = true;
}

/* RAM -> 记录布局。字段顺序即落盘布局，逐字段显式搬，禁止 memcpy 键结构体。 */
static void record_of(uint16_t ki, analog_record_t *out) {
    const analog_key_t *k    = &g_analog_key[ki];
    out->actuation_threshold = k->actuation_threshold;
    out->release_threshold   = k->release_threshold;
    out->actuation_offset    = k->actuation_offset;
    out->release_offset      = k->release_offset;
    out->bottom_reading      = k->bottom_reading;
    out->flags               = k->flags;
}

/* 数据段 = 全部记录 + 全局段，起点在头之后。必须与 persist_checksum_ram 逐个
 * 字节一一对应，否则 persist_load 的校验永远不过、每次开机都白刷一遍 EEPROM。 */
#define ANALOG_PERSIST_DATA_BASE ((uint32_t)ANALOG_EEPROM_BASE + ANALOG_PERSIST_HEADER_BYTES)
#define ANALOG_PERSIST_DATA_BYTES ((uint32_t)ANALOG_NUM_KEYS * ANALOG_PERSIST_RECORD_BYTES + (uint32_t)sizeof(analog_global_t))

static uint8_t persist_checksum_ram(void) {
    uint8_t x = 0;
    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_record_t r;
        record_of(i, &r);
        const uint8_t *p = (const uint8_t *)&r;
        for (uint16_t j = 0; j < sizeof(r); j++) x ^= p[j];
    }
    const uint8_t *gp = (const uint8_t *)&g_analog_global;
    for (uint16_t j = 0; j < sizeof(g_analog_global); j++) x ^= gp[j];
    return x;
}

static uint8_t persist_checksum_eeprom(void) {
    uint8_t x = 0;
    for (uint32_t i = 0; i < ANALOG_PERSIST_DATA_BYTES; i++) {
        x ^= eeprom_read_byte((uint8_t *)(uintptr_t)(ANALOG_PERSIST_DATA_BASE + i));
    }
    return x;
}

static void persist_write_header(void) {
    analog_persist_header_t h;
    memset(&h, 0, sizeof(h));
    h.magic        = ANALOG_PERSIST_MAGIC;
    h.version      = ANALOG_PERSIST_VERSION;
    h.num_keys     = (uint8_t)ANALOG_NUM_KEYS;
    h.record_bytes = ANALOG_PERSIST_RECORD_BYTES;
    h.checksum     = persist_checksum_ram();
    eeprom_update_block(&h, (void *)(uintptr_t)ANALOG_EEPROM_BASE, sizeof(h));
}

/* 全量提交：出厂重置 / 首次上机 / 校验失败时把当前状态写成合法区 */
static void persist_flush_all(void) {
    eeprom_update_block(&g_analog_global, (void *)(uintptr_t)ANALOG_PERSIST_GLOBAL_BASE, sizeof(g_analog_global));
    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_record_t r;
        record_of(i, &r);
        eeprom_update_block(&r, (void *)(uintptr_t)ANALOG_PERSIST_REC_BASE(i), sizeof(r));
    }
    memset(g_persist_dirty, 0, sizeof(g_persist_dirty));
    memset(g_persist_shadow, 0, sizeof(g_persist_shadow)); /* 全区已落盘，影子位一并作废 */
    g_persist_dirty_global  = false;
    g_persist_shadow_global = false;
    persist_write_header(); /* 校验和盖在数据段上，必须最后写 */
    g_persist_last_flush    = timer_read32();
}

static void persist_flush_dirty(void) {
    if (g_persist_dirty_global) {
        eeprom_update_block(&g_analog_global, (void *)(uintptr_t)ANALOG_PERSIST_GLOBAL_BASE, sizeof(g_analog_global));
        g_persist_dirty_global = false;
    }
    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        if (!(g_persist_dirty[i >> 3] & (uint8_t)(1u << (i & 7)))) continue;
        analog_record_t r;
        record_of(i, &r);
        eeprom_update_block(&r, (void *)(uintptr_t)ANALOG_PERSIST_REC_BASE(i), sizeof(r));
        g_persist_dirty[i >> 3] &= (uint8_t)~(uint8_t)(1u << (i & 7));
    }
    persist_write_header();
    g_persist_last_flush = timer_read32();
}

/* 加载：头部不合法(首次上机/布局变更/版本不符)或校验不过 => 返回 false 整区作废。
 * 跟随全局键的 4 项配置不占冗余存储，加载时从全局记录推导。 */
static bool persist_load(void) {
    analog_persist_header_t h;
    eeprom_read_block(&h, (const void *)(uintptr_t)ANALOG_EEPROM_BASE, sizeof(h));
    if (h.magic != ANALOG_PERSIST_MAGIC || h.version != ANALOG_PERSIST_VERSION || h.num_keys != (uint8_t)ANALOG_NUM_KEYS || h.record_bytes != ANALOG_PERSIST_RECORD_BYTES) return false;
    if (persist_checksum_eeprom() != h.checksum) return false;

    eeprom_read_block(&g_analog_global, (const void *)(uintptr_t)ANALOG_PERSIST_GLOBAL_BASE, sizeof(g_analog_global));
    /* 全局段过 clamp：满量程被调小的板子(如 1023 -> 300)记录尺寸不变，整区不会作废，
     * 会原样读回"旧满量程下写的"阈值；直接沿用会让跟随全局的键永远触发不了。 */
    g_analog_global.actuation_threshold = clamp_travel(g_analog_global.actuation_threshold);
    g_analog_global.release_threshold   = clamp_travel(g_analog_global.release_threshold);
    g_analog_global.actuation_offset    = clamp_travel(g_analog_global.actuation_offset);
    g_analog_global.release_offset      = clamp_travel(g_analog_global.release_offset);

    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_record_t r;
        eeprom_read_block(&r, (const void *)(uintptr_t)ANALOG_PERSIST_REC_BASE(i), sizeof(r));

        analog_key_t *k = &g_analog_key[i];
        if (r.flags & ANALOG_FLAG_FOLLOW_GLOBAL) {
            k->actuation_threshold = g_analog_global.actuation_threshold;
            k->release_threshold   = g_analog_global.release_threshold;
            k->actuation_offset    = g_analog_global.actuation_offset;
            k->release_offset      = g_analog_global.release_offset;
        } else {
            k->actuation_threshold = clamp_travel(r.actuation_threshold);
            k->release_threshold   = clamp_travel(r.release_threshold);
            k->actuation_offset    = clamp_travel(r.actuation_offset);
            k->release_offset      = clamp_travel(r.release_offset);
        }
        k->bottom_reading = clamp_bottom_reading(r.bottom_reading); /* 旧固件可能写过越界锚点 */
        /* 只收本版本认识的位：EEPROM 里的 bit3..7 按约定恒 0，万一被外部工具写脏
         * 也绝不带进 RAM——否则下次落盘会把脏位原样写回，永久留在区里。 */
        k->flags          = r.flags & (ANALOG_FLAG_RT_ENABLED | ANALOG_FLAG_FOLLOW_GLOBAL | ANALOG_FLAG_CONTINUOUS);
        k->extremum       = k->actuation_threshold;
    }
    return true;
}

void analog_task(void) {
    bool dirty = g_persist_dirty_global;
    for (uint16_t i = 0; i < sizeof(g_persist_dirty) && !dirty; i++) dirty = (g_persist_dirty[i] != 0);
    if (!dirty) return;

    /* 防抖：实时校准在扫描里反复推 bottom_reading，攒到间隔到期一次性提交 */
    if ((uint32_t)(timer_read32() - g_persist_last_flush) < ANALOG_PERSIST_FLUSH_MS) return;

    persist_flush_dirty();
}

/* ---- 实例定义 ---- */
analog_key_t    g_analog_key[ANALOG_NUM_KEYS];
analog_global_t g_analog_global;
uint8_t         g_analog_pressed_bits[ANALOG_PRESSED_WORDS];
uint16_t        g_analog_tracked_key = ANALOG_TRACK_NONE;

/* 只为被跟踪键留一份行程；不常驻 sw[96] */
static analog_travel_t g_tracked_sw = 0;
static bool    g_initialized = false;

/* 板级后端钩子的 weak 默认(模型层钩子在上面的模型头文件里)：
 * 无通用标准 raw-adc 后端，报不了原始读数。 */
__attribute__((weak)) int16_t analog_backend_get_raw_adc(uint16_t ki) {
    (void)ki;
    return -1;
}

/* ---- 按下状态位图 ---- */
static inline bool pressed_get(uint16_t ki) {
    return (g_analog_pressed_bits[ki >> 3] >> (ki & 7)) & 1u;
}

static inline void pressed_set(uint16_t ki, bool value) {
    if (value) {
        g_analog_pressed_bits[ki >> 3] |= (uint8_t)(1u << (ki & 7));
    } else {
        g_analog_pressed_bits[ki >> 3] &= (uint8_t)~(1u << (ki & 7));
    }
}

/* ---- 出厂默认值 ---- */
static void fill_defaults(void) {
    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_key_t *k   = &g_analog_key[i];
        k->actuation_threshold = ANALOG_DEFAULT_ACTUATION_THRESHOLD;
        k->release_threshold   = ANALOG_DEFAULT_RELEASE_THRESHOLD;
        k->actuation_offset    = ANALOG_DEFAULT_ACTUATION_OFFSET;
        k->release_offset      = ANALOG_DEFAULT_RELEASE_OFFSET;
        k->bottom_reading      = ANALOG_DEFAULT_BOTTOM_READING;
        k->top_reading         = ANALOG_DEFAULT_TOP_READING;
        k->extremum            = k->actuation_threshold;
        k->flags               = ANALOG_DEFAULT_FLAGS;
    }
    memset(g_analog_pressed_bits, 0, sizeof(g_analog_pressed_bits));

    g_analog_global.actuation_threshold = ANALOG_DEFAULT_ACTUATION_THRESHOLD;
    g_analog_global.release_threshold   = ANALOG_DEFAULT_RELEASE_THRESHOLD;
    g_analog_global.actuation_offset    = ANALOG_DEFAULT_ACTUATION_OFFSET;
    g_analog_global.release_offset      = ANALOG_DEFAULT_RELEASE_OFFSET;
    g_analog_global.rt_enabled          = ANALOG_DEFAULT_RT_ENABLED;
    /* 补齐位恒为 0：它进校验和也进 EEPROM，留未初始化值会让落盘内容不确定 */
    g_analog_global.reserved            = 0;

    g_analog_tracked_key = ANALOG_TRACK_NONE;
    g_tracked_sw         = 0;
}

void analog_init(void) {
    if (g_initialized) return;
    fill_defaults(); /* 先给全字段(含 top_reading 开机值)一个合法出厂值 */
    if (!persist_load()) persist_flush_all(); /* 首次上机/布局变更/校验失败：写成合法区 */
    /* 锚点已定(出厂或 EEPROM)，重算模型派生参数(ISF 的 D/K 等) */
    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_backend_calibration_changed(i, g_analog_key[i].top_reading, g_analog_key[i].bottom_reading);
    }
    g_persist_last_flush = timer_read32();
    g_initialized        = true;
}

/* ---- 触发判断 ---- */
bool analog_step_key(uint16_t ki, analog_travel_t sw) {
    if (ki >= ANALOG_NUM_KEYS) return false;

    analog_key_t *k = &g_analog_key[ki];

    if (ki == g_analog_tracked_key) g_tracked_sw = sw;

    /* 宽域下 uint8_t 会截断 >255 的阈值 */
    analog_travel_t act = k->actuation_threshold;
    analog_travel_t rel = k->release_threshold;
    if (rel > act) {
        rel = k->actuation_threshold;
        act = k->release_threshold;
    }

    bool pressed = pressed_get(ki);

    /* --- 1) 上死区：强制释放 --- */
    if (sw < rel) {
        if (pressed) {
            k->extremum = sw;
            pressed_set(ki, false);
            return true;
        }
        /* 已在释放态：继续跟踪谷值，理由同下死区 */
        if (sw < k->extremum) k->extremum = sw;
        return false;
    }

    /* --- 2) 下死区：强制触发 --- */
    if (sw > act) {
        if (!pressed) {
            k->extremum = sw;
            pressed_set(ki, true);
            return true;
        }
        /* 已在按下态：仍要继续跟踪峰值。若此处不更新 extremum，峰值会冻结在刚越过
         * act 的一刻，RT 释放判据退化成"自 act 附近回落"而非"自真实峰值回落"
         * (act=200/峰值=255/off=20 时应 235 释放，实际要退到 ~180)，手感变钝。 */
        if (sw > k->extremum) k->extremum = sw;
        return false;
    }

    /* --- 3) [rel, act] 带内：RT 极值跟踪 --- */
    if (!(k->flags & ANALOG_FLAG_RT_ENABLED)) return false;

    if (pressed) {
        int off = (int)k->release_offset; /* 0 => 该方向不跟踪 */
        if (off != 0) {
            if ((int)sw > (int)k->extremum) {
                k->extremum = sw; /* 继续下压：抬高峰值 */
            } else if ((int)sw < (int)k->extremum - off) {
                k->extremum = sw;
                pressed_set(ki, false);
                return true; /* 自峰值回落超过 release_offset：RT 释放 */
            }
        }
    } else {
        int off = (int)k->actuation_offset; /* 0 => 该方向不跟踪 */
        if (off != 0) {
            if ((int)sw < (int)k->extremum) {
                k->extremum = sw; /* 继续抬起：压低谷值 */
            } else if ((int)sw > (int)k->extremum + off) {
                k->extremum = sw;
                pressed_set(ki, true);
                return true; /* 自谷值上行超过 actuation_offset：RT 触发 */
            }
        }
    }

    return false;
}

bool analog_get_pressed(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) return false;
    return pressed_get(ki);
}

/* ---- 校准 ---- */
void analog_set_top_reading(uint16_t ki, uint16_t value) {
    if (ki >= ANALOG_NUM_KEYS) return;
    value = clamp_top_reading(value); /* 越界=噪声，回落到边界(见文件头部说明) */
    if (g_analog_key[ki].top_reading == value) return; /* 未变不打扰模型层 */
    g_analog_key[ki].top_reading = value;
    /* top_reading 不持久化(每次开机重采)，不标脏 */
    analog_backend_calibration_changed(ki, value, g_analog_key[ki].bottom_reading);
}

void analog_set_bottom_reading(uint16_t ki, uint16_t value) {
    if (ki >= ANALOG_NUM_KEYS) return;
    value = clamp_bottom_reading(value);
    if (g_analog_key[ki].bottom_reading == value) return;
    g_analog_key[ki].bottom_reading = value;
    persist_mark_key(ki); /* 扫描里会被实时触底采样反复推高，由 analog_task 防抖统一提交 */
    analog_backend_calibration_changed(ki, g_analog_key[ki].top_reading, value);
}

/* ---- 配置写入与全局级联 ---- */

/* 单键写入即"自定义"：清 FOLLOW_GLOBAL，从此不受全局改动影响。 */
void analog_set_key_config(uint16_t ki, const analog_travel_t params[4], bool rt_on) {
    if (ki >= ANALOG_NUM_KEYS || params == NULL) return;

    analog_key_t *k        = &g_analog_key[ki];
    k->actuation_threshold = clamp_travel(params[0]);
    k->release_threshold   = clamp_travel(params[1]);
    k->actuation_offset    = clamp_travel(params[2]);
    k->release_offset      = clamp_travel(params[3]);
    k->flags               = (uint8_t)((k->flags & ~(uint8_t)(ANALOG_FLAG_RT_ENABLED | ANALOG_FLAG_FOLLOW_GLOBAL)) | (rt_on ? ANALOG_FLAG_RT_ENABLED : 0));
    persist_mark_key(ki);
}

/* 改全局：写全局一处 + 级联刷新跟随键的 5 项(4 配置 + RT 位)；FOLLOW_GLOBAL 位不动。 */
void analog_set_global(const analog_global_t *g) {
    if (g == NULL) return;

    g_analog_global = *g;
    g_analog_global.reserved = 0; /* 调用方的补齐位可能是栈上未初始化值，落盘前抹平 */
    /* 4 项阈值过 clamp 后再级联：级联源必须是"已收进行程域"的值，
     * 否则越界值会被这一处复制到所有跟随全局的键上。 */
    g_analog_global.actuation_threshold = clamp_travel(g_analog_global.actuation_threshold);
    g_analog_global.release_threshold   = clamp_travel(g_analog_global.release_threshold);
    g_analog_global.actuation_offset    = clamp_travel(g_analog_global.actuation_offset);
    g_analog_global.release_offset      = clamp_travel(g_analog_global.release_offset);

    const uint8_t rt_bit = g_analog_global.rt_enabled ? (uint8_t)ANALOG_FLAG_RT_ENABLED : (uint8_t)0;

    for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
        analog_key_t *k = &g_analog_key[i];
        if (!(k->flags & ANALOG_FLAG_FOLLOW_GLOBAL)) continue; /* 自定义键不受影响 */

        k->actuation_threshold = g_analog_global.actuation_threshold;
        k->release_threshold   = g_analog_global.release_threshold;
        k->actuation_offset    = g_analog_global.actuation_offset;
        k->release_offset      = g_analog_global.release_offset;
        k->flags               = (uint8_t)((k->flags & ~(uint8_t)ANALOG_FLAG_RT_ENABLED) | rt_bit);
        /* 跟随键的 RAM 被这里改过，必须逐个标脏。不能只标全局：头校验和是按 RAM 全量
         * 计算的，而 persist_flush_dirty() 只写脏项 —— 漏标会让数据段与校验和失配，
         * 下次开机整区作废。（曾误以为"加载时按全局推导"可以省掉这些记录，但那只
         * 影响读取路径，救不了校验和。） */
        persist_mark_key(i);
    }
    persist_mark_global();
}

/* ---- 复位 ---- */
bool analog_reset_key(uint16_t ki) {
    if (ki == 0xFFFF) {
        /* 出厂重置：连校准锚点一起回编译期默认值，逐键通知模型重算派生参数。 */
        fill_defaults();
        for (uint16_t i = 0; i < ANALOG_NUM_KEYS; i++) {
            analog_backend_calibration_changed(i, g_analog_key[i].top_reading, g_analog_key[i].bottom_reading);
        }
        persist_flush_all(); /* 显式用户动作，立即落盘，不等防抖 */
        return true;
    }

    if (ki >= ANALOG_NUM_KEYS) return false;

    /* 单键复位 = 回全局并重新跟随。校准不动：top 每次开机重采，bottom 是本键物理量。 */
    analog_key_t *k        = &g_analog_key[ki];
    k->actuation_threshold = g_analog_global.actuation_threshold;
    k->release_threshold   = g_analog_global.release_threshold;
    k->actuation_offset    = g_analog_global.actuation_offset;
    k->release_offset      = g_analog_global.release_offset;
    k->flags               = (uint8_t)((k->flags & ~(uint8_t)ANALOG_FLAG_RT_ENABLED) | (g_analog_global.rt_enabled ? (uint8_t)ANALOG_FLAG_RT_ENABLED : (uint8_t)0) | ANALOG_FLAG_FOLLOW_GLOBAL);
    persist_mark_key(ki); /* 记录保留生效值，虽然加载时会按跟随全局推导，仍写回保持一致 */
    return true;
}

bool analog_key_is_customized(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) return false;
    return !(g_analog_key[ki].flags & ANALOG_FLAG_FOLLOW_GLOBAL);
}

/* 协议层(quantum/vial.c)直接改 flags 后标脏用；核心内部改配置仍走各自的 set_* 路径。 */
void analog_mark_dirty(uint16_t ki) {
    persist_mark_key(ki);
}

/* 语义见 g_persist_suppress 定义处的注释（0xF2 调参期间设 true 使标脏变 no-op）。
 *
 * 解除抑制(false)时把抑制期攒下的影子脏位并回真脏位图：这些键的 RAM 已被改过，
 * 必须让它们参与后续落盘，否则头校验和(按 RAM 算)会与数据段(只写脏项)失配，
 * 导致下次开机整区作废。并回不会立刻写 flash —— 仍由 analog_task 的 500ms 防抖
 * 或用户显式 0xF6 决定落盘时机，故 0xF2 拖动期间依然是零写入。 */
void analog_set_persist_suppress(bool suppress) {
    if (g_persist_suppress && !suppress) {
        for (uint16_t i = 0; i < sizeof(g_persist_shadow); i++) {
            g_persist_dirty[i] |= g_persist_shadow[i];
            g_persist_shadow[i] = 0;
        }
        if (g_persist_shadow_global) {
            g_persist_dirty_global   = true;
            g_persist_shadow_global  = false;
        }
    }
    g_persist_suppress = suppress;
}

/* 0xF6 显式保存：全量落盘当前 RAM 状态(全局+全部记录+头校验和)。
 * 与 analog_task 的脏标记防抖增量提交不同：本函数一次性写全部记录，供 GUI "保存"
 * 按钮把 0xF2 调参期间只改了 RAM(未标脏)的阈值真正写入 EEPROM。eeprom_update_block
 * 的读-比-写使值未变的记录不产生实际擦写，故全量提交不额外磨损 EEPROM。 */
void analog_persist_commit(void) {
    persist_flush_all();
}

/* ---- 触底校准模式(§5.5，运行态) ---- */
bool g_analog_bottom_out_mode = false;

void analog_set_bottom_out_mode(bool on) {
    g_analog_bottom_out_mode = on;
    if (on) {
        /* 进入即清全部按下位：模式期间扫描侧清零矩阵位，退出后不残留"幽灵按下"。
         * 不通知模型层(锚点没变)，也不标脏(运行态不落盘)。 */
        memset(g_analog_pressed_bits, 0, sizeof(g_analog_pressed_bits));
    }
}

bool analog_get_bottom_out_mode(void) {
    return g_analog_bottom_out_mode;
}

void analog_force_release(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) return;
    pressed_set(ki, false);
}

/* ---- 单键实时跟踪 ---- */
void analog_set_tracked_key(uint16_t ki) {
    if (ki >= ANALOG_NUM_KEYS) ki = ANALOG_TRACK_NONE;
    if (ki == g_analog_tracked_key) return;

    g_analog_tracked_key = ki;
    g_tracked_sw         = 0; /* 换键即作废旧值，避免上键行程污染新键 */
}

analog_travel_t analog_get_tracked_sw(void) {
    if (g_analog_tracked_key == ANALOG_TRACK_NONE) return 0;
    return g_tracked_sw;
}

int16_t analog_get_tracked_raw(void) {
    if (g_analog_tracked_key == ANALOG_TRACK_NONE) return -1;
    return analog_backend_get_raw_adc(g_analog_tracked_key);
}
