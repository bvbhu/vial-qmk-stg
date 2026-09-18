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

/* stg65_ec：EC 静电容矩阵扫描 + Vial analog 桥接
 *
 * CUSTOM_MATRIX = lite：本文件提供 matrix_init_custom / matrix_scan_custom，
 * matrix_init/scan/get_row/print/debounce 由 quantum/matrix_common.c 提供。
 * 扫描内逐键 analog_model_sw() -> analog_step_key() 推进核心状态机，
 * 返回 true 表示 raw 矩阵有变化。
 */

#include "quantum.h"
#include "analog.h"              /* QMK ADC 驱动：adc_mux / pinToMux / adc_read */
#include "analog/analog_core.h"  /* Vial analog 核心层：推模型状态机 + 持久化 */
#include "matrix.h"
#include "wait.h"
#include "print.h"
#include "atomic_util.h"
#include "bootloader.h"

void bootmagic_reset_eeprom(void);

/* ---- 引脚表(config.h) ---- */
static const pin_t   row_pins[MATRIX_ROWS]      = MATRIX_ROW_PINS;
static const pin_t   amux_en_pins[AMUX_COUNT]   = AMUX_EN_PINS;
static const pin_t   amux_sel_pins[3]           = AMUX_SEL_PINS;
static const uint8_t amux_col_sizes[AMUX_COUNT] = AMUX_COL_CHANNELS_SIZES;
static const uint8_t amux_col_channels[AMUX_COUNT][AMUX_MAX_COLS_COUNT] = { AMUX_COL_CHANNELS };

/* ---- 未使用矩阵位(split 键位，无实体键) ---- */
#ifdef UNUSED_POSITIONS_LIST
static const uint8_t unused_positions[][2] = UNUSED_POSITIONS_LIST;
static bool is_unused_position(uint8_t row, uint8_t col) {
    for (size_t i = 0; i < sizeof(unused_positions) / sizeof(unused_positions[0]); i++) {
        if (unused_positions[i][0] == row && unused_positions[i][1] == col) {
            return true;
        }
    }
    return false;
}
#endif

static adc_mux  ec_adc_mux;                          /* ADC 检测通道(ANALOG_PORT) */
static uint16_t last_absv[MATRIX_ROWS][MATRIX_COLS]; /* 每键最近一次真实 ADC 读数 */

/* ---- 运放使能(OPAMP_EN_PIN，OPAMP_EN_ACTIVE=1 高有效) ----
 * EC 信号经运放放大后送 ANALOG_PORT(A4)；运放关闭则 A4 读数恒为 0。
 * 扫描/采样期间开启、结束关闭以省电(BLE/LPM)。 */
#ifdef OPAMP_EN_PIN
#    if OPAMP_EN_ACTIVE
#        define OPAMP_ENABLE()  gpio_write_pin_high(OPAMP_EN_PIN)
#        define OPAMP_DISABLE() gpio_write_pin_low(OPAMP_EN_PIN)
#    else
#        define OPAMP_ENABLE()  gpio_write_pin_low(OPAMP_EN_PIN)
#        define OPAMP_DISABLE() gpio_write_pin_high(OPAMP_EN_PIN)
#    endif
#else
#    define OPAMP_ENABLE()  (void)0
#    define OPAMP_DISABLE() (void)0
#endif

/* 多路复用器选通：EN 低有效——先禁用全部，写通道地址(低位在前)，再启用目标 mux */
static void select_amux_channel(uint8_t amux, uint8_t col) {
    for (uint8_t i = 0; i < AMUX_COUNT; i++) {
        gpio_write_pin_high(amux_en_pins[i]);
    }
    uint8_t ch = amux_col_channels[amux][col];
    for (uint8_t bit = 0; bit < 3; bit++) {
        if (ch & (1u << bit)) {
            gpio_write_pin_high(amux_sel_pins[bit]);
        } else {
            gpio_write_pin_low(amux_sel_pins[bit]);
        }
    }
    gpio_write_pin_low(amux_en_pins[amux]);
}

/* 单键采样(cipulot EC 时序)：
 * 1) 其余行全拉低防鬼影，当前行先拉低获得干净沿；
 * 2) 放电脚转 Hi-Z 停止泄放 -> 当前行拉高充电 -> 关中断读 ADC；
 * 3) 放电脚拉低(输出)泄放 DISCHARGE_TIME。 */
static uint16_t ec_readkey_raw(uint8_t row, uint8_t amux, uint8_t col) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        gpio_write_pin_low(row_pins[r]);
    }
    select_amux_channel(amux, col);
    (void)adc_read(ec_adc_mux); /* flush：让 ADC 采样保持电容与新通道/行状态对齐 */

    uint16_t absv;
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_input(DISCHARGE_PIN);   /* Hi-Z：停止泄放 */
        gpio_write_pin_high(row_pins[row]);  /* 充电 */
        absv = adc_read(ec_adc_mux);          /* 真实读数 */
    }
    gpio_set_pin_output(DISCHARGE_PIN);
    gpio_write_pin_low(DISCHARGE_PIN);       /* 泄放 */
    wait_us(DISCHARGE_TIME);
    (void)adc_read(ec_adc_mux); /* flush：泄放残余、复位采样保持 */
    return absv;
}

/* GUI 后端钩子：返回该键最近一次扫描的真实 ADC 读数(absv，未经键程映射) */
int16_t analog_backend_get_raw_adc(uint16_t ki) {
    if (ki >= MATRIX_ROWS * MATRIX_COLS) return -1;
    uint8_t row = ki / MATRIX_COLS, col = ki % MATRIX_COLS;
    return (int16_t)last_absv[row][col];
}

void matrix_init_custom(void) {
    /* ADC 检测脚设为模拟输入(STM32 必需，否则读数为 0/噪声) */
    palSetLineMode(ANALOG_PORT, PAL_MODE_INPUT_ANALOG);
    ec_adc_mux = pinToMux(ANALOG_PORT);
    adc_read(ec_adc_mux); /* dummy 读取：确保 adcStart() 已被调用(bootmagic 直读依赖) */

    /* 行引脚：输出低(充电驱动) */
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        gpio_set_pin_output(row_pins[r]);
        gpio_write_pin_low(row_pins[r]);
    }
    /* mux：EN 输出(初始全禁用)、SEL 输出低 */
    for (uint8_t i = 0; i < AMUX_COUNT; i++) {
        gpio_set_pin_output(amux_en_pins[i]);
        gpio_write_pin_high(amux_en_pins[i]);
    }
    for (uint8_t i = 0; i < 3; i++) {
        gpio_set_pin_output(amux_sel_pins[i]);
        gpio_write_pin_low(amux_sel_pins[i]);
    }
    /* 放电脚：输出低(默认泄放) */
    gpio_set_pin_output(DISCHARGE_PIN);
    gpio_write_pin_low(DISCHARGE_PIN);

    /* 运放使能脚：输出，默认关闭(扫描/采样时再开，省电) */
    gpio_set_pin_output(OPAMP_EN_PIN);
    OPAMP_DISABLE();
}

/* 开机校准：采样各键静置读数(噪声基底)写入 top_reading。
 * bottom_reading 出厂默认 900，触底由实时校准推高并防抖落盘。 */
void calibrate_matrix(void) {
    uint16_t accum[MATRIX_ROWS][MATRIX_COLS] = {{0}};
    OPAMP_ENABLE();
    for (volatile uint8_t d = 0; d < MATRIX_COLS; d++) {} /* 空转一轮，等运放建立 */
    for (uint8_t round = 0; round < 20; round++) {
        uint8_t col_base = 0;
        for (uint8_t amux = 0; amux < AMUX_COUNT; amux++) {
            for (uint8_t col = 0; col < amux_col_sizes[amux]; col++) {
                for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
#ifdef UNUSED_POSITIONS_LIST
                    if (is_unused_position(row, col_base + col)) continue;
#endif
                    accum[row][col_base + col] += ec_readkey_raw(row, amux, col);
                }
            }
            col_base += amux_col_sizes[amux];
        }
        wait_ms(5);
    }
    uint8_t col_base = 0;
    for (uint8_t amux = 0; amux < AMUX_COUNT; amux++) {
        for (uint8_t col = 0; col < amux_col_sizes[amux]; col++) {
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
#ifdef UNUSED_POSITIONS_LIST
                if (is_unused_position(row, col_base + col)) continue;
#endif
                analog_set_top_reading(ANALOG_KI(row, col_base + col), accum[row][col_base + col] / 20);
            }
        }
        col_base += amux_col_sizes[amux];
    }
    OPAMP_DISABLE();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool    updated  = false;
    OPAMP_ENABLE();
    for (volatile uint8_t d = 0; d < MATRIX_COLS; d++) {} /* 空转一轮，等运放建立 */
    uint8_t col_base = 0;
    for (uint8_t amux = 0; amux < AMUX_COUNT; amux++) {
        for (uint8_t col = 0; col < amux_col_sizes[amux]; col++) {
            uint8_t gcol = col_base + col;
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                matrix_row_t bit = (matrix_row_t)(1u << gcol);
#ifdef UNUSED_POSITIONS_LIST
                if (is_unused_position(row, gcol)) continue;
#endif
                uint16_t absv = ec_readkey_raw(row, amux, col);
                uint16_t ki   = ANALOG_KI(row, gcol);

                /* 触底校准模式：抑制输出 + 只推高 bottom（语义见 analog_core.h §5.5） */
                if (analog_get_bottom_out_mode()) {
                    if ((int)absv > (int)ANALOG_BOTTOM_READING(row, gcol)) {
                        analog_set_bottom_reading(ki, absv);
                    }
                    if ((current_matrix[row] & bit) != 0) updated = true;
                    current_matrix[row] &= ~bit;
                    analog_force_release(ki);
                    last_absv[row][gcol] = absv;
                    continue;
                }

                /* 实时校准(顶部/触底读数)：读数持续偏离锚点才更新，防抖落盘在 analog_task */
                if ((int)absv < (int)ANALOG_TOP_READING(row, gcol) - CALIBRATION_THRESHOLD) {
                    analog_set_top_reading(ki, absv);
                } else if ((int)absv > (int)ANALOG_BOTTOM_READING(row, gcol) + CALIBRATION_THRESHOLD) {
                    analog_set_bottom_reading(ki, absv);
                }

                last_absv[row][gcol] = absv;

                /* 推模型：返回 true = 按下状态翻转，据此翻矩阵位 */
                if (analog_step_key(ki, analog_model_sw(ki, absv))) {
                    updated = true;
                    if (analog_get_pressed(ki)) {
                        current_matrix[row] |= bit;
                    } else {
                        current_matrix[row] &= ~bit;
                    }
                }
            }
        }
        col_base += amux_col_sizes[amux];
    }
    OPAMP_DISABLE();
    return updated;
}

/* bootmagic：analog_init 尚未执行，但 matrix_init_custom 里的 dummy adc_read
 * 已确保 ADC 驱动启动，可直读 EC。Esc(0,0) 深按(读数显著高于静置锚点)进 bootloader。
 * 覆盖 quantum/bootmagic.c 的 weak 默认实现(默认扫数字矩阵，EC 矩阵测不到键)。 */
void bootmagic_scan(void) {
    /* EC 矩阵需先初始化引脚/ADC/运放(bootmagic 可能在 matrix_init 之前运行) */
    matrix_init_custom();
    OPAMP_ENABLE();
    for (volatile uint8_t d = 0; d < MATRIX_COLS; d++) {}
    uint16_t v = ec_readkey_raw(0, 0, 0);
    OPAMP_DISABLE();
    if (v > ANALOG_DEFAULT_TOP_READING) {
        bootmagic_reset_eeprom();
        bootloader_jump();
    }
}

/* ---- LPM EC 钩子(kb_common/lpm_core.c 的 LPM_EC_MATRIX 分支调用) ---- */

/* RTC 唤醒循环 lpm_hal_init 内调用：HAL 重初始化后恢复 EC 引脚与 ADC */
int ec_init(void) {
    matrix_init_custom();
    return 0;
}

/* 低功耗期间用 ADC 扫描检测任意按键 */
bool ec_matrix_scan(matrix_row_t current_matrix[]) {
    return matrix_scan_custom(current_matrix);
}

/* 调试打印：每键最近一次真实 ADC 读数网格(KB_DEBUG=no 时 uprintf 为空实现，符号仍需存在) */
void ec_print_matrix(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uprintf("%4u ", (unsigned)last_absv[row][col]);
        }
        uprintf("\n");
    }
}
