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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================================
 *  Vial Analog Layer —— 核心运行时(推模型)
 *
 *  核心层(quantum/analog): 持每键配置+校准+运行态与全局参数，跑触发/RT 状态机；
 *      只见 0-255 行程域，top/bottom_reading 是轴体模型在原始 ADC 域的校准锚点。
 *  模型层(analog_model_*.h): absv -> sw 映射，全 weak，默认线性，见 §8。
 *  板级(keyboards/<x>): 采 ADC，扫描里逐键 analog_model_sw() -> analog_step_key()。
 * ========================================================================= */

/* ---- 每键标志位(占用原对齐填充字节) ---- */
enum {
    /* RT 主开关：关=纯阈值滞回；开=按方向生效，offset==0 表示该方向不跟踪 */
    ANALOG_FLAG_RT_ENABLED = (1 << 0),
    /* 跟随全局：4 项配置+RT 位恒由全局级联同步；单独写该键配置即清除此位(转自定义)。 */
    ANALOG_FLAG_FOLLOW_GLOBAL = (1 << 1),
    /* 实时上报开关(Vial Analog 协议 0xF2 的 CONTINUOUS 位，GUI 节流用；
     * 核心状态机不读它，仅随 flags 持久化、经 0xF1 原样回报) */
    ANALOG_FLAG_CONTINUOUS = (1 << 2),
    /* bit3..7 预留 */
};

/* ---- 1. 几何 ---- */
#if !defined(MATRIX_ROWS) || !defined(MATRIX_COLS)
#    error "analog.h 需要 MATRIX_ROWS/MATRIX_COLS(由 keyboard.json 生成)；请在包含本头之前先引入 quantum.h 或 matrix.h"
#endif

#define ANALOG_MATRIX_ROWS MATRIX_ROWS
#define ANALOG_MATRIX_COLS MATRIX_COLS
#define ANALOG_NUM_KEYS    (MATRIX_ROWS * MATRIX_COLS)
#define ANALOG_KI(row, col) ((uint16_t)((row) * ANALOG_MATRIX_COLS + (col)))

/* ---- 2. 每键运行时(恰好 10 字节) ----
 *   [0..3] 配置(行程域 0-255)  [4..7] 校准锚点(原始 ADC)
 *   [8] RT 极值(行程域)        [9] flags
 * 存的就是生效值，状态机直读不回查全局(一致性由 FOLLOW_GLOBAL 级联维护)。
 * flags 排在运行态字段之后，故"结构体前缀==持久化记录"不成立：
 * 将来持久化要显式 packed 记录逐字段搬，禁止 memcpy 本结构。 */
typedef struct {
    uint8_t  actuation_threshold; /* 下死区界：行程上穿它必定按下 */
    uint8_t  release_threshold;   /* 上死区界：行程下穿它必定抬起 */
    uint8_t  actuation_offset;    /* RT 触发距离：未按下时从极值上行超过它即触发 */
    uint8_t  release_offset;      /* RT 释放距离：已按下时从极值回落超过它即释放 */
    uint16_t bottom_reading;      /* 触底读数 */
    uint16_t top_reading;         /* 静置读数，每次开机重新采样(不持久化) */
    uint8_t  extremum;            /* RT 极值追踪 */
    uint8_t  flags;               /* ANALOG_FLAG_* 组合 */
} analog_key_t;

_Static_assert(sizeof(analog_key_t) == 10, "analog_key_t must be exactly 10 bytes");

/* ---- 3. 全局参数 ----  */
typedef struct {
    uint8_t actuation_threshold;
    uint8_t release_threshold;
    uint8_t actuation_offset;
    uint8_t release_offset;
    uint8_t rt_enabled; /* 0/1，级联写入跟随全局键的 ANALOG_FLAG_RT_ENABLED */
    /* 补齐到偶数：本结构是持久化区的最后一段，它若是奇数则 ANALOG_PERSIST_SIZE
     * 为奇数，而区首是"从 EEPROM 末尾向前数"(ADDR = TOTAL - SIZE)，区首会跟着
     * 变成奇数地址，8 字节记录就全部落在奇地址上(见 §9 的偶地址约定)。
     * 补 1 字节让总长天然为偶，比事后 &~1 掩码更能自证布局。写盘时恒为 0。 */
    uint8_t reserved;
} analog_global_t;

_Static_assert(sizeof(analog_global_t) == 6, "analog_global_t must be exactly 6 bytes (even, keeps ANALOG_PERSIST_SIZE even)");

/* ---- 4. 实例与访问宏 ---- */
extern analog_key_t    g_analog_key[ANALOG_NUM_KEYS];
extern analog_global_t g_analog_global;

#define ANALOG_PRESSED_WORDS ((ANALOG_NUM_KEYS + 7) / 8)
extern uint8_t g_analog_pressed_bits[ANALOG_PRESSED_WORDS]; /* 每键 1 bit，状态机的内部记忆 */

#define ANALOG_ACTUATION_THRESHOLD(r, c) (g_analog_key[ANALOG_KI(r, c)].actuation_threshold)
#define ANALOG_RELEASE_THRESHOLD(r, c)   (g_analog_key[ANALOG_KI(r, c)].release_threshold)
#define ANALOG_ACTUATION_OFFSET(r, c)    (g_analog_key[ANALOG_KI(r, c)].actuation_offset)
#define ANALOG_RELEASE_OFFSET(r, c)      (g_analog_key[ANALOG_KI(r, c)].release_offset)
#define ANALOG_BOTTOM_READING(r, c)      (g_analog_key[ANALOG_KI(r, c)].bottom_reading)
#define ANALOG_TOP_READING(r, c)         (g_analog_key[ANALOG_KI(r, c)].top_reading)
#define ANALOG_EXTREMUM(r, c)            (g_analog_key[ANALOG_KI(r, c)].extremum)
#define ANALOG_FLAGS(r, c)               (g_analog_key[ANALOG_KI(r, c)].flags)

#define ANALOG_RT_ENABLED(r, c)    (ANALOG_FLAGS(r, c) & ANALOG_FLAG_RT_ENABLED)
#define ANALOG_FOLLOW_GLOBAL(r, c) (ANALOG_FLAGS(r, c) & ANALOG_FLAG_FOLLOW_GLOBAL)

/* ---- 5. 实时行程上报：只跟踪单个键 ----
 * 上位机轮询时显式带上被跟踪键号，固件只为其留一份 sw；
 * 省掉旧设计的 sw[96] 常驻数组与全矩阵轮询带宽。 */
#define ANALOG_TRACK_NONE 0xFFFF

extern uint16_t g_analog_tracked_key;

void analog_set_tracked_key(uint16_t ki); /* 越界或 TRACK_NONE 均关闭跟踪 */
uint8_t analog_get_tracked_sw(void);      /* 未跟踪时读 0 */
int16_t analog_get_tracked_raw(void);     /* raw 走板级钩子；未跟踪或不可用读 -1 */

/* ---- 5.5 触底校准模式(运行态，绝不落盘) ----
 * 开启时扫描侧抑制全部键输出(等效 KC_NO)、状态机不推进，读数照常采样；
 * 扫描侧把"读得比当前 bottom 更深"的值喂给 analog_set_bottom_reading(只推高)，
 * 用户逐个按满每个键即可完成触底校准，关闭即结束。
 * 纯运行态、开机默认关：上一次没关绝不能带到下次启动，否则键盘变砖。 */
extern bool g_analog_bottom_out_mode;
void analog_set_bottom_out_mode(bool on);
bool analog_get_bottom_out_mode(void);

/* ---- 6. 编译期出厂默认值(板级 config.h 覆盖) ----
 * TL96MG(Hall) 阈值与 ADC 锚点取自参考实现；RT 默认关=出厂即纯阈值滞回。 */
#ifndef ANALOG_DEFAULT_ACTUATION_THRESHOLD
#    define ANALOG_DEFAULT_ACTUATION_THRESHOLD 200
#endif
#ifndef ANALOG_DEFAULT_RELEASE_THRESHOLD
#    define ANALOG_DEFAULT_RELEASE_THRESHOLD 192
#endif
#ifndef ANALOG_DEFAULT_ACTUATION_OFFSET
#    define ANALOG_DEFAULT_ACTUATION_OFFSET 0
#endif
#ifndef ANALOG_DEFAULT_RELEASE_OFFSET
#    define ANALOG_DEFAULT_RELEASE_OFFSET 0
#endif
#ifndef ANALOG_DEFAULT_TOP_READING
#    define ANALOG_DEFAULT_TOP_READING 375
#endif
#ifndef ANALOG_DEFAULT_BOTTOM_READING
#    define ANALOG_DEFAULT_BOTTOM_READING 675
#endif
/* 出厂：跟随全局、RT 关闭。全局 rt_enabled 由 FLAGS 推导，避免两处真相。 */
#ifndef ANALOG_DEFAULT_FLAGS
#    define ANALOG_DEFAULT_FLAGS ANALOG_FLAG_FOLLOW_GLOBAL
#endif
#define ANALOG_DEFAULT_RT_ENABLED ((ANALOG_DEFAULT_FLAGS & ANALOG_FLAG_RT_ENABLED) ? 1 : 0)

/* ---- 7. 行为 API ---- */
void analog_init(void); /* 填出厂默认值；(暂缓)加载 EEPROM */

/* 推模型状态机：板级扫描逐键调用；返回 true = 按下状态翻转，调用方据此翻矩阵位。 */
bool analog_step_key(uint16_t ki, uint8_t sw);
bool analog_get_pressed(uint16_t ki);

/* 校准量写入：都会回调模型层重算派生参数(§8)。 */
void analog_set_top_reading(uint16_t ki, uint16_t value);    /* 启动校准用 */
void analog_set_bottom_reading(uint16_t ki, uint16_t value); /* 触底校准用 */

/* 配置写入：单键写会清 FOLLOW_GLOBAL(转自定义)；
 * analog_set_global 写全局一处并级联刷新所有跟随全局键的 5 项(位本身不动)。 */
void analog_set_key_config(uint16_t ki, const uint8_t params[4], bool rt_on);
void analog_set_global(const analog_global_t *g);

/* 复位：单键=拷回全局并重新跟随(校准不动)；ki==0xFFFF=全部回出厂并逐键通知模型。 */
bool analog_reset_key(uint16_t ki);
bool analog_key_is_customized(uint16_t ki); /* <=> FOLLOW_GLOBAL 未置位，无需按值比对 */

/* 标脏：调用方直接改过 g_analog_key[ki].flags 后告知核心，由 analog_task 防抖统一落盘。
 * 只改运行态(如 extremum)或改锚点请走 analog_set_*_reading，不要用本函数。 */
void analog_mark_dirty(uint16_t ki);

/* 暂缓落盘开关：true 时 persist_mark_* 变 no-op。
 * v3 起 0xF2(调参)仅改 RAM 不落盘——vial_analog_set_wire_config 在处理 0xF2 时
 * set→处理→reset(同步无重入)，故只作用于 0xF2 路径；0xF4 校准 / 0xF5 复位 /
 * 板级扫描的标脏不受影响。用户点 GUI "保存"才发 0xF6 落盘。 */
void analog_set_persist_suppress(bool suppress);

/* 强制释放：只清按下位、不通知模型层、不返回翻转标志。触底校准模式抑制输出用：
 * 模式开启时按住着的键要立即变无效，退出后也不残留"幽灵按下"。 */
void analog_force_release(uint16_t ki);

/* ---- 8. 键程映射模型层标准钩子 ----
 * 实现在 analog_model_*.h(全 weak)，由 analog_core.c 按 config.h 的 #define 选编，
 * 与构建系统无关；未被任何模型实现的钩子走默认线性，规则见 analog_model.h。 */
uint8_t analog_model_sw(uint16_t ki, uint16_t absv); /* absv: ADC 差值(Hall 0..2047 / EC 0..1023) -> sw 0-255 */
void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom); /* 核心改锚点后回调，重算模型派生参数 */

/* 板级实现：返回该键最近一次真实 ADC 读数(absv)，<0 = 不可用。weak 默认 -1。 */
int16_t analog_backend_get_raw_adc(uint16_t ki);

/* ---- 9. 持久化：EEPROM 布局 ----
 * 只落盘"出厂后会被用户改动且开机重采拿不到"的量：
 *   每键 8 字节 = 4 项配置 + bottom_reading + flags + 补齐位；
 *   top_reading 每次开机重采、extremum 是运行态，都不持久化。
 *
 * 区址由 QMK 的 EEPROM 分配链决定：nvm_dynamic_keymap.c 把动态宏区尾部
 * 让出 ANALOG_PERSIST_SIZE 字节，链的缩让与 analog_core.c 的寻址共用
 * nvm_eeprom_analog_internal.h，板级 config.h 无需参与。
 *
 * 偶地址约定：区首是"从 EEPROM 末尾向前数"(ADDR = TOTAL - SIZE)，所以区首的
 * 奇偶完全由 SIZE 决定。SIZE 取偶 => 区首为偶 => 8 字节头之后的每条记录也都
 * 落在偶地址上。本尺寸天然为偶(10 字节 key 不进区；8 头 + 8n 记录 + 6 全局)，
 * 由下方静态断言把关，链上无需再套 &~1 掩码。 */
#define ANALOG_PERSIST_MAGIC         0x474E4156u /* "VANG"，小端存放 */
#define ANALOG_PERSIST_VERSION       1u
#define ANALOG_PERSIST_RECORD_BYTES  8u

/* 字段顺序即落盘布局；改动须 bump ANALOG_PERSIST_VERSION。
 * reserved 把记录补齐到 8 字节：记录地址全落在偶地址(FEE 按半字写最优)，
 * 结构体内零对齐填充。 */
typedef struct {
    uint8_t  actuation_threshold;
    uint8_t  release_threshold;
    uint8_t  actuation_offset;
    uint8_t  release_offset;
    uint16_t bottom_reading;
    uint8_t  flags;
    uint8_t  reserved;
} analog_record_t;

_Static_assert(sizeof(analog_record_t) == ANALOG_PERSIST_RECORD_BYTES, "analog_record_t 必须恰好 8 字节(无填充)，否则持久化布局错位");

/* 8 字节头：magic + version/键数/记录字节数 + checksum(数据段逐字节 XOR，不符整区作废) */
typedef struct {
    uint32_t magic;
    uint8_t  version;
    uint8_t  num_keys;
    uint8_t  record_bytes;
    uint8_t  checksum;
} analog_persist_header_t;

_Static_assert(sizeof(analog_persist_header_t) == 8, "analog_persist_header_t 必须恰好 8 字节");

#define ANALOG_PERSIST_HEADER_BYTES sizeof(analog_persist_header_t)
#define ANALOG_PERSIST_SIZE (ANALOG_PERSIST_HEADER_BYTES + ANALOG_NUM_KEYS * ANALOG_PERSIST_RECORD_BYTES + sizeof(analog_global_t))

/* 布局自证：三段的字节数都是偶数，故总长必为偶 => 区首(末尾向前数)为偶地址，
 * 每条 8 字节记录随之落在偶地址。这条断言是 §9 偶地址约定的唯一守卫。 */
_Static_assert((ANALOG_PERSIST_HEADER_BYTES % 2) == 0, "头必须为偶字节");
_Static_assert((ANALOG_PERSIST_RECORD_BYTES % 2) == 0, "记录必须为偶字节");
_Static_assert((sizeof(analog_global_t) % 2) == 0, "全局段必须为偶字节");
_Static_assert((ANALOG_PERSIST_SIZE % 2) == 0, "ANALOG_PERSIST_SIZE 必须为偶：区首从 EEPROM 末尾向前数，奇数会让所有记录落到奇地址");

/* 周期落盘入口：housekeeping_task 每循环调用，脏标记 + 防抖后一次性提交。
 * 扫描中的实时校准会反复推高 bottom_reading，逐次写会撑爆 FEE 写日志。 */
void analog_task(void);

/* 显式保存(0xF6)：全量落盘当前 RAM 状态(配置+锚点+全局+头校验和)。
 * 与 analog_task 的脏标记防抖增量提交不同：本函数一次性写全部记录，
 * 供 GUI "保存"按钮把 0xF2 调参期间只改了 RAM 的阈值真正写入 EEPROM。 */
void analog_persist_commit(void);
