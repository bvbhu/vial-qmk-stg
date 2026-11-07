# tl12wlh481 无线功能说明

本键盘沿用 `bvbhu/tl12blef103` 的 **BHQ 无线框架**（`drivers/bluetooth/bhq/` +
`keyboards/keymagichorse/kb_common/`），通过 UART 接 CH582F / CH591R 桥模块，
桥固件在 `wch-ble-bridge/`。协议基准是 `wch-ble-bridge/uart_protocol.md`
（上游 `LinKeyDream/qmk_firmware_wireless`）。

USB / 蓝牙 3 通道 / 2.4G 的**操作方式**（配对、回连、多主机切换、Windows
Reason:8 排查等）与 tl12blef103 完全相同，请直接看
[`bvbhu/tl12blef103/BLUETOOTH.md`](../tl12blef103/BLUETOOTH.md)。
本文只列 N32H481 版本的差异与当前限制。

---

## 一、默认键位（与 tl12blef103 相同）

**Layer 0**：`A B C D E F` / `G H I J K` + 空格位 `MO(1)`
**Layer 1**（按住空格位）：`USB_TOG` `BLE_SW1` `BLE_SW2` `BLE_SW3` `RF_TOG` `NK_TOGG`

短按 / 长按的时长定义、30 秒广播窗口、Vial「无线」页按钮表等全部照旧。

---

## 二、相对 tl12blef103 的实现差异

| 项 | tl12blef103 (F103) | **tl12wlh481 (N32H481)** |
|---|---|---|
| 桥接串口 | STM32 USART2 + `PAL_MODE_STM32_ALTERNATE_PUSHPULL` | **N32 USART3（`SD3`，PD8/PD9）+ `PAL_MODE_ALTERNATE(17/18)`** |
| WS2812 | QMK `ws2812` bitbang driver | **`ws2812_n32.c` DWT 定时 bit-bang + `rgb_matrix.driver = "custom"`** |
| 动态键位存储 | F1 Flash 模拟 EEPROM | **片内 Flash 双页镜像 `eeprom_custom.c`** |
| Vial RAW HID | 标准 interrupt OUT endpoint | **EP0 `SET_REPORT`**（N32 USBHS 只有 OUT EP0 / 9..15） |
| 低功耗 | `lpm_stm32f1`（STOP + 矩阵 GPIO 唤醒，10 s 休眠） | **未启用** |
| 电池采样 | `battery.c` + F1 ADC / VREFINT | **未启用** |
| VID/PID | `0x544C / 0x1200` | **`0x544C / 0x1201`** |
| Vial UID | `54 4C 31 32 42 4C 45 01` | **`54 4C 31 32 57 4C 48 81`**（必须不同） |

0x11 `SET_CONFIG` 下发逻辑（500 ms 延迟 + 3 次重试）与 F103 版逐行等价，
只是把 `tl12_` 前缀换成了 `tl12wl_`。

---

## 三、当前限制（重要）

### 1. 串口已换成 USART3/PD8-PD9，仍缺真机收发验证

桥接串口走 **USART3（`SD3`）**，引脚 `PD8`(TX, AF17) / `PD9`(RX, AF18)。之所以能落在
PD8/PD9 上，是因为 N32H48x 给 USART3/UART5/UART8 做了**全引脚映射**：参考手册
Table 5-29 里 `USART3_TX` = 任意脚 `AF17`、`USART3_RX` = 任意脚 `AF18`，不用任何
AFIO 重映射寄存器（数据手册特性页原话：*"TX/RX of USART3/UART5/UART8 can be mapped
to all pins"*）。

`PD8/PD9` 在固定复用表里确实没有串口功能（只有 `ATIM1`、`GTIM9/10`、`SPI3`、
`I2S3`、`FEMC`、`ETH`），所以**不能用 USART2**——这也是从最初 PA2/PA3 方案改过来的原因。

固件侧已按官方 SDK 交叉核对 `USART3_BASE = APB1 + 0x1000`、
`RCC_APB1PCLKEN_USART3EN`/`RCC_APB1PRST_USART3RST = 0x8`、`USART3_IRQn = 39`
（对应 `VectorDC`），与本仓库 `n32_isr.h` 的 `N32_USART3_HANDLER = VectorDC`、
`hal_serial_lld.c` 的 `SD3` 实例完全一致；ELF 里 `SD3`、`VectorDC` 已核验为强符号，
`SD2` 已完全移除。详见 `WIRING.md` §四。

也就是说**固件侧不再是猜测**，剩下的是物理接线与电气连通：
工作区没有连接开发板，`_diag` 诊断工具也已删除，桥接串口的实际收发、
蓝牙配对 / 2.4G 仍需上板验证。N32 USARTv1 SERIAL LLD 本身也只做过静态审核 + 编译回归。

### 2. 不休眠、不报电量

`kb_common` 的低功耗后端只有 `lpm_chip_stm32f0/f1/f4.h`、
`lpm_chip_at32f4.h` 和 `matrix_sleep_default_stm32.c`；`battery.c` 直接调用
STM32 ADC 的 `adcSTM32EnableTSVREFE()` 等 API。N32 移植目前两者都没有，
所以 `rules.mk` 里：

```make
KB_LPM_ENABLED           = no
KB_CHECK_BATTERY_ENABLED = no
```

带来的行为差异：

- **不会自动休眠**（10 秒规则、广播中不休眠、按键唤醒都不存在）；
- `Bat: N% mV` 用户键（QK_USER_30）**没有有效读数**；
- 低电量降频/保护不生效；
- `bhq_common_init()` 仍把 `USB_POWER_SENSE_PIN`（定线后为 `C3`）配成内部下拉输入
  （悬空时钉在 0），但**当前配置下
  `usb_power_connected()` 没有任何活跃调用者**：它的调用点集中在 `lpm_core.c`、
  `battery.c` 和 `KB_CHECK_BATTERY_ENABLED` 内的 battery 任务，而本板这两个功能
  都关了。链接时该函数被 `--gc-sections` 裁掉（当前 ELF 里已核验无此符号）。
  因此 USB 插拔**不会**经 `C3` 主动触发桥状态切换；USB / 蓝牙 / 2.4G 的切换仍由
  BHQ 的 `transport_set(KB_TRANSPORT_USB)` 与 `USB_TOG` / `BLE_SW1..3` / `RF_TOG`
  按键驱动。若日后需要按 VBUS 自动切换，须把该判断接进 housekeeping 任务，
  或先补齐 N32 的 LPM / battery 后端。

要恢复这两个功能，需要为 N32H48x 写 `lpm_chip_n32h48x.h` +
`matrix_sleep_default_n32.c`，并把 `battery.c` 的 ADC 调用换成 N32 ADC LLD，
然后打开两个开关。这与引脚定线无关，是独立的移植工作量。

### 3. 上电 500 ms 内不响应无线操作

与 F103 版相同：0x11 配置在开机 500 ms 后才发第一次。等 1 秒再操作。

---

## 四、已验证 / 待验证

| 项 | 状态 |
|---|---|
| `make bvbhu/tl12wlh481:default` 编译 + 链接 | **PASS**（2026-11-07，USART3/PD8-PD9 定值后 `BUILD_EXIT=0`，61,612 B） |
| 矩阵 2×6 / RGB 12 灯 / Vial 布局 | 与 tl12blef103 逐字段一致（静态核对） |
| N32 SERIAL LLD 编译回归 | `bvbhu/n32h481_iotest:default` 已 PASS |
| 串口 AF 编号 / RCC 位 / 中断向量 | **已按官方手册逐项核对**（AF17/18、RCC `0x8`、IRQ 39 → `VectorDC`），ELF 中 `SD3`/`VectorDC` 为强符号、`SD2` 已移除 |
| 桥接串口收发、电气电平、蓝牙/2.4G | **待接线与真机验证** |
| 蓝牙配对 / 2.4G / 多主机 | **待接线与真机验证** |
| 休眠 / 电池 | **未实现** |

---

## 五、待办清单（按优先级）

1. ~~查 N32H48x GPIO 复用表，填 `TL12WL_UART_TX_AF` / `RX_AF`~~ → **已完成**：定为 USART3（PD8/PD9）AF17/AF18，已写入 `config.h` + `mcuconf.h`（`N32_SERIAL_USE_USART3 = TRUE`）
2. 按实际 PCB 定线，同步 `keyboard.json` 与 `config.h`；USART3 是全引脚映射，换脚只改 `UART_TX_PIN`/`UART_RX_PIN`，AF 保持 17/18
3. 真机验证：USB 枚举 → Vial 改键 → 桥接串口 → 蓝牙配对/回连
4. 需要无线续航时再移植 N32 LPM 与电池 ADC
