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

/**
 * @file lpm_chip_stm32f0.h
 * @brief STM32F0（Cortex-M0，如 STM32F072）芯片专属低功耗配置
 *
 * 提供 STOP 模式寄存器操作、时钟初始化、USB 引脚配置和 RTC 周期唤醒。
 * 接口与 lpm_chip_stm32f4.h / lpm_chip_stm32f1.h 完全一致（见 lpm_chip.h 说明），
 * 由 kb_common.mk 依据 KB_LPM_DRIVER 名字里的 "stm32f0" 选择。
 *
 * 与 F4/F1 的差异（都已在代码处注释）：
 *   - F0 的 PWR_CR 只有 LPDS/PDDS，没有 F4 的 MRLVDS/LPLVDS/FPDS；
 *   - F0 的 DBGMCU_CR 只有 DBG_STOP/DBG_STANDBY，没有 DBG_SLEEP；
 *   - F0 的 USB 在 PA11/PA12 且 AF 编号是 4（F4 是 10）；
 *   - F0 的 LSI 是 40kHz（F4 是 32kHz），RTC WUT 的 tick 换算不同；
 *   - F0 的 RTC 是 RTCv2（与 F4 同一 LLD），同样支持 rtcSTM32SetPeriodicWakeup()。
 */

#pragma once
#include "quantum.h"
#include "gpio.h"

#ifdef LPM_RTC_WAKEUP
#    include "hal.h"
#endif

/* ------------------------------------------------------------------ */
/*  通用接口                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief 进入 STM32F0 STOP 模式
 *
 * 流程：可选关 HSE（改用 HSI）→ 配置 PWR 寄存器 → __WFI() → 清除 SLEEPDEEP
 */
static inline void lpm_chip_enter_stop_mode(void) {
#if STM32_HSE_ENABLED
    /* Switch to HSI */
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_SW)) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
        ;

    /* Set HSE off */
    RCC->CR &= ~RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY))
        ;

    palSetLineMode(LPM_STM32_HSE_PIN_IN, PAL_MODE_INPUT_ANALOG);
    palSetLineMode(LPM_STM32_HSE_PIN_OUT, PAL_MODE_INPUT_ANALOG);
#endif

    /* F0 只有 LPDS(深睡眠进 Stop)/PDDS(进 Standby)：必须清 PDDS，否则进 Standby 会
     * 复位并丢 RAM。CWUF/CSBF 写 1 清上一次的唤醒/待机标志，避免刚进 WFI 就被旧标志
     * 直接穿透返回。F4 才有的 MRLVDS/LPLVDS/FPDS 在 F0 上不存在。 */
    PWR->CR &= ~PWR_CR_PDDS;
    PWR->CR |= PWR_CR_CWUF | PWR_CR_CSBF;
    PWR->CR |= PWR_CR_LPDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __WFI();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}

/**
 * @brief 唤醒后重新初始化 STM32 时钟（STOP 期间 HSI/PLL/HSI48 全部停止）
 */
static inline void lpm_chip_clock_init(void) {
    stm32_clock_init();
}

/**
 * @brief 禁用调试功能以降低功耗
 */
static inline void lpm_chip_debug_disable(void) {
    /* F0 的 DBGMCU_CR 只有 DBG_STOP / DBG_STANDBY（无 F4/F1 的 DBG_SLEEP） */
    DBGMCU->CR &= ~DBGMCU_CR_DBG_STOP;
    DBGMCU->CR &= ~DBGMCU_CR_DBG_STANDBY;
}

/**
 * @brief 唤醒后配置 USB D+/D- 引脚（F0：PA11/PA12，AF4）
 */
static inline void lpm_chip_usb_pins_init(void) {
    palSetLineMode(A11, PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST | PAL_STM32_PUPDR_FLOATING | PAL_MODE_ALTERNATE(4U));
    palSetLineMode(A12, PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST | PAL_STM32_PUPDR_FLOATING | PAL_MODE_ALTERNATE(4U));
}

/**
 * @brief 休眠前将 USB D+/D- 设为模拟输入（同时断开内部 1.5k 上拉，主机可见断开）
 */
static inline void lpm_chip_usb_pins_sleep(void) {
    palSetLineMode(A11, PAL_MODE_INPUT_ANALOG);
    palSetLineMode(A12, PAL_MODE_INPUT_ANALOG);
}

/* ------------------------------------------------------------------ */
/*  RTC 接口                                                           */
/* ------------------------------------------------------------------ */

#ifdef LPM_RTC_WAKEUP

/**
 * @brief RTC 初始化（F0 由 ChibiOS RTCv2 驱动在 halInit() 中完成，无需额外操作）
 */
static inline void lpm_chip_rtc_init(void) {
    /* ChibiOS RTC 驱动在 halInit() 中自动 rtcInit()/rtcStart()，并打通用
     * RTC WKUP 所在的 EXTI 线（见 RTCv2 LLD 的 rtc_lld_start()）。 */
}

/**
 * @brief 将毫秒转换为 RTC WUTR 寄存器值（bit[18:16]=WUCKSEL，bit[15:0]=WUT）
 *
 * F0 的 LSI 是 40kHz（F4 是 32kHz），WUCKSEL=0 时 WUT 时钟 = RTCCLK/16 = 2500Hz。
 */
static inline uint32_t lpm_rtc_wakeup_calc(uint32_t ms) {
    uint32_t wutr;

    if (ms <= 1000) {
        uint32_t ticks = (ms * (STM32_LSICLK / 16) + 500) / 1000;
        if (ticks == 0) {
            ticks = 1;
        }
        wutr = (0U << 16) | (ticks - 1);
    } else {
        uint32_t sec = ms / 1000;
        if (sec == 0) {
            sec = 1;
        }
        wutr = (4U << 16) | (sec - 1);
    }

    return wutr;
}

/**
 * @brief 配置 STM32F0 RTC 周期性唤醒
 * @param interval_ms 唤醒间隔（毫秒）
 */
static inline void lpm_chip_rtc_wakeup_setup(uint32_t interval_ms) {
    RTCWakeup wakeupspec;
    wakeupspec.wutr = lpm_rtc_wakeup_calc(interval_ms);
    rtcSTM32SetPeriodicWakeup(&RTCD1, &wakeupspec);
    rtcSetCallback(&RTCD1, NULL);
}

/**
 * @brief 清除 RTC 唤醒标志（F0 由 ChibiOS 驱动处理）
 */
static inline void lpm_chip_rtc_wakeup_clear(void) {
    /* ChibiOS RTC 驱动自动处理标志清除 */
}

/**
 * @brief 禁用 RTC 周期性唤醒（低电量时调用，只保留 USB 插入唤醒）
 */
static inline void lpm_chip_rtc_wakeup_disable(void) {
    rtcSTM32SetPeriodicWakeup(&RTCD1, NULL);
}

#endif /* LPM_RTC_WAKEUP */
