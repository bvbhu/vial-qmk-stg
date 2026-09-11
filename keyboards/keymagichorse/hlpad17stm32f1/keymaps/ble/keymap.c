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
#include QMK_KEYBOARD_H
#include "config.h"
#include "ws2812.h"
#include "color.h"
#include "bhq_common.h"
#include "wireless.h"
#include "keymap_introspection.h"

#ifdef VIA_ENABLE
#   include "via.h"
#endif

#if defined (RGB_MATRIX_CUSTOM_BATTERY_EFFECT)
#   include "rgb_matrix_battery_effect.h"
#endif

# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
#   include "rgb_matrix_index_by_wireless_keycode.h"
#   include "rgb_matrix_blink_effect.h"
#endif

# if defined(KB_CHECK_BATTERY_ENABLED)
#   include "battery.h"
#endif

// 临时变量，用于临时存放矩阵灯是否开启
uint8_t is_sleep = 0;
uint8_t rgb_matrix_is_enabled_temp_v = 0;

#define RGB_BAT      QK_USER_1       

// 延时点亮 RGB 的标志位
static uint8_t rgb_matrix_delay_open_flag = 0;
static uint32_t rgb_matrix_delay_open_timer = 0;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = LAYOUT(
		KC_NUM, KC_PSLS,  KC_PAST,  KC_PMNS,
		KC_P7,  KC_P8,    KC_P9,    KC_PPLS,
		KC_P4,  KC_P5,    KC_P6,
		KC_P1,  KC_P2,    KC_P3, 
		KC_P0,  KC_PDOT,            LT(1, KC_PENT)
	),
    
	[1] = LAYOUT(
		KC_A,    KC_TRNS, KC_TRNS,  KC_TRNS,
		BLE_SW1, BLE_SW2, BLE_SW3,  RF_TOG,
		USB_TOG, NK_TOGG, KC_TRNS,
		KC_TRNS, KC_TRNS, QK_BOOT, 
		RM_TOGG, RM_NEXT,           KC_TRNS
	),

	[2] = LAYOUT(
		KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, 
		KC_TRNS, KC_TRNS,           KC_TRNS
	),

	[3] = LAYOUT(
		KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, 
		KC_TRNS, KC_TRNS,           KC_TRNS
	),
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    
    if(keycode == RGB_BAT)
    {
#if defined (RGB_MATRIX_CUSTOM_BATTERY_EFFECT)
        if(record->event.pressed)
        {
            rgb_matrix_battery_effect_enabled();
        }
        else
        {
            rgb_matrix_battery_effect_disabled();
        }
#endif
    }
    // 关闭rgb矩阵灯 但 对应的指示灯还是可以亮的
    if(keycode == QK_RGB_MATRIX_TOGGLE)
    {
        if (record->event.pressed) {
            switch (rgb_matrix_get_flags()) {
                case LED_FLAG_ALL: {
                    rgb_matrix_set_flags(LED_FLAG_NONE);
                    rgb_matrix_set_color_all(0, 0, 0);
                } break;
                default: {
                    rgb_matrix_set_flags(LED_FLAG_ALL);
                } break;
            }
        }
        // 确保矩阵灯打开
        if (!rgb_matrix_is_enabled()) {
            rgb_matrix_set_flags(LED_FLAG_ALL);
            rgb_matrix_enable();
        }
        return false;
    }
    return process_record_bhq(keycode, record);
}

#ifdef VIA_ENABLE
__attribute__((weak)) bool via_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id   = &(data[0]);
    uint8_t *command_data = &(data[1]);
    uint16_t keycode = 0;
    switch (*command_id) {
        case id_dynamic_keymap_set_keycode: 
        {
            keycode = (command_data[3] << 8) | command_data[4];
            if(
                keycode == RF_TOG || 
                keycode == USB_TOG || 
                keycode == BLE_SW1 || 
                keycode == BLE_SW2 || 
                keycode == BLE_SW3
                )
            {
                wireless_keycode_rgb_index_refresh();
            }
            break;
        }
    }

    return via_command_bhq(data, length);
}
#endif


// 2812 电源开关
void ws2812_set_power(uint8_t on)
{
    gpio_set_pin_output(WS2812_POWER_PIN);        // ws2812 power
    if(on)  // 开
    {
#if WS2812_POWER_ON_LEVEL == 0
        gpio_write_pin_low(WS2812_POWER_PIN);
#else
        gpio_write_pin_high(WS2812_POWER_PIN);
#endif
    }
    else    // 关
    {
#if WS2812_POWER_ON_LEVEL == 0
        gpio_write_pin_high(WS2812_POWER_PIN);
#else
        gpio_write_pin_low(WS2812_POWER_PIN);
#endif
    }
}


// After initializing the peripheral
void keyboard_post_init_kb(void)
{
    wireless_keycode_rgb_index_init();
# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
    rgb_matrix_blink_effect_init();
#endif

#if defined (RGB_MATRIX_CUSTOM_BATTERY_EFFECT)
    rgb_matrix_battery_effect_init();
#endif

    rgb_matrix_delay_open_flag = 1;
    ws2812_set_power(1);
    rgb_matrix_is_enabled_temp_v = rgb_matrix_is_enabled();
    // rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_REACTIVE_WIDE);// rgb_matrix_mode_noeeprom(RGB_MATRIX_MULTISPLASH);    // 这两个测试xy用，挺好 // rgb_matrix_mode_noeeprom(RGB_MATRIX_CYCLE_SPIRAL);
}

// 低功耗外围设备电源控制
void lpm_device_power_open(void) 
{
    rgb_matrix_delay_open_flag = 1;
    ws2812_set_power(1);
    if(is_sleep == 1)
    {
        is_sleep = 0;
        ws2812_init();
        if(rgb_matrix_is_enabled_temp_v)
        {
            rgb_matrix_enable();    // 重新打开rgb矩阵灯
        }
        rgb_matrix_set_suspend_state(false);
    }
}

//关闭外围设备电源
void lpm_device_power_close(void) 
{
    is_sleep = 1;
    // 低功耗前 获取矩阵灯的状态
    rgb_matrix_is_enabled_temp_v = rgb_matrix_is_enabled();
    // 软关灯
    if(rgb_matrix_is_enabled_temp_v == 0)
    {
        // 软关灯，且不写入eeprom
        rgb_matrix_disable_noeeprom();  
    }
    rgb_matrix_set_suspend_state(true);
    // 关闭电源
    // ws2812电源关闭
    ws2812_set_power(0);

    gpio_set_pin_output(WS2812_DI_PIN);        // ws2812 DI Pin
    gpio_write_pin_low(WS2812_DI_PIN);
}




//  每个通道的颜色 以及大写按键的颜色
// HSV_BLUE        // 蓝牙 蓝色
// RGB_PURPLE      // 大小写：紫色
// HSV_RED         // 低电量：红色

void rgb_matrix_all_black(void)
{
    for (size_t i = 0; i < RGB_MATRIX_LED_COUNT; i++)
    {
        rgb_matrix_set_color(i, RGB_BLACK);
    }
}
// 矩阵灯任务
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // todo：搞了两坨是干啥，晚上优化掉试试
    if (rgb_matrix_delay_open_flag == 1) {
        rgb_matrix_delay_open_flag = 2;
        rgb_matrix_delay_open_timer = timer_read32();
        rgb_matrix_all_black();
        return false;
    }
    if (rgb_matrix_delay_open_flag == 2) {
        rgb_matrix_all_black();
        if (timer_elapsed32(rgb_matrix_delay_open_timer) > 500) { // 延时 500ms
            rgb_matrix_delay_open_flag = 0;
        }
        return false;
    }

    // 如果当前是USB连接，或者是蓝牙/2.4G连接且已配对连接状态
    if( (transport_get() > KB_TRANSPORT_USB && wireless_get() == WT_STATE_CONNECTED) || ( usb_power_connected() == true && transport_get() == KB_TRANSPORT_USB))
    {
        // 两个大写灯
        if (host_keyboard_led_state().num_lock) {
            // 两个大写灯
            rgb_matrix_set_color(0, RGB_PURPLE); 
            // Q17 W18 E19 R20
        }
        else
        {
            if(rgb_matrix_get_flags() == LED_FLAG_NONE)
            {
                rgb_matrix_set_color(0, RGB_BLACK); 
            }
        }
    }  
    // usb模式时，没有枚举成功，就强行灭灯
    if(transport_get() == KB_TRANSPORT_USB)
    {
        if(USBD1.state != USB_ACTIVE)
        {
            rgb_matrix_all_black();
        }
    }
    // 无线模式时，没有连接成功，就强行灭灯
    if(transport_get() > KB_TRANSPORT_USB)
    {
        if(wireless_get() != WT_STATE_CONNECTED)
        {
            rgb_matrix_all_black();
        }
    }

// ************** 闪烁rgb灯逻辑 **************
# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
    rgb_matrix_blink_effect_hook(led_min, led_max);
#endif
// ************** 闪烁rgb灯逻辑 **************

// ************** 显示电量灯条 逻辑 **************
#if defined (RGB_MATRIX_CUSTOM_BATTERY_EFFECT)
    rgb_matrix_battery_effect_hook(led_min, led_max);
#endif
// ************** 显示电量灯条 逻辑 **************
    return false;
}

// 无线蓝牙回调函数
void wireless_ble_hanlde_kb(uint8_t host_index,uint8_t advertSta,uint8_t connectSta,uint8_t pairingSta)
{
# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
    rgb_matrix_all_unblink();
    // 蓝牙没有连接 && 蓝牙广播开启  && 蓝牙配对模式
    if(connectSta != 1 && advertSta == 1 && pairingSta == 1)
    {
        // 这里第一个参数使用host_index正好对应_rgb_layers的索引
        rgb_matrix_blink(rgb_matrix_index_by_wireless_keycode(BT_PRF1 + host_index), RGB_BLUE, 0, 100, 100);
    }
    // 蓝牙没有连接 && 蓝牙广播开启  && 蓝牙非配对模式
    else if(connectSta != 1 && advertSta == 1 && pairingSta == 0)
    {
        rgb_matrix_blink(rgb_matrix_index_by_wireless_keycode(BT_PRF1 + host_index), RGB_BLUE, 0, 200, 300);
    }
    else if(connectSta != 1 && advertSta == 0 && pairingSta == 0)
    {
        rgb_matrix_all_unblink();
    }
    // 蓝牙已连接
    if(connectSta == 1)
    {
        rgb_matrix_blink(rgb_matrix_index_by_wireless_keycode(BT_PRF1 + host_index), RGB_BLUE, 5, 50, 50);
    }
#endif
}

// 24g函数回调
void wireless_rf24g_hanlde_kb(uint8_t connectSta,uint8_t pairingSta)
{
# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
    if(connectSta == 1)
    {
        rgb_matrix_blink(rgb_matrix_index_by_wireless_keycode(OU_2P4G), RGB_BLUE, 5, 50, 50);
    }
#endif
}

// 电量回调函数 红灯 慢闪
void battery_percent_changed_kb(uint8_t level)
{
# if defined(RGB_MATRIX_CUSTOM_BLINK_EFFECT)
    rgb_matrix_all_unblink();
    if(level <= 10)
    {
        rgb_matrix_all_unblink();
        rgb_matrix_blink(0, RGB_RED, 255, 500, 500);
    }
#endif
}

// 将未使用的引脚设置为输入模拟 
// PS：在6095中，如果不加以下代码休眠时是102ua。如果加了就是30ua~32ua浮动
void lpm_set_unused_pins_to_input_analog(void)
{

}
