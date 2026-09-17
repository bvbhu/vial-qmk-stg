#include "tl96mgf072.h"
#include "print.h"
#include "analog.h"              /* QMK ADC 驱动：adc_mux / pinToMux / adc_read */
#include "analog/analog_core.h"  /* 核心层：推模型状态机 + 持久化 */
#include "matrix.h"
#include "wait.h"
#include "bootloader.h"
#include "action.h"
#include "action_layer.h"

__attribute__((weak)) void bootmagic_reset_eeprom(void);

static const pin_t row_pins[] = MATRIX_ROW_PINS; // 行引脚数组(由硬件定义)
static const pin_t col_pins[] = MATRIX_COL_PINS; // 列引脚数组(由硬件定义)
static adc_mux adcMux[MATRIX_ROWS];              // ADC多路复用器配置数组(每个行对应一个)
static matrix_row_t matrix[MATRIX_ROWS];
static uint16_t last_absv[MATRIX_ROWS][MATRIX_COLS]; // 每键真实 ADC 读数(供 GUI 显示原始值)

// GUI 后端钩子：返回该键最近一次扫描的真实 ADC 读数(absv，未经键程映射)
int16_t analog_backend_get_raw_adc(uint16_t ki)
{
    if (ki >= MATRIX_ROWS * MATRIX_COLS) return -1;
    uint8_t row = ki / MATRIX_COLS, col = ki % MATRIX_COLS;
    return (int16_t)last_absv[row][col];
}

// 开机校准：采样各键静置读数(噪声基底)写入 top_reading，重算模型派生参数。
// bottom_reading 是持久化的物理锚点，这里不动；触底由实时校准推高并防抖落盘。
void calibrate_matrix(void)
{
    uint16_t noise_floor_accum[MATRIX_ROWS][MATRIX_COLS] = { 0 };
    int16_t  noise_floor_valid[MATRIX_ROWS][MATRIX_COLS] = { 0 }; // 异常样本以负数计数
    for (uint8_t i = 0; i < 20; i++)
    {
        for (uint8_t col = 0; col < MATRIX_COLS; col++)
        {
            gpio_write_pin_high(col_pins[col]); // 激活当前列
            wait_us(10);                        // 等待稳定
            for (uint8_t row = 0; row < MATRIX_ROWS; row++)
            {
                uint16_t adc_value_fornoise = adc_read(adcMux[row]);
                uint16_t absv = adc_value_fornoise <= 2047 ? 2047 - adc_value_fornoise : adc_value_fornoise - 2048;
                // 异常样本(采样期间该键被按下)直接丢弃，不计入均值：旧写法把它替换成
                // 一个无关常量再照常累加，等于人为拉高该键的噪声基底。
                if (absv > ANALOG_DEFAULT_BOTTOM_READING) { noise_floor_valid[row][col]--; continue; }
                noise_floor_accum[row][col] += absv;
            }
            gpio_write_pin_low(col_pins[col]); // 关闭当前列
        }
        wait_ms(5); // 采样间隔
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++)
        for (uint8_t col = 0; col < MATRIX_COLS; col++)
        {
            uint16_t n = 20 + noise_floor_valid[row][col];
            // 全部样本都异常(该键整轮被按住)：保留出厂锚点，别写 0
            if (n == 0) continue;
            analog_set_top_reading(row * MATRIX_COLS + col, (uint16_t)(noise_floor_accum[row][col] / n));
        }
}

// 标准矩阵函数实现
void matrix_init(void) // 初始化矩阵
{
    // 初始化ADC
    for (int idx = 0; idx < MATRIX_ROWS; idx++)
    {
        setPinInputHigh(row_pins[idx]);                                    // 设置行引脚为输入模式(每行一路 ADC)
        palSetPadMode(GPIOA, row_pins[idx] & 0x0F, PAL_MODE_INPUT_ANALOG); // 确保GPIO配置为模拟模式
        adcMux[idx] = pinToMux(row_pins[idx]);                             // 将行引脚映射到ADC多路复用器
        adc_read(adcMux[idx]);                                             // 初始读取(稳定ADC)
    }
    // 初始化列引脚为输出低电平
    for (uint8_t idx = 0; idx < MATRIX_COLS; idx++)
    {
        gpio_set_pin_output(col_pins[idx]); // 设置列引脚为输出模式
        gpio_write_pin_low(col_pins[idx]);  // 输出低电平
    }
    wait_us(200);
    for (int i = 0; i < MATRIX_ROWS; i++)
        matrix[i] = 0;
}

uint8_t matrix_scan(void) // 矩阵扫描
{
    bool updated = false;
    // 扫描所有列：ADC -> absv -> 模型键程 sw，逐键推进核心状态机(推模型)
    for (uint8_t col = 0; col < MATRIX_COLS; col++)
    {
        gpio_write_pin_high(col_pins[col]); // 激活当前列
        wait_us(10);                        // 等待稳定
        for (uint8_t row = 0; row < MATRIX_ROWS; row++)
        {
            uint16_t adc_value_current = adc_read(adcMux[row]);
            uint16_t absv = adc_value_current <= 2047 ? 2047 - adc_value_current : adc_value_current - 2048;
            uint16_t ki   = row * MATRIX_COLS + col;

            /* 触底校准模式：抑制输出 + 只推高 bottom（语义见 analog_core.h §5.5） */
            if (analog_get_bottom_out_mode())
            {
                if ((int)absv > (int)ANALOG_BOTTOM_READING(row, col))
                    analog_set_bottom_reading(ki, absv);

                matrix_row_t bit = (matrix_row_t)(1u << col);
                if ((matrix[row] & bit) != 0) updated = true;
                matrix[row] &= ~bit;
                analog_force_release(ki);

                last_absv[row][col] = absv;
                continue;
            }

            // 实时校准(顶部/触底读数)：读数持续偏离锚点才更新，防抖落盘在 analog_task
            if (absv < (int)ANALOG_TOP_READING(row, col) - CALIBRATION_THRESHOLD)
                analog_set_top_reading(ki, absv);
            else if (absv > (int)ANALOG_BOTTOM_READING(row, col) + CALIBRATION_THRESHOLD)
                analog_set_bottom_reading(ki, absv);
            // 静置基点回升：只降不升会让长期(温度)漂移单向累积，所有键的行程被
            // 系统性抬高。读数明显高于当前 top 时按 1 LSB 缓慢跟上去。
            else if (absv > (int)ANALOG_TOP_READING(row, col) + CALIBRATION_THRESHOLD)
                analog_set_top_reading(ki, (uint16_t)(ANALOG_TOP_READING(row, col) + 1));

            last_absv[row][col] = absv;

            // 推模型：返回 true = 按下状态翻转，据此翻矩阵位
            if (analog_step_key(ki, analog_model_sw(ki, absv)))
            {
                updated = true;
                if (analog_get_pressed(ki))
                    matrix[row] |= (matrix_row_t)(1u << col);
                else
                    matrix[row] &= ~((matrix_row_t)(1u << col));
            }
        }
        gpio_write_pin_low(col_pins[col]); // 关闭当前列
    }
    return updated;
}

matrix_row_t matrix_get_row(uint8_t row) // 获取指定行的矩阵状态
{
    return matrix[row];
}

void matrix_print(void) // 打印矩阵状态
{
    uprintf("Matrix state:\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++)
        uprintf("Row %u: 0x%04X\n", row, matrix[row]);
}

__attribute__((weak)) void bootmagic_scan(void) // 重写bootmagic扫描以避免与EC冲突
{
    // We need multiple scans because debouncing can't be turned off.
    gpio_write_pin_high(col_pins[0]);
    if (adc_read(adcMux[0]) < 1250)
    {
        bootmagic_reset_eeprom();
        // Jump to bootloader.
        bootloader_jump();
    }
    gpio_write_pin_low(col_pins[0]);
}
