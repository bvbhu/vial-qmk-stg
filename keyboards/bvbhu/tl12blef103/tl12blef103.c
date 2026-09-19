/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * tl12blef103 板级主文件 (与目录同名, QMK 自动加入 SRC)
 *
 * 协议基准: https://github.com/LinKeyDream/qmk_firmware_wireless
 *   master      = 720d869a   (0x17 SET_DEV_INFO, 键盘私有)
 *   dev_rtc_test= d6199e10   (新增 0x11 SET_CONFIG, kb_common 通用)
 *
 * 本固件按 **0x11** 下发配置(见 wch-ble-bridge/uart_protocol.md §3.2):
 *   一次性传 蓝牙连接参数/功率/休眠/四组名称/VID-PID。
 * 不再使用旧 0x17 (已废弃)。
 */
#include "tl12blef103.h"
#include "timer.h" /* timer_read32 / timer_elapsed32 */
#include "ws2812.h" /* 唤醒后重配 WS2812 数据脚 (ws2812_init) */

#if defined(BLUETOOTH_BHQ)
#    include "km_printf.h" /* km_printf: KB_DEBUG 未开时为空实现 */
#endif

/* F1 ADC 兼容: ChibiOS F1xx 缺 adcSTM32EnableTSVREFE, battery.c 读 VREFINT 前调用 */
#if defined(STM32F1XX)
void adcSTM32EnableTSVREFE(void) {
    ADC1->CR2 |= ADC_CR2_TSVREFE;
}
#endif

void board_init(void) {
#if defined(BLUETOOTH_BHQ)
    bhq_common_init(); /* 电池 + USB 电源检测 */
#endif
#if defined(KB_LPM_ENABLED)
    lpm_init(); /* F103 STOP + 矩阵 GPIO 唤醒 */

    /* 补回 USB 检测脚(A6)的内部下拉。
     * lpm_init() 会把 A6 改回浮空(撤销 bhq_common_init() 设的下拉),
     * A6 悬空会导致 usb_power_connected() 读值不定。放在 lpm_init() 之后才有效。 */
    gpio_set_pin_input_low(USB_POWER_SENSE_PIN);
#endif
}

/* ============================================================================
 *  休眠时关闭 RGB / 唤醒后恢复
 *
 *  WS2812 是锁存器件, STOP 后停止推送数据时灯珠会保持最后一帧继续耗电。
 *  通过覆写 lpm_core.c 的 weak 函数 lpm_device_power_close/open 实现:
 *    close(): WFI 前推一帧全黑熄灭灯珠
 *    open():  唤醒后重配 WS2812 引脚(halInit() 会冲掉输出模式)并补推一帧
 *
 *  注意: 不要用 rgb_matrix_disable/enable 配对。disable 只改配置不推数据帧,
 *  唤醒后灯珠停在休眠前的全黑帧上, 直到效果下次重绘才恢复(实测灯不亮)。
 *  从不禁用矩阵: 进 STOP 后 rgb_matrix_task() 本就不跑, disable 无收益只增坑。
 * ========================================================================= */
#if defined(KB_LPM_ENABLED) && defined(RGB_MATRIX_ENABLE)

void lpm_device_power_close(void) {
    rgb_matrix_set_color_all(0, 0, 0);
    rgb_matrix_update_pwm_buffers();     /* 推一帧全黑; WS2812 锁存后熄灭 */
}

void lpm_device_power_open(void) {
    /* halInit()/lpm_chip_clock_init() 会冲掉 WS2812 数据脚的输出模式,
     * 这里补回。缺了它表现为"唤醒后灯不亮"。 */
    ws2812_init();
    rgb_matrix_update_pwm_buffers();
}

#endif /* KB_LPM_ENABLED && RGB_MATRIX_ENABLE */

/* ============================================================================
 *  0x11 SET_CONFIG 发送端
 *
 *  上游实现参考: kb_common/bhq_common.c 的 bhq_wireless_task(),
 *  用指定初始化(designated initializer)从 QMK 配置宏取值, 延迟 500ms 发一次。
 *  这里同样延迟 500ms 再发: CH582 桥上电慢于 STM32, 发早了会丢。
 *  与上游 0x17 的重发策略(300ms x 12)不同, 0x11 只发一次; 若你的桥启动较慢,
 *  可把 TL12_CFG_RETRY_COUNT 调大。
 * ========================================================================= */
#if defined(BLUETOOTH_BHQ)

#    ifndef TL12_CFG_DELAY_MS
#        define TL12_CFG_DELAY_MS 500
#    endif
#    ifndef TL12_CFG_RETRY_COUNT
#        define TL12_CFG_RETRY_COUNT 3
#    endif
#    ifndef TL12_CFG_RETRY_INTERVAL_MS
#        define TL12_CFG_RETRY_INTERVAL_MS 400
#    endif

/* 名称来源: 与上游 bhq_common.h 的默认宏保持一致, 各键盘可在 config.h 覆盖 */
#    ifndef BLE_NAME
#        define BLE_NAME PRODUCT
#    endif
#    ifndef BLE_SWIFT_PAIR_NAME
#        define BLE_SWIFT_PAIR_NAME PRODUCT
#    endif
#    ifndef USB_DOG_NAME
#        define USB_DOG_NAME PRODUCT "_DOG"
#    endif
#    ifndef USB_DOG_VENDOR_NAME
#        define USB_DOG_VENDOR_NAME MANUFACTURER
#    endif

/* 连接参数(与上游示例同值) */
#    ifndef TL12_LE_INTERVAL_MIN
#        define TL12_LE_INTERVAL_MIN 6 /* 单位 1.25ms */
#    endif
#    ifndef TL12_LE_INTERVAL_MAX
#        define TL12_LE_INTERVAL_MAX 35
#    endif
#    ifndef TL12_LE_TIMEOUT
#        define TL12_LE_TIMEOUT 500
#    endif
#    ifndef TL12_TX_POWER
#        define TL12_TX_POWER 0x3D /* 最强 */
#    endif
#    ifndef TL12_SLEEP_1_S
#        define TL12_SLEEP_1_S 10 /* 一级休眠(秒) */
#    endif
#    ifndef TL12_SLEEP_2_S
#        define TL12_SLEEP_2_S 300 /* 二级休眠/关机(秒) */
#    endif

/* 把字符串安全塞进定长数组(不带 NUL; 长度另由 Length 字段给出)。
 * 注意 bhqDevConfigInfo_t 的 bleNameStr 是 uint8_t[] 而非 char[], 直接
 * ".bleNameStr = BLE_NAME" 在上游能编译是因为 BLE_NAME 是字符串字面量且
 * 目标为 uint8_t 数组——C 里字符串字面量初始化 uint8_t[] 是合法的(逐字符)。
 * 但**赋值**(非初始化)不行, 故这里用 memcpy 显式拷贝, 且长度先夹到上限。 */
static uint8_t tl12_clamp_len(uint16_t len, uint16_t max, const char *what) {
    if (len > max) {
        km_printf("[cfg] %s too long: %d > %d, truncated\n", what, (int)len, (int)max);
        return (uint8_t)max;
    }
    return (uint8_t)len;
}

static void tl12_build_and_send_config(void) {
    bhqDevConfigInfo_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    /* 蓝牙的配置信息 */
    cfg.le_connection_interval_min     = TL12_LE_INTERVAL_MIN;
    cfg.le_connection_interval_max     = TL12_LE_INTERVAL_MAX;
    cfg.le_connection_interval_timeout = TL12_LE_TIMEOUT;
    cfg.tx_poweer                      = TL12_TX_POWER;

    /* 电池相关字段已废弃, 全部填 0 (上游注释明确要求) */
    cfg.mk_is_read_battery_voltage = 0;
    cfg.mk_adc_pga                 = 0;
    cfg.mk_rvd_r1                  = 0;
    cfg.mk_rvd_r2                  = 0;

    cfg.sleep_1_s = TL12_SLEEP_1_S;
    cfg.sleep_2_s = TL12_SLEEP_2_S;

    /* 四组名称: 长度先夹到上游上限, 再拷贝内容 */
    cfg.bleNameStrLength = tl12_clamp_len(strlen(BLE_NAME), BLE_ADVERT_NAME_MAX, "BLE_NAME");
    memcpy(cfg.bleNameStr, BLE_NAME, cfg.bleNameStrLength);

    cfg.bleSwiftPairNameStrLength = tl12_clamp_len(strlen(BLE_SWIFT_PAIR_NAME), BLE_SWIFT_PAIR_NAME_MAX, "BLE_SWIFT_PAIR_NAME");
    memcpy(cfg.bleSwiftPairNameStr, BLE_SWIFT_PAIR_NAME, cfg.bleSwiftPairNameStrLength);

    cfg.usbNameStrLength = tl12_clamp_len(strlen(USB_DOG_NAME), USB_NAME_MAX, "USB_DOG_NAME");
    memcpy(cfg.usbNameStr, USB_DOG_NAME, cfg.usbNameStrLength);

    cfg.usbVendorNameStrLength = tl12_clamp_len(strlen(USB_DOG_VENDOR_NAME), USB_VENDOR_NAME_MAX, "USB_DOG_VENDOR_NAME");
    memcpy(cfg.usbVendorNameStr, USB_DOG_VENDOR_NAME, cfg.usbVendorNameStrLength);

    /* 通用的配置信息 */
    cfg.vendor_id_source = 1; /* 1 = USB-IF */
    cfg.verndor_id       = VENDOR_ID;
    cfg.product_id       = PRODUCT_ID;

    bhq_ConfigParam(cfg);
}

static void tl12_config_task(void) {
    static uint8_t  sent      = 0;
    static uint32_t start_at  = 0;
    static uint32_t last_sent = 0;

    if (sent >= TL12_CFG_RETRY_COUNT) {
        return;
    }

    uint32_t now = timer_read32();

    if (sent == 0) {
        /* 首次: 记录起点, 等 TL12_CFG_DELAY_MS 让桥稳定后再发 */
        if (start_at == 0) {
            start_at = now ? now : 1; /* 避免 0 被当成"未初始化" */
            return;
        }
        if (timer_elapsed32(start_at) < TL12_CFG_DELAY_MS) {
            return;
        }
    } else if (timer_elapsed32(last_sent) < TL12_CFG_RETRY_INTERVAL_MS) {
        return;
    }

    last_sent = now;
    tl12_build_and_send_config();
    sent++;
}
#endif /* BLUETOOTH_BHQ */

void keyboard_post_init_kb(void) {
#if defined(BLUETOOTH_BHQ)
    tl12_config_task(); /* 记录起点, 真正的首发在 housekeeping 里延迟触发 */
#endif
    keyboard_post_init_user(); /* 透传用户钩子 */
}

/* ============================================================================
 *  开机自动进入蓝牙模式
 *
 *  问题: kb_common 的 transport.c 上电默认 KB_TRANSPORT_USB, 而本板电池供电
 *  (无 USB)时上电后所有按键都发向不存在的 USB —— 必须按一次 BT1 才切到蓝牙;
 *  同时 QMK 上电不发任何 0x14 广播命令, 桥(设计上广播完全由 QMK 控制)便不开
 *  广播, Windows 想自动回连也找不到设备。两个因素叠加 = "断电重启后必须按
 *  BT1 才能回连"。
 *
 *  方案: 上电 TL12_BT_BOOT_DELAY_MS 后, 若未插 USB 且用户仍未选择通道,
 *  自动切 BT1 并打开 30s 非配对广播(绑定过的 Windows 会静默自动回连,
 *  不会触发重新配对)。默认transport 仍是 USB, 插 USB 的场景不受影响。
 * ========================================================================= */
#ifndef TL12_BT_BOOT_DELAY_MS
#    define TL12_BT_BOOT_DELAY_MS 800 /* 等桥上电就绪(桥启动约需数百 ms) */
#endif
#ifndef TL12_BT_BOOT_RETRY_COUNT
#    define TL12_BT_BOOT_RETRY_COUNT 6 /* 0x14 广播帧重发次数 */
#endif
#ifndef TL12_BT_BOOT_RETRY_INTERVAL_MS
#    define TL12_BT_BOOT_RETRY_INTERVAL_MS 600 /* 重发间隔 */
#endif

static void tl12_bt_boot_task(void) {
    /* 广播帧重试: 桥上电需要数百 ms 就绪 UART, 单发一次会偶尔落在就绪窗口
     * 之前被丢(实测: 同一固件有时自动回连、有时必须按键)。这里按
     * TL12_BT_BOOT_RETRY_INTERVAL_MS 间隔重发, 收到桥的"已连接/已广播"
     * 状态帧(wt_state 离开 DISCONNECTED/INITIALIZED)后停止。 */
    static uint8_t  tries    = 0;
    static uint32_t last_try = 0;
    static bool     boot_done = false;

    if (boot_done) {
        return;
    }

    if (tries == 0) {
        if (timer_read32() < TL12_BT_BOOT_DELAY_MS) {
            return;
        }
    } else if (timer_elapsed32(last_try) < TL12_BT_BOOT_RETRY_INTERVAL_MS) {
        return;
    }

    /* 桥已回状态(连接或广播中) -> 广播帧已被处理, 停止重试 */
    if (tries > 0 &&
        (wireless_get() == WT_STATE_CONNECTED ||
         wireless_get() == WT_STATE_ADV_UNPAIRED ||
         wireless_get() == WT_STATE_ADV_PAIRING)) {
        boot_done = true;
        return;
    }

    /* 重试上限后放弃(桥可能真没接/没上电), 交给按键兜底逻辑 */
    if (tries >= TL12_BT_BOOT_RETRY_COUNT) {
        boot_done = true;
        return;
    }

    /* 插着 USB 就保持 USB 模式, 交给用户手动切换。
     * 但要**显式告知桥关闭广播**: 桥侧有"已绑定未连接即自主广播"的兜底
     * (HOGP 规范行为), 有线使用时不能让它在旁边一直广播 —— Windows 会
     * 自动回连, 白耗电且状态错乱。这里的 CLOSE 同样按重试节奏重发,
     * 避免落在桥 UART 就绪窗口之前被丢。 */
    if (usb_power_connected()) {
        last_try = timer_read32();
        bhq_CloseBleAdvertising();
        tries++;
        if (tries >= TL12_BT_BOOT_RETRY_COUNT) {
            boot_done = true;
        }
        return;
    }
    /* 用户已在启动后主动选过通道(如按键切换)则不覆盖。
     * 注意: 判据是"transport 不再是 BLUETOOTH_1"而不是"不再是 USB"——
     * 第一次发送后 transport 已被本任务设为 BLUETOOTH_1, 若按旧判据
     * (!= USB) 则重试首轮即退出, 退化回单发(实测广播帧全丢的根因)。 */
    if (tries > 0 && transport_get() != KB_TRANSPORT_BLUETOOTH_1) {
        boot_done = true;
        return;
    }

    last_try = timer_read32();
    transport_set(KB_TRANSPORT_BLUETOOTH_1);
    bhq_OpenBleAdvertising(0, 30); /* host 0, 30s 非配对广播 */
    tries++;
}

void housekeeping_task_kb(void) {
#if defined(BLUETOOTH_BHQ)
    tl12_bt_boot_task(); /* 上电自动进蓝牙模式(一次性) */
    bhq_wireless_task(); /* 多主机切换 + 电池 */
#    if defined(KB_LPM_ENABLED)
    /* 有线(USB)模式不休眠。
     *
     * 背景: kb_common 的 lpm_sleep_prepare() 只看 usb_power_connected(),
     * 而那需要 USB_POWER_SENSE_PIN(A6) 上的 5V 分压 (5V --100K-- A6 --100K-- GND)。
     * 本板未做该分压时 A6 悬空, 会被恒判为"未插 USB", 于是有线时也进 STOP。
     * 因此这里再叠一层传输模式判断: 只要当前在 USB 通道, 就不允许休眠。
     * 两个条件任一成立即不休眠 —— 更保险, 且不改变共用框架的其他键盘行为。 */
    if (!IS_USB_TRANSPORT(transport_get()) && !usb_power_connected()) {
        lpm_task(); /* 低功耗休眠 */
    }
#    endif
    tl12_config_task(); /* 0x11 配置下发(延迟 + 重试) */
#endif

    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#if defined(BLUETOOTH_BHQ)
    if (!process_record_bhq(keycode, record)) {
        return false;
    }
#endif

    return process_record_user(keycode, record); /* 透传用户钩子 */
}
