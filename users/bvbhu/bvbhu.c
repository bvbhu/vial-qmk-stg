#include "bvbhu.h"

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
    if (il_led_count == 0)
        return false;
    int layer = get_highest_layer(layer_state);
    led_t led_state = host_keyboard_led_state();
    uint8_t mods = get_mods();
    bool active[indicator_count] = {
        layer == 0, layer == 1, layer == 2, layer == 3,
        led_state.caps_lock, led_state.num_lock, !led_state.num_lock, led_state.scroll_lock,
        mods & MOD_MASK_CTRL, mods & MOD_MASK_SHIFT, mods & MOD_MASK_ALT, mods & MOD_MASK_GUI,
        ck_tapped == Lead
    };
    for (uint8_t i = 0; i < il_led_count; i++)
    {
        uint8_t idx = il_leds[i].idx;
        if (idx < indicator_count && active[idx])
            RGB_MATRIX_INDICATOR_SET_COLOR(il_leds[i].pos, il_leds[i].r, il_leds[i].g, il_leds[i].b);
    }
    return false;
}

void keyboard_post_init_user(void)
{
    bvbhu_apply_fixed_config(); // 固定配置：解析指示灯表并打印
}