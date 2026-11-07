# N32H47x/48x 家族适配与 SPI 驱动

> 2026-11-07：在已合入 `vial` 的 N32H481RE 移植基础上，把平台层泛化到同 SDK 家族，
> 并新增 SPI HAL 驱动。本文记录可复用边界、改动点、配置方法与验证结果。

## 1. 结论：哪些 MCU 可以"参考 H481 适配"

国民技术 `N32H47x_48x_Library` 对整个 H47x/48x 家族只提供**一份** CMSIS 头
（`n32h47x_48x.h`）、一份启动文件、一份系统时钟代码。型号由编译宏选择，
只影响中断向量表，寄存器地址、RCC/时钟树、GPIO AFSEL、ADC/USART/SPI 定义完全共用。
因此"能不能复用"的分水岭不是内核（都是 M4F），而是 **USB 控制器**。

- **可直接复用（本次已实现）**：`N32H481RE`、`N32H482`、`N32H487`。
  三者都有 USB HS DualRole（DWC OTG，PB14=D- / PB15=D+，中断 82/83/84/116），
  512K Flash / 192K 主 SRAM，240MHz，LQFP64 引脚兼容。现有 `USBOTGv1`、
  `ADCv1`、`USARTv1`、`GPIOv1`、`SYSTICKv1` 与 Flash EEPROM、WS2812 DWT 驱动
  全部照用，只需把 `keyboard.json` 的 `processor` 换成对应型号。
- **需要新 USB 驱动**：`N32H473`、`N32H474`。CMSIS 中断表里它们只有
  `USB_FS_HP_IRQn=19` / `USB_FS_LP_IRQn=20` / `USB_FS_WKUP_IRQn=42`，没有 USBHS
  相关中断；官方产品页也只写 "USB FS Device"。现有 `USBOTGv1` LLD 是 USBHS 的，
  不能直接搬。要适配需新增 USB FS Device LLD（含端点、描述符、SET_REPORT 路径），
  把主频上限从 240MHz 校到 200MHz，并把 USB 引脚由 PB14/PB15 改到 FS 引脚。
  这部分等于重写键盘最关键的 USB 通路，工作量远大于本次改动。
- **不能做 USB 键盘**：`N32H475`。它的 CMSIS 中断表连 USB_FS 三个中断都没有，
  该子系列定位是数字电源/电机控制，不具备 USB 设备控制器。
- **不能复用**：N32G45x / G43x / L40x / WB 等其它 N32 系列。它们使用不同的
  SDK、不同的寄存器命名与时钟/RCC 架构，等于从零做一个新 ChibiOS port，
  "参考 H481" 只能借鉴流程（CMSIS 放置、mcu_selection、板级配置），不能复用代码。

一句话：**同家族里 USBHS 的三款已经能编译，USBFS 的两款需要新 LLD，其余不划算。**

## 2. 本次家族化改动

- `platforms/chibios/mcu_selection.mk`：把原来只匹配 `N32H481RE` 的分支改成
  `filter N32H481RE N32H482 N32H487`，并把精确型号以 `-DN32H482=` 形式传给所有
  编译单元（空值宏，与 board.h 里的 `#define` 等价，不会重复定义告警）。
  `MCU_FAMILY=N32` / `MCU_SERIES=N32H48x` / `MCU_STARTUP=n32h48x` /
  `BOARD=GENERIC_N32H481XE` 不变。
- `lib/python/qmk/constants.py` + `data/schemas/keyboard.jsonschema`：
  processor 白名单加入 `N32H482`、`N32H487`。**两处都要改**，否则
  `qmk generate-config-h` 会报 `processor is not one of [...]` 并整份 keyboard.json 失效。
- 链接脚本沿用 `N32H481RE.ld`（512K flash，末尾 16K 留给片内 EEPROM；192K SRAM），
  文件名保留只为兼容 `MCU_LDSCRIPT`，注释已说明它是家族共用脚本。
- 型号宏的兜底逻辑从"无条件 `#define N32H481RE`"改为"未指定型号时才回退"，涉及
  `board/board.h`、`hal_lld.h`、`cmparams.h`。
- `n32_registry.h` 的能力表从 `#if defined(N32H481RE)` 泛化为 USBHS 三型号共用，
  H47x 明确排除（避免误报 `N32_HAS_USBHS`）。

板目录仍叫 `GENERIC_N32H481XE`，是历史命名；它现在是整个 USBHS 家族的通用板，
不要因为名字以为只能跑 H481。

## 3. SPI 适配（N32H481 及同家族）

### 3.1 实现

新增 `lib/chibios-contrib/os/hal/ports/N32/LLD/SPIv1/`
（`hal_spi_lld.c/.h` + `driver.mk`），由 `N32/N32H48x/platform.mk` 引入。

- 中断驱动（TXE/RXNE），**不依赖 DMA**（本 port 暂无 DMA LLD）；8-bit 帧、
  master、软件 NSS，FIFO 关闭，一次中断搬一帧，适合键盘的短传输。
- 覆盖 `SPI1..SPI6`，由 `N32_SPI_USE_SPI1..6` 与 `N32_SPI_SPIx_IRQ_PRIORITY`
  在板级 `mcuconf.h` 开关，默认只开 SPI1（IRQ 优先级 12）。
  SPI2/3 的时钟在 APB1，SPI1/4/5/6 在 APB2；复位/门控、NVIC 使能与释放都在 LLD 内。
- 与 STM32 不同的寄存器细节已处理：SPI 使能位是 **CTRL2.SPIEN**（不是 CTRL1.SPE），
  BR 在 CTRL1 bits[2:0]，CPOL/CPHA 是 `SPI_CLKPOL`/`SPI_CLKPHA`，
  还必须清 `SPI_I2S_CFGR` 的 bit1 才是 SPI 模式。
- `platforms/chibios/chibios_config.h` 把 QMK `spi_master.c` 写入的 STM32 风格
  `SPI_CR1_*` 位别名到 N32 CTRL1（`BR_2/BR_1/BR_0` ↔ CTRL1 的 BR[2:0]，字段值
  0..7 正好对应 /2../256），并把 `USE_GPIOV1` 提升为整个 N32 家族生效。

### 3.2 默认引脚（重要）

QMK 原始默认是 `SPID2` + `SCK=B13 / MOSI=B15 / MISO=B14`，而本家族
**B14/B15 是 USB HS D-/D+、B13 是 OTG ID**，绝对不能拿来跑 SPI。
因此 N32 平台默认改为：

- `SPI_DRIVER = SPID1`
- `SPI_SCK_PIN = A5`，`SPI_MOSI_PIN = A7`，`SPI_MISO_PIN = A6`
- 三者 `SPI_*_PAL_MODE = PAL_MODE_ALTERNATE(1)`（SDK `GPIO_AF_SPI1_*_PA*` 都是 AF1）
- 片选沿用 ChibiOS `SPI_SELECT_MODE_PAD`，通过 `spi_start(slavePin, ...)` 传入

`bvbhu/tl12wlh481` 的矩阵占用了 A5/A6/A7，若要在那块板上用 SPI，必须覆盖这些宏。
可用的替代组合（AF 号来自 SDK `n32h47x_48x_gpio.h`，LQFP64 未邦定脚见
`keyboards/bvbhu/n32h481_iotest/config.h`）：

- SPI3 + `PB3`(SCK) / `PB4`(MISO) / `PB5`(MOSI)，全部 AF1；注意 PB3/PB4 是 JTAG 脚，
  用 SWD 烧录时才释放，TL12 未占用这三个脚，是最省事的组合。
- SPI3 + `PD9`(SCK) / `PD11`(MISO) / `PD12`(MOSI)，全部 AF1；但 TL12 的 BHQ 桥
  已把 PD9 用作 USART3_RX，需要挪走 UART 或放弃无线桥。
- SPI1 + `PE7/PE8/PE9`（AF1）与 TL12 的 WS2812 DI（E7）冲突；
  SPI1 + `PB3/PB4/PB5`（AF2）同样受 JTAG 影响。
- SPI2 的 `PB13/PB14/PB15`（AF1）与 USB 冲突，**不要用**。

覆盖示例（放在键盘 `config.h`）：

```c
#define SPI_DRIVER SPID3
#define SPI_SCK_PIN B3
#define SPI_MISO_PIN B4
#define SPI_MOSI_PIN B5
#define SPI_SCK_PAL_MODE PAL_MODE_ALTERNATE(1)
#define SPI_MISO_PAL_MODE PAL_MODE_ALTERNATE(1)
#define SPI_MOSI_PAL_MODE PAL_MODE_ALTERNATE(1)
```

同时在键盘 `mcuconf.h` 里 `#undef/#define N32_SPI_USE_SPI3 TRUE`。

### 3.3 启用方式

1. 键盘 `halconf.h`：
   ```c
   #undef HAL_USE_SPI
   #define HAL_USE_SPI TRUE
   #include_next <halconf.h>
   ```
   只写命令行 `-DHAL_USE_SPI=TRUE` 不够稳：`USE_SMART_BUILD` 靠读取 halconf.h
   决定是否编译 LLD 源码。
2. 键盘 `rules.mk`：`SPI_DRIVER_REQUIRED = yes`（QMK 会加入 `spi_master.c` 并再次
   定义 `HAL_USE_SPI`）。之后即可调用 `spi_start()` / `spi_transmit()` /
   `spi_receive()` / `spi_write()`。
3. `bvbhu/n32h481_iotest` 已按上述配置作为编译回归载体（新增 `rules.mk`，
   `halconf.h` 打开 `HAL_USE_SPI`）。

## 4. 编译验证（均为 `BUILD_EXIT=0`）

- `bvbhu/n32h481_iotest:default`：27,928 B；`SPID1`、`spi_lld_init`、
  `VectorCC`（SPI1 ISR）为强符号，`spi_master.o` 与 `hal_spi_lld.o` 均已编译链接。
- 临时把该键盘 `processor` 改成 `N32H482` 编译一次：27,928 B；
  再改成 `N32H487` 编译一次：27,928 B。两次都确认编译参数里带 `-DN32H482=` /
  `-DN32H487=` 与 `-DQMK_MCU=...`，随后已把 processor 恢复为 `N32H481RE`。
- 回归：`bvbhu/tl12wlh481:default` 61,612 B（与上一次烧录产物一致）、
  `bvbhu/n32h481_demo:vial` 63,556 B、`bvbhu/n32h481_iotest:default` 27,928 B。

编译命令（Git Bash 里直接跑 make 会因为 `qmk` CLI 缺 `HOME`/`SHELL` 失败，
用 PowerShell 设置好环境再跑）：

```powershell
$env:PATH = "D:\Program Files\Keyboards\program\QMK_MSYS\opt\qmk\bin;D:\Program Files\Keyboards\program\QMK_MSYS\mingw64\bin;D:\Program Files\Keyboards\program\QMK_MSYS\usr\bin;" + $env:PATH
$env:MSYSTEM = "MINGW64"
$env:SHELL  = "D:\Program Files\Keyboards\program\QMK_MSYS\usr\bin\bash.exe"
make bvbhu/n32h481_iotest:default -j4
```

## 5. 尚未验证 / 后续

- SPI 只做了静态编译与链接验证，**没有真机收发**（逻辑分析仪/回环自检都未做）。
  上板前重点确认中断里的 `TE`/`RNE` 时序与 CS 极性，以及 16-bit 帧需求
  （当前 LLD 固定 8-bit，若接 16-bit 屏需扩展 `SPI_CTRL1_DATFF`）。
- H482 / H487 只做了编译验证，**没有对应开发板的真机验证**；
  USB 枚举、Flash EEPROM 页地址、时钟上限都按 H481RE 的实测值沿用。
- H47x 的 USB FS LLD、DMA LLD、`HAL_USE_UART`、I2C 等仍是空缺。
- `lib/chibios-contrib` 是 submodule（当前 detached 于 `c87fdd07`），
  本次 SPI LLD 与家族化改动都在它的工作区里；要推送给别人必须先提交到
  `bvbhu/ChibiOS-Contrib` 的 `n32h481` 分支，再在主仓 bump gitlink。
