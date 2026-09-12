#include "bvbhu.h"
#include <string.h>
#ifdef KEYBOARD_tl96mgf072
#    include "tl96mgf072.h"
#endif
#include "nvm/nvm_dynamic_keymap.h"

// 固定配置：原由 users/macro_config 宏0键值对设置，现写死。
// 对应 "Lead_term":"2000" "F13_term":"500" "RSPC_term":"250" "PROJ_term":"2000" "Fn_term":"200"
// "feature_pscr":"1" "feature_cv":"1" "feature_pad":"1"（后三项用户未指定，沿用默认开启）
const uint16_t custom_keycode_config[custom_keycode_count] = {
    [idx_lead] = 2000,
    [idx_f13] = 500,
    [idx_rightspace] = 250,
    [idx_proj] = 2000,
    [idx_fn] = 200,
    [idx_pscr] = 1,
    [idx_cv] = 1,
    [idx_pad] = 1
};

uint16_t ck_tapped = KC_NO;
static uint8_t ck_count = 0;
static bool ck_pressed = false;
static deferred_token ck_defer_id = INVALID_DEFERRED_TOKEN;

static bool process_record_lead(uint16_t keycode, keyrecord_t *record);
static bool process_record_f13(uint16_t keycode, keyrecord_t *record);
static bool process_record_rightspace(uint16_t keycode, keyrecord_t *record);
static bool process_record_project(uint16_t keycode, keyrecord_t *record);
static bool process_record_fn(uint16_t keycode, keyrecord_t *record);
static bool process_record_pscr(uint16_t keycode, keyrecord_t *record);
static bool process_record_cv(uint16_t keycode, keyrecord_t *record);
static bool process_record_pad(uint16_t keycode, keyrecord_t *record);
static uint32_t lead_callback(uint32_t trigger_time, void *context);
static uint32_t f13_callback(uint32_t trigger_time, void *context);
static uint32_t rightspace_callback(uint32_t trigger_time, void *context);
static uint32_t project_callback(uint32_t trigger_time, void *context);
static void ck_cancel_deferred(void);
static void ck_schedule_deferred(uint32_t delay_ms, deferred_exec_callback callback);

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
    if (ck_tapped == Lead) // Lead拦截
    {
        if (record->event.pressed) lead_callback(0, (void *)(uintptr_t)keycode);
        return false;
    }
    if (ck_tapped == F_13 && keycode != F_13) // F13拦截
    {
        return false;
    }
    if (ck_tapped != KC_NO && ck_tapped != keycode) // 被打断
    {
        ck_cancel_deferred();
        switch (ck_tapped)
        {
            case Lead:
                lead_callback(0, NULL);
                break;
            case F_13:
                f13_callback(0, NULL);
                break;
            case RightSpace:
                rightspace_callback(0, NULL);
                break;
            case G(KC_P):
                project_callback(0, NULL);
                break;
        }
        ck_tapped = KC_NO;
    }
    switch (keycode)
    {
        case Lead:
            return process_record_lead(keycode, record);
        case F_13:
            return process_record_f13(keycode, record);
        case RightSpace:
            return process_record_rightspace(keycode, record);
        case G(KC_P):
            if (custom_keycode_config[idx_proj] <= 0) return true;
            return process_record_project(keycode, record);
        case Fn:
            return process_record_fn(keycode, record);
        case KC_PSCR:
            if (!custom_keycode_config[idx_pscr]) return true;
            return process_record_pscr(keycode, record);
        case C(KC_V):
            if (!custom_keycode_config[idx_cv]) return true;
            return process_record_cv(keycode, record);
        case KC_P1 ... KC_PDOT:
            if (!custom_keycode_config[idx_pad]) return true;
            return process_record_pad(keycode, record);
    }
    return true;
}

static bool process_record_lead(uint16_t keycode, keyrecord_t *record)
{
    if (record->event.pressed)
    {
        ck_tapped = Lead;
        ck_count = 0;
    }
    return false;
}

static bool process_record_f13(uint16_t keycode, keyrecord_t *record)
{
    ck_pressed = record->event.pressed;
    if (record->event.pressed)
    {
        if (ck_tapped == KC_NO)
        {
            ck_tapped = F_13;
            ck_count = 0;
        }
        ck_count++;
        ck_schedule_deferred(custom_keycode_config[idx_f13], f13_callback);
    }
    return false;
}

static bool process_record_rightspace(uint16_t keycode, keyrecord_t *record)
{
    ck_pressed = record->event.pressed;
    if (record->event.pressed)
    {
        if (ck_tapped == KC_NO)
        {
            ck_tapped = RightSpace;
            ck_count = 0;
        }
        ck_count++;
        ck_schedule_deferred(custom_keycode_config[idx_rightspace], rightspace_callback);
        layer_on(1);
    }
    else
        layer_off(1);
    return false;
}

static bool process_record_project(uint16_t keycode, keyrecord_t *record)
{
    ck_pressed = record->event.pressed;
    if (record->event.pressed)
    {
        if (ck_tapped == KC_NO) ck_tapped = G(KC_P);
        if (!(get_mods() & MOD_MASK_GUI)) register_code(KC_RGUI);
        register_code(KC_P);
    }
    else
    {
        unregister_code(KC_P);
        ck_schedule_deferred(custom_keycode_config[idx_proj], project_callback);
    }
    return false;
}

static bool process_record_fn(uint16_t keycode, keyrecord_t *record)
{
    static uint16_t press_time = 0;
    if (record->event.pressed)
        press_time = timer_read();
    else
    {
        if (timer_elapsed(press_time) < custom_keycode_config[idx_fn])
        {
            uint8_t current = get_highest_layer(layer_state);
            if (current < DYNAMIC_KEYMAP_LAYER_COUNT - 1) layer_move(current + 1);
        }
        else
            layer_move(0);
    }
    return false;
}

static bool process_record_pscr(uint16_t keycode, keyrecord_t *record)
{
    static uint8_t amod = 0;
    if (record->event.pressed)
    {
        amod = get_mods() & MOD_MASK_ALT;
        if (amod)
            unregister_mods(amod);
        else
            register_code(KC_RALT);
        register_code(KC_PSCR);
    }
    else
    {
        unregister_code(KC_PSCR);
        if (amod)
            register_mods(amod);
        else
            unregister_mods(MOD_MASK_ALT);
    }
    return false;
}

static bool process_record_cv(uint16_t keycode, keyrecord_t *record)
{
    if (get_mods() & MOD_MASK_GUI)
    {
        if (record->event.pressed) tap_code(KC_V);
        return false;
    }
    return true;
}

static bool process_record_pad(uint16_t keycode, keyrecord_t *record)
{
    if (get_highest_layer(layer_state) > 0) return true;
    if (record->event.pressed) dynamic_keymap_macro_send(keycode - KC_P1 + 1);
    return false;
}

// 将键码转换为匹配字符（用于lead序列匹配）
static char keycode_to_char(uint16_t kc)
{
    switch (kc)
    {
        case KC_0:
        case KC_P0:
            return '0';
        case KC_1 ... KC_9:
            return '1' + (kc - KC_1);
        case KC_P1 ... KC_P9:
            return '1' + (kc - KC_P1);
        case KC_A ... KC_Z:
            return 'a' + (kc - KC_A);
        default:
            return 0;
    }
}

// ===== 以下两个函数从 users/macro_config 移入（Lead 序列查宏仍需使用） =====
// 读取宏区指定偏移的字节
static uint8_t get_macro_byte(uint16_t offset)
{
    uint8_t c;
    dynamic_keymap_macro_get_buffer(offset, 1, &c);
    return c;
}

// 查找内容以str开头的宏（str必须与宏开头完全匹配）
// 返回宏索引，未找到返回-1
static int16_t macro_match_prefix(const char *str)
{
    uint32_t total = nvm_dynamic_keymap_macro_size();
    uint8_t cnt = dynamic_keymap_macro_get_count();
    if (get_macro_byte((uint16_t)(total - 1)) != 0) // 检查EEPROM末尾是否为0（确保没有被占用）
        return -1;
    // 逐个宏逐字节匹配
    uint32_t offset = 0;
    uint8_t str_len = (uint8_t)strlen(str);
    for (uint8_t mi = 0; mi < cnt && offset < total; mi++)
    {
        bool is_match = true;
        uint32_t i = 0;
        uint8_t mc;
        // 先检查边界再读取，且 i 用 32 位避免超过 255 字节时回绕成死循环
        while (offset + i < total && (mc = get_macro_byte((uint16_t)(offset + i))) != 0)
        {
            if (i < str_len && mc != str[i]) // str的每个字符必须精确匹配
                is_match = false;
            i++;
        }
        if (is_match && i >= str_len) // 宏内容长度必须 ≥ 输入序列，否则不算匹配
            return mi;
        offset += i + 1; // 跳过\0到下一个宏
    }
    return -1;
}

static uint32_t lead_callback(uint32_t trigger_time, void *context)
{
    static char lead_str[LEAD_SEQ_LEN + 1];
    ck_cancel_deferred();
    if (context != NULL)
    {
        uint16_t keycode = (uint16_t)(uintptr_t)context;
        if (keycode != KC_SPC && keycode != KC_ENT && keycode != Lead && keycode <= QK_BASIC_MAX && ck_count < LEAD_SEQ_LEN)
        {
            char ch = keycode_to_char(keycode);
            if (ch == 0) return 0;
            lead_str[ck_count] = ch;
            ck_count++;
            ck_schedule_deferred(custom_keycode_config[idx_lead], lead_callback);
            return 0;
        }
    }
    ck_tapped = KC_NO;
    if (ck_count < 3) return 0;
    lead_str[ck_count] = '\0';
    // 查找匹配的宏
    int16_t mi = macro_match_prefix(lead_str);
    if (mi >= 0)
    {
        dynamic_keymap_macro_send(mi);
        return 0;
    }
    // 未匹配宏，检查是否全数字并依次输出
    for (uint8_t i = 0; i < ck_count; i++)
        if (lead_str[i] < '0' || lead_str[i] > '9') return 0;
    send_string(lead_str);
    return 0;
}

static uint32_t f13_callback(uint32_t trigger_time, void *context)
{
    ck_cancel_deferred();
    ck_tapped = KC_NO;
    switch (ck_pressed ? 0 : ck_count)
    {
        case 0:
            break;
        case 1:
        {
            led_t led_state = host_keyboard_led_state();
            if (led_state.caps_lock) tap_code(KC_CAPS);
            if (led_state.scroll_lock) tap_code(KC_SCRL);
            if (!led_state.num_lock) tap_code(KC_NUM);
            if (get_highest_layer(layer_state) > 0) layer_move(0);
            break;
        }
        case 2:
            tap_code16(A(KC_F4));
            break;
        case 3:
            // 原重载宏0配置（users/macro_config），固定配置无需重载
            break;
        case 4:
            break;
        default:
            reset_keyboard();
    }
    return 0;
}

static uint32_t rightspace_callback(uint32_t trigger_time, void *context)
{
    ck_cancel_deferred();
    ck_tapped = KC_NO;
    switch (get_highest_layer(layer_state) == 0 ? ck_count : 0)
    {
        case 4:
            SEND_STRING(" - " SS_LCTL("v"));
        case 3:
            tap_code(KC_BSPC);
        case 2:
            tap_code(KC_ENT);
            break;
        case 1:
            tap_code(KC_SPC);
            break;
    }
    return 0;
}

static uint32_t project_callback(uint32_t trigger_time, void *context)
{
    ck_cancel_deferred();
    ck_tapped = KC_NO;
    unregister_mods(get_mods() & MOD_MASK_GUI);
    return 0;
}

static void ck_cancel_deferred(void)
{
    if (ck_defer_id != INVALID_DEFERRED_TOKEN)
    {
        cancel_deferred_exec(ck_defer_id);
        ck_defer_id = INVALID_DEFERRED_TOKEN;
    }
}

static void ck_schedule_deferred(uint32_t delay_ms, deferred_exec_callback callback)
{
    ck_cancel_deferred();
    ck_defer_id = defer_exec(delay_ms, callback, NULL);
}
