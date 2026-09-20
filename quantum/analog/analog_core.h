/* Copyright 2026 bvbhu
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
 *      只见 0..ANALOG_MAX_TRAVEL 行程域(满量程 kb 可配，见 §1.5)，top/bottom_reading
 *      是轴体模型在原始 ADC 域的校准锚点。
 *  模型层(analog_model_*.h): absv -> sw 映射，全 weak，默认线性，见 §8。
 *  kb(keyboards/<x>): 采 ADC，扫描里逐键 analog_model_sw() -> analog_step_key()。
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

/* ---- 1.5 行程域与最大键程 ----
 * ANALOG_MAX_TRAVEL 是行程域满量程(0=顶部/释放, M=触底)，kb config.h 覆盖，默认 255。
 * 动态宽度：M<=255 用 uint8(默认板零开销)，否则 uint16。判据取 <=255 而非 <255，
 * 否则默认 255 被判成 uint16、协议包与 EEPROM 无谓膨胀。
 * clamp 必须比较 ANALOG_MAX_TRAVEL 而非类型上限——否则小满量程板放过越界、大满量程板提前截断。 */
#ifndef ANALOG_MAX_TRAVEL
#    define ANALOG_MAX_TRAVEL 255
#endif
_Static_assert(ANALOG_MAX_TRAVEL >= 1 && ANALOG_MAX_TRAVEL <= 65535, "ANALOG_MAX_TRAVEL 必须在 1..65535");

#if ANALOG_MAX_TRAVEL <= 255
typedef uint8_t analog_travel_t;
#    define ANALOG_TRAVEL_WIDE 0 /* 1 = 行程域升为 uint16；结构体尺寸与协议分支据此判断 */
#else
typedef uint16_t analog_travel_t;
#    define ANALOG_TRAVEL_WIDE 1
#endif

/* ---- 2. 每键运行时(默认满量程下恰好 10 字节) ----
 *   [0..3] 配置(行程域 0..ANALOG_MAX_TRAVEL)  [4..7] 校准锚点(原始 ADC)
 *   [8] RT 极值(行程域)        [9] flags
 * 存的就是生效值，状态机直读不回查全局(一致性由 FOLLOW_GLOBAL 级联维护)。
 * flags 排在运行态字段之后，故"结构体前缀==持久化记录"不成立：
 * 将来持久化要显式 packed 记录逐字段搬，禁止 memcpy 本结构。 */
typedef struct {
    analog_travel_t actuation_threshold; /* 下死区界：行程上穿它必定按下 */
    analog_travel_t release_threshold;   /* 上死区界：行程下穿它必定抬起 */
    analog_travel_t actuation_offset;    /* RT 触发距离：未按下时从极值上行超过它即触发 */
    analog_travel_t release_offset;      /* RT 释放距离：已按下时从极值回落超过它即释放 */
    uint16_t bottom_reading;             /* 触底读数 */
    uint16_t top_reading;                /* 静置读数，每次开机重新采样(不持久化) */
    analog_travel_t extremum;            /* RT 极值追踪 */
    uint8_t  flags;                      /* ANALOG_FLAG_* 组合 */
} analog_key_t;

_Static_assert(sizeof(analog_key_t) == (ANALOG_TRAVEL_WIDE ? 16u : 10u), "analog_key_t 尺寸随行程域宽度变化：uint8 域 10 字节 / uint16 域 16 字节");

/* ---- 3. 全局参数 ----  */
typedef struct {
    analog_travel_t actuation_threshold;
    analog_travel_t release_threshold;
    analog_travel_t actuation_offset;
    analog_travel_t release_offset;
    uint8_t rt_enabled; /* 0/1，级联写入跟随全局键的 ANALOG_FLAG_RT_ENABLED */
    /* 补齐到偶数：本结构是持久化区的最后一段，它若是奇数则 ANALOG_PERSIST_SIZE
     * 为奇数，而区首是"从 EEPROM 末尾向前数"(ADDR = TOTAL - SIZE)，区首会跟着
     * 变成奇数地址，8 字节记录就全部落在奇地址上(见 §9 的偶地址约定)。
     * 补 1 字节让总长天然为偶，比事后 &~1 掩码更能自证布局。写盘时恒为 0。 */
    uint8_t reserved;
} analog_global_t;

_Static_assert(sizeof(analog_global_t) == (ANALOG_TRAVEL_WIDE ? 10u : 6u), "analog_global_t 尺寸随行程域宽度变化：uint8 域 6 字节 / uint16 域 10 字节，两者皆偶");

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
analog_travel_t analog_get_tracked_sw(void); /* 未跟踪时读 0 */
int16_t analog_get_tracked_raw(void);     /* raw 走 kb 钩子；未跟踪或不可用读 -1 */

/* ---- 5.5 触底校准模式(运行态，绝不落盘) ----
 * 开启时扫描侧抑制全部键输出(等效 KC_NO)、状态机不推进，读数照常采样；
 * 扫描侧把"读得比当前 bottom 更深"的值喂给 analog_set_bottom_reading(只推高)，
 * 用户逐个按满每个键即可完成触底校准，关闭即结束。
 * 纯运行态、开机默认关：上一次没关绝不能带到下次启动，否则键盘变砖。
 *
 * 唯一的正常出口是上位机发 0xF4 mode5，故固件侧备了两道兜底：
 *   ① 开机默认关 —— analog_init() 显式复位(冷启动靠 BSS，但复位/STOP 恢复不清 BSS)
 *   ② 无心跳超时自动关 —— analog_task() 按 ANALOG_BOTTOM_OUT_TIMEOUT_MS
 *      (默认 60s，kb config.h 可覆盖)判定 */
extern bool g_analog_bottom_out_mode;
void analog_set_bottom_out_mode(bool on);
bool analog_get_bottom_out_mode(void);

/* 续期心跳(兜底②)：由 vial.c 处理 0xF4 时调用。 */
void analog_bottom_out_heartbeat(void);

/* ---- 6. 编译期出厂默认值(kb config.h 覆盖) ----
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
/* 静置/触底原始 ADC 出厂锚点(板相关物理量，无通用默认值，必须由 kb config.h 定义)。
 * 直接用漂移区间内边宏作出厂锚点：出厂态取最小跨度(=> 最大 D)，与生成器选
 * ISF_SCALE 的最坏角点对齐。线性模型板没有漂移区间，这两个宏就是它的出厂锚点。
 *   ANALOG_TOPREADING_MAX     出厂静置锚点(漂移区间上边)
 *   ANALOG_BOTTOMREADING_MIN  出厂触底锚点(漂移区间下边) */
#if !defined(ANALOG_TOPREADING_MAX) || !defined(ANALOG_BOTTOMREADING_MIN)
#    error "kb config.h 必须定义 ANALOG_TOPREADING_MAX 与 ANALOG_BOTTOMREADING_MIN（静置/触底原始 ADC 出厂锚点）"
#endif
/* 出厂：跟随全局、RT 关闭。全局 rt_enabled 由 FLAGS 推导，避免两处真相。 */
#ifndef ANALOG_DEFAULT_FLAGS
#    define ANALOG_DEFAULT_FLAGS ANALOG_FLAG_FOLLOW_GLOBAL
#endif
#define ANALOG_DEFAULT_RT_ENABLED ((ANALOG_DEFAULT_FLAGS & ANALOG_FLAG_RT_ENABLED) ? 1 : 0)

/* 出厂阈值必须落在行程域内：满量程被调小时(如 ANALOG_MAX_TRAVEL=100)若忘了同步
 * 改这几个默认值，出厂态就是"阈值高于满量程"——键永远触发不了，且因为
 * ANALOG_DEFAULT_* 都是编译期常量，这种错配只能在编译期抓。 */
_Static_assert(ANALOG_DEFAULT_ACTUATION_THRESHOLD <= ANALOG_MAX_TRAVEL, "出厂触发阈值超出 ANALOG_MAX_TRAVEL");
_Static_assert(ANALOG_DEFAULT_RELEASE_THRESHOLD <= ANALOG_MAX_TRAVEL, "出厂断开阈值超出 ANALOG_MAX_TRAVEL");
_Static_assert(ANALOG_DEFAULT_ACTUATION_OFFSET <= ANALOG_MAX_TRAVEL, "出厂 RT 触发距离超出 ANALOG_MAX_TRAVEL");
_Static_assert(ANALOG_DEFAULT_RELEASE_OFFSET <= ANALOG_MAX_TRAVEL, "出厂 RT 释放距离超出 ANALOG_MAX_TRAVEL");
/* 触底必须比静置更深：clamp 边界 top ≤ TOPREADING_MAX 与 bottom ≥ BOTTOMREADING_MIN
 * 只有在 BOTTOMREADING_MIN > TOPREADING_MAX 时才保证 top < bottom。模型层的派生
 * 参数量级断言(analog_model_isf.c)也用这条来保证它的分母为正。 */
_Static_assert(ANALOG_BOTTOMREADING_MIN > ANALOG_TOPREADING_MAX, "出厂锚点必须 bottom > top，否则未校准态就倒挂");

/* ---- 6.5 原始读数域(ADC 位宽与钳位上界) ----
 * 与具体键程模型无关，故放核心层；模型层(analog_model_*.c)直接用。 */

/* ADC 位宽：从 QMK 的 ADC_RESOLUTION 推导，**不接受 kb 另设参数**。
 *
 * 该宏的默认值在 QMK 里是 10 位(platforms/chibios/drivers/analog.c:125 的
 * `#ifndef ADC_RESOLUTION`，两个分支都是 *_10BIT)。但那是 .c 私有的、头文件看不到，
 * 故本头在 kb 没定义时按**同样的 10 位**兜底 —— 与 QMK 自身默认保持一致，
 * 不按某块现成板子的取值来定。
 *
 * ⚠️ 必须先判 defined 再比较：`#if` 里未定义的标识符取 0，于是
 * `ADC_RESOLUTION == ADC_CFGR1_RES_12BIT` 在两者都未定义时是 0==0 → **静默为真**，
 * 会绕过下面那条报错(看起来"推导成功"，其实什么都没推导)。 */
#if !defined(ADC_RESOLUTION)
#    define ANALOG_ADC_BITS 10 /* 与 QMK 的 ADC_RESOLUTION 默认值一致 */
#elif ADC_RESOLUTION == ADC_CFGR1_RES_12BIT || ADC_RESOLUTION == 12
#    define ANALOG_ADC_BITS 12
#elif ADC_RESOLUTION == ADC_CFGR1_RES_10BIT || ADC_RESOLUTION == 10
#    define ANALOG_ADC_BITS 10
#elif ADC_RESOLUTION == ADC_CFGR1_RES_8BIT || ADC_RESOLUTION == 8
#    define ANALOG_ADC_BITS 8
#elif ADC_RESOLUTION == ADC_CFGR1_RES_6BIT || ADC_RESOLUTION == 6
#    define ANALOG_ADC_BITS 6
#else
#    error "无法由 QMK 的 ADC_RESOLUTION 判定 ADC 位宽(仅支持 12/10/8/6 位)"
#endif
_Static_assert(ANALOG_ADC_BITS >= 6 && ANALOG_ADC_BITS <= 12, "ANALOG_ADC_BITS 应落在 6..12");

/* 读数钳位上界：缺省取 ADC 满量程*/
#ifndef ANALOG_BOTTOMREADING_MAX
#    define ANALOG_BOTTOMREADING_MAX ((1u << ANALOG_ADC_BITS) - 1u)
#endif
_Static_assert(ANALOG_BOTTOMREADING_MAX >= 1u && ANALOG_BOTTOMREADING_MAX <= 65535u, "ANALOG_BOTTOMREADING_MAX 必须在 1..65535");

/* 查表忽略 absv 的低几位(ISF 模型用，每格 2^N 个读数)。放 core 层仅为让构建期
 * 提取器(util/analog_isf_extract.py)能扫到 #ifndef 兜底；线性模型不读它。
 * 范围 0..3：过大会让表格数过少、曲线失真，无意义。 */
#ifndef ANALOG_ISF_IGNORE_BITS
#    define ANALOG_ISF_IGNORE_BITS 1
#endif
_Static_assert(ANALOG_ISF_IGNORE_BITS >= 0 && ANALOG_ISF_IGNORE_BITS <= 3, "ANALOG_ISF_IGNORE_BITS 应在 0..3");

/* ---- 7. 行为 API ---- */
void analog_init(void); /* 填出厂默认值 + 加载 EEPROM(失败则整区写成合法出厂区) */

/* 推模型状态机：kb 扫描逐键调用；返回 true = 按下状态翻转，调用方据此翻矩阵位。 */
bool analog_step_key(uint16_t ki, analog_travel_t sw);
bool analog_get_pressed(uint16_t ki);

/* 校准量写入：都会回调模型层重算派生参数(§8)。 */
void analog_set_top_reading(uint16_t ki, uint16_t value);    /* 启动校准用 */
void analog_set_bottom_reading(uint16_t ki, uint16_t value); /* 触底校准用 */

/* 配置写入：单键写会清 FOLLOW_GLOBAL(转自定义)；
 * analog_set_global 写全局一处并级联刷新所有跟随全局键的 5 项(位本身不动)。 */
void analog_set_key_config(uint16_t ki, const analog_travel_t params[4], bool rt_on);
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
 * kb 扫描的标脏不受影响。用户点 GUI "保存"才发 0xF6 落盘。 */
void analog_set_persist_suppress(bool suppress);

/* 强制释放：只清按下位、不通知模型层、不返回翻转标志。触底校准模式抑制输出用：
 * 模式开启时按住着的键要立即变无效，退出后也不残留"幽灵按下"。 */
void analog_force_release(uint16_t ki);

/* ---- 8. 键程映射模型层标准钩子 ----
 * 实现在 analog_model_*.c(全 weak，各自定义本对钩子)，由 build_vial.mk 按 kb
 * rules.mk 的 ANALOG_MODEL 值选编对应 .c；此处只作声明供 analog_core.c 调用。 */
/* absv: ADC 差值(Hall 0..2047 / EC 0..1023) -> 行程 0..ANALOG_MAX_TRAVEL */
analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv);
void analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom); /* 核心改锚点后回调，重算模型派生参数 */

/* kb 实现：返回该键最近一次真实 ADC 读数(absv)，<0 = 不可用。weak 默认 -1。 */
int16_t analog_backend_get_raw_adc(uint16_t ki);

/* ---- 9. 持久化：EEPROM 布局 ----
 * 只落盘"出厂后会被用户改动且开机重采拿不到"的量：
 *   每键记录 = 4 项配置 + bottom_reading + flags + 补齐位(行程域 uint8 时 8 字节，
 *   升到 uint16 时 12 字节)；top_reading 每次开机重采、extremum 是运行态，都不持久化。
 *
 * 区址由 QMK 的 EEPROM 分配链决定：nvm_dynamic_keymap.c 把动态宏区尾部
 * 让出 ANALOG_PERSIST_SIZE 字节，链的缩让与 analog_core.c 的寻址共用
 * nvm_eeprom_analog_internal.h，kb config.h 无需参与。
 *
 * 偶地址约定：区首是"从 EEPROM 末尾向前数"(ADDR = TOTAL - SIZE)，所以区首的
 * 奇偶完全由 SIZE 决定。SIZE 取偶 => 区首为偶 => 8 字节头之后的每条记录也都
 * 落在偶地址上。本尺寸天然为偶(key 结构不进区；头 8 为偶 + 记录 8/12 为偶 +
 * 全局 6/10 为偶)，由下方静态断言把关，链上无需再套 &~1 掩码。
 *
 * 行程域宽度写进记录尺寸(8 -> 12)会改变 SIZE，而记录里的 record_bytes 与
 * version 一起构成作废判据，故换宽度的板子开机即整区作废重写，不会读到错位的旧数据。 */
#define ANALOG_PERSIST_MAGIC         0x474E4156u /* "VANG"，小端存放 */
/* EEPROM 布局版本(§9)：与 magic / num_keys / record_bytes 构成 persist_load 的
 * 作废判据，版本不符整区重写。 1=初版；2=行程域宽度可变(记录尺寸 8->12)。
 * 改动落盘布局须 bump 本宏。**与 VIAL_ANALOG_PROTOCOL_VERSION(quantum/vial.c，
 * 空口命令线格式版本)语义无关、取值不同，无联动**——改其一不必动另一个。 */
#define ANALOG_PERSIST_VERSION       2u

/* 字段顺序即落盘布局；改动须 bump ANALOG_PERSIST_VERSION。
 * reserved 把记录补齐到偶数字节：记录地址全落在偶地址(FEE 按半字写最优)，
 * 结构体内零对齐填充。 */
typedef struct {
    analog_travel_t actuation_threshold;
    analog_travel_t release_threshold;
    analog_travel_t actuation_offset;
    analog_travel_t release_offset;
    uint16_t bottom_reading;
    uint8_t  flags;
    uint8_t  reserved;
} analog_record_t;

#define ANALOG_PERSIST_RECORD_BYTES ((uint32_t)sizeof(analog_record_t))

_Static_assert(sizeof(analog_record_t) == (ANALOG_TRAVEL_WIDE ? 12u : 8u), "analog_record_t 尺寸随行程域宽度变化且必须无填充：uint8 域 8 字节 / uint16 域 12 字节");

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
 * 每条记录(8 或 12 字节)随之落在偶地址。这条断言是 §9 偶地址约定的唯一守卫。 */
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
