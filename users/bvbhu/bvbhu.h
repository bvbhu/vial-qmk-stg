#ifndef BVBHU_H
#define BVBHU_H

#include "quantum.h"

// 键盘需在 keymaps/bvbhu/config.h 中定义行列到LED索引的映射
// col/row 均为 1-based，A1 = 左上角（顶行最左）
#ifndef LED_INDEX
#    define LED_INDEX(col, row) g_led_config.matrix_co[(row) - 1][(col) - 1]
#endif

enum custom_keycodes
{
    Lead = QK_KB_0,
    F_13,
    RightSpace,
    Fn,
};

// 自定义按键配置下标（合并延迟和开关）
// 配置为编译期固定值（原 users/macro_config 宏0键值对改为写死，数值见 custom_keycodes.c 与 fixed_config.c）
// clang-format off
enum custom_keycode_idx
{
    idx_lead, idx_f13, idx_rightspace, idx_proj, idx_fn,
    idx_pscr, idx_cv, idx_pad,
    custom_keycode_count
};

//指示灯键名序号
enum indicator_idx
{
    idx_Layer0_LED, idx_Layer1_LED, idx_Layer2_LED, idx_Layer3_LED,
    idx_CapsLock_LED, idx_NumLock_LED, idx_NumUnlock_LED, idx_ScrollLock_LED,
    idx_Ctrl_LED, idx_Shift_LED, idx_Alt_LED, idx_Gui_LED,
    idx_Lead_LED,
    indicator_count,
    indicator_end = 0xFF    //结束标志
};
// clang-format on

// 单个LED配置项（运行时：pos 已解析为LED索引）
typedef struct
{
    uint8_t idx; // 指示灯序号 (enum indicator_idx)
    uint8_t pos; // LED位置
    uint8_t r, g, b;
} indicator_led_t;

// 固定LED配置项（编译期：cell = 列字母+行数字，如 "G5"；1-based，A1 = 左上角）
typedef struct
{
    uint8_t    idx;  // 指示灯序号 (enum indicator_idx)
    const char *cell; // LED位置，如 "G5"
    uint8_t    r, g, b;
} indicator_led_cfg_t;

// 运行期解析出的指示灯表（fixed_config.c）
#define BVBHU_INDICATOR_MAX 15
extern indicator_led_t il_leds[BVBHU_INDICATOR_MAX];
extern uint8_t         il_led_count;

// fixed_config.c
extern const indicator_led_cfg_t il_led_cfg[];
extern const uint8_t             il_led_cfg_count;
void bvbhu_apply_fixed_config(void); // keyboard_post_init_user 调用一次

// custom_keycodes.c
#define LEAD_SEQ_LEN 8 // LEAD序列长度
extern const uint16_t custom_keycode_config[custom_keycode_count];
extern uint16_t ck_tapped;
bool process_record_user(uint16_t keycode, keyrecord_t *record);

#endif
