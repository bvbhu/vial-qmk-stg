/* Copyright 2025 keymagichorse
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
#include "uart.h"
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = LAYOUT(
		KC_NUM,   		KC_PSLS,  KC_PAST,  KC_PMNS,
		KC_P7,    		KC_P8,    KC_P9,    QK_BOOT,
		KC_P4,    		KC_P5,    KC_P6,
		KC_P1,    		KC_P2,    KC_P3, 
		LT(1, KC_P0), 	KC_PDOT,  KC_PENT
	),

	[1] = LAYOUT(
		KC_A, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_B, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_C, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, 
		KC_TRNS, KC_TRNS, KC_TRNS
	),

	[2] = LAYOUT(
		KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, 
		KC_TRNS, KC_TRNS, KC_TRNS
	),

	[3] = LAYOUT(
		KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS,
		KC_TRNS, KC_TRNS, KC_TRNS, 
		KC_TRNS, KC_TRNS, KC_TRNS
	),
};

const rgblight_segment_t PROGMEM num_lock_[] = RGBLIGHT_LAYER_SEGMENTS( {0, 1, HSV_PURPLE} );      // 大小写：紫色

const rgblight_segment_t* const PROGMEM _rgb_layers[] = RGBLIGHT_LAYERS_LIST( 
    num_lock_
);


bool led_update_user(led_t led_state) {
  rgblight_set_layer_state(0, led_state.num_lock);
  return true;
}


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

void rgb_adv_unblink_all_layer(void) {
    for (uint8_t i = 0; i < 1; i++) {
        rgblight_unblink_layer(i);
    }
}

uint8_t hello_log[] = "hello debug\n";
// After initializing the peripheral
void keyboard_post_init_kb(void)
{
	ws2812_set_power(1);

	rgblight_disable();
	rgblight_layers = _rgb_layers;  // 层灯光赋值
	rgb_adv_unblink_all_layer();
	palSetLineMode(A1, PAL_MODE_INPUT_ANALOG); 
	palSetLineMode(A0, PAL_MODE_INPUT_ANALOG); 
	uart_init(115200);
	uart_transmit(hello_log,13);
}
