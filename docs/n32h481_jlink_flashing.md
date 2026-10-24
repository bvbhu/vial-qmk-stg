# N32H481 J-Link 接线与烧录说明

本文档说明如何用 SEGGER J-Link 给 **N32H481REL7K** 开发板接线并烧录
QMK 固件(`.bin`/`.hex`)。

## 1. 板子上的调试引脚

N32H481REL7K(LQFP64)的调试口是标准 **SWD**:

| 信号 | MCU 引脚 | 板子上的位置 |
|---|---|---|
| SWDIO | PA13(脚 51) | 排针 H5 第 13 脚 |
| SWCLK | PA14(脚 52) | 排针 H5 第 12 脚 |
| NRST | NRST(脚 7) | 排针 H2 第 10 脚(标 RST) |
| GND | GND(脚 36/60) | 排针 H3 第 15 脚(标 GND) |
| 3V3(VTref) | VDD(脚 37/61) | 排针 H1 第 1 脚(标 5V3,即 3V3 网络) |
| BOOT0(可选) | PB7(脚 57) | 排针 H5 第 7 脚(标 BOOT) |

排针脚序(丝印 1..15,本仓库排针列表):
- **H5**(15→1):`A8 A9 A13 A14 A15 B3 B4 B5 BOOT B8 B9 F3 F4 F5 F7`
- **H2**(15→1):`F8 F9 F10 H0 H1 RST C0 C1 C2 C3 A0 A1 A2 A3 A4`
- **H3**(15→1):`D14 D13 D12 D11 D10 D9 D8 DP DN B13 B12 E14 E13 E12 GND`

## 2. J-Link 接线(标准 20 针 / 9 针 SWD 排线)

用杜邦线或 SWD 排线连接:

```
J-Link        →  开发板
  VTref  (1)  →  3V3(H1-1)
  GND    (4)  →  GND (H3-15)
  SWDIO  (7)  →  PA13 (H5-13)
  SWCLK  (9)  →  PA14 (H5-12)
  nRESET (15) →  RST  (H2-10)
```

- **VTref 必须接 3V3**:J-Link 用它检测目标电压;不接会报
  `Cannot connect to target` / `Target voltage not found`。
- 烧录时建议把 **BOOT0(H5-7)接地**(PB7=0),避免意外进入 BOOT ROM
  bootloader;烧完可断开。
- 板子由 USB Type-C 单独供电即可,不必 J-Link 供电。

## 3. 烧录步骤

### 3.1 编译固件

```sh
qmk compile -kb bvbhu/n32h481_demo -km default
# 或
qmk compile -kb bvbhu/n32h481_iotest -km default
qmk compile -kb bvbhu/n32h481_test -km default
```

产物在仓库根目录:`bvbhu_n32h481_demo_default.bin` 等。

### 3.2 J-Link 设备名

SEGGER J-Link 从 v7.x 起的设备库含 N32H4xx 系列。命令行直接指定:

```
-device N32H481RE
```

> 若你的 J-Link 版本报 `Unknown device "N32H481RE"`:
> ① 升级 J-Link Software and Documentation Pack;或
> ② 用 J-Link Configurator 手动添加 N32H481(选 N32H48x 系,Flash
> 起始 0x08000000、512KB);或
> ③ 改用官方 Keil Flash 算法(`N32H48x_FLASH_Programming_Algorithm`,
> 本仓库 `N32H481/` 目录下有源码)生成 .FLM 后挂到 J-Link;或
> ④ 走芯片内置 UART Bootloader(BOOT0 拉高 + USART3)。

### 3.3 命令行烧录(推荐)

新建 `flash.jlink`(官方 SDK 同款内容):

```
r
loadfile bvbhu_n32h481_demo_default.bin
r
g
qc
```

然后执行:

```sh
JLink.exe -device N32H481RE -if SWD -speed 4000 -CommanderScript flash.jlink
```

- `-speed 4000` = 4 MHz SWD;不稳可降到 1000。
- `loadfile` 支持 `.bin`(自动按 0x08000000)与 `.hex`。
- 成功时输出 `O.K.` 且无 `ERROR`;之后板子复位并运行固件。

### 3.4 手动方式(GUI)

1. 打开 **J-Flash** 或 **J-Link Commander**。
2. Device 选 `N32H481RE`;Interface 选 **SWD**,速度 4 MHz。
3. `connect` 成功(VTref 电压 3.3V 显示正常)。
4. `loadfile <固件>.bin`(或 File→Open 后 Program)。
5. `r` 复位,`g` 运行。

### 3.5 常见错误

| 现象 | 原因 | 解决 |
|---|---|---|
| `Target voltage not found` | VTref 未接 3V3 | 接 H1-1 |
| `Cannot connect to target` | SWDIO/SWCLK 接反或板未上电 | 检查 4 线,给板供电 |
| `Unknown device N32H481RE` | J-Link 库旧 | 升级 J-Link Pack 或手动加设备 |
| `Flash download failed` | Flash 算法不匹配/写保护 | 用官方 FLM,或先全片擦除(`erase`) |

## 4. 验证

烧录后:

- **n32h481_test / n32h481_iotest**:USB 接电脑,枚举为 HID 键盘;iotest
  里短接任意引脚到 GND,屏幕输出对应字母(大写/小写区分引脚)。
- **n32h481_demo**:拨动 U2 开关输出 `m`、磁铁靠近霍尔输出 `h`;
  三个灯分别亮红/绿/蓝。
- 若 USB 不枚举:检查 B14/B15(USB D-/D+)接线与 48MHz USBHS 时钟
  (已由 HAL 初始化,见 submodule `e1514cfb`)。
