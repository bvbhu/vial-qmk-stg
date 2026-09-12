#include "bvbhu.h"
#include "print.h"

/* 固定配置（取代 users/macro_config 的宏0动态解析）：
 *
 *   "Lead_term":"2000"   "F13_term":"500"   "RSPC_term":"250"
 *   "PROJ_term":"2000"   "Fn_term":"200"
 *   "feature_pscr":"1"   "feature_cv":"1"   "feature_pad":"1"（未指定，沿用默认开启）
 *
 * 延迟/开关写死在 custom_keycodes.c 的 custom_keycode_config；
 * LED 项沿用原格式 "Cell#RRGGBB"（Cell = 列字母+行数字，1-based，A1 = 左上角），
 * 颜色 0x03C800 绿 / 0xC80000 红。
 */
const indicator_led_cfg_t il_led_cfg[] = {
    {idx_Layer1_LED,     "G5", 0x03, 0xC8, 0x00},
    {idx_Layer2_LED,     "H5", 0x03, 0xC8, 0x00},
    {idx_Layer3_LED,     "I5", 0x03, 0xC8, 0x00},
    {idx_CapsLock_LED,   "A4", 0xC8, 0x00, 0x00},
    {idx_NumUnlock_LED,  "B4", 0xC8, 0x00, 0x00},
    {idx_ScrollLock_LED, "C4", 0xC8, 0x00, 0x00},
    {idx_Ctrl_LED,       "A6", 0xC8, 0x00, 0x00},
    {idx_Shift_LED,      "A5", 0xC8, 0x00, 0x00},
    {idx_Alt_LED,        "D6", 0xC8, 0x00, 0x00},
    {idx_Gui_LED,        "C6", 0xC8, 0x00, 0x00},
    {idx_Lead_LED,       "G3", 0x03, 0xC8, 0x00},
    {idx_Lead_LED,       "H3", 0x03, 0xC8, 0x00},
    {idx_Lead_LED,       "I3", 0x03, 0xC8, 0x00},
};
const uint8_t il_led_cfg_count = sizeof(il_led_cfg) / sizeof(il_led_cfg[0]);

// 运行期解析出的指示灯表：pos 依赖 g_led_config（rgb_matrix 生成），只能开机后计算。
indicator_led_t il_leds[BVBHU_INDICATOR_MAX];
uint8_t         il_led_count = 0;

// 把固定配置解析成运行期指示灯表；由 keyboard_post_init_user（bvbhu.c）调用一次。
void bvbhu_apply_fixed_config(void)
{
    uprintf("=== bvbhu fixed config ===\n");
    for (uint8_t i = 0; i < il_led_cfg_count; i++)
    {
        const indicator_led_cfg_t *cfg = &il_led_cfg[i];
        char c0 = cfg->cell[0], c1 = cfg->cell[1];
        if (c0 < 'A' || c0 > 'Z' || c1 < '1' || c1 > '9') continue;
        uint8_t col = c0 - 'A';
        uint8_t row = c1 - '0' - 1;
        if (col >= MATRIX_COLS || row >= MATRIX_ROWS) continue;
        uint8_t pos = LED_INDEX(col + 1, row + 1);
        if (pos == NO_LED || il_led_count >= BVBHU_INDICATOR_MAX) continue;
        il_leds[il_led_count].idx = cfg->idx;
        il_leds[il_led_count].pos = pos;
        il_leds[il_led_count].r   = cfg->r;
        il_leds[il_led_count].g   = cfg->g;
        il_leds[il_led_count].b   = cfg->b;
        il_led_count++;
        uprintf("%s: %u#%02X%02X%02X\n", cfg->cell, pos, cfg->r, cfg->g, cfg->b);
    }
    uprintf(
        "Lead_term:%u F13_term:%u RSPC_term:%u PROJ_term:%u Fn_term:%u\n"
        "pscr:%u cv:%u pad:%u\n",
        custom_keycode_config[idx_lead], custom_keycode_config[idx_f13], custom_keycode_config[idx_rightspace],
        custom_keycode_config[idx_proj], custom_keycode_config[idx_fn],
        custom_keycode_config[idx_pscr], custom_keycode_config[idx_cv], custom_keycode_config[idx_pad]);
    uprintf("=========================\n");
}
