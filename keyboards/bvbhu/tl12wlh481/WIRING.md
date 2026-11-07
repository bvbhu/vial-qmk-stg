# tl12wlh481 接线说明（引脚已定线）

> 本键盘与 `bvbhu/tl12blef103` **使用同一块矩阵板**：2 行 × 6 列、COL2ROW、
> 12 颗 WS2812、同一套 `LAYOUT` / `rgb_matrix.layout` / Vial 布局。
> 差别只有主控：STM32F103C8T6 → **N32H481RE（LQFP64）**。
>
> 引脚已按实际定线固定（2026-11-07）。改线时必须同时改
> `keyboard.json` 的 `matrix_pins` 与 `config.h` 顶部区块，两处不一致会静默错键。

---

## 一、引脚总表（已定线）

| 功能 | 引脚 | 说明 |
|---|---|---|
| ROW0 | **B0** | 行驱动（推挽输出） |
| ROW1 | **C4** | 行驱动（推挽输出） |
| COL0 | **A5** | 列读入（内部上拉） |
| COL1 | **A7** | 列读入（内部上拉） |
| COL2 | **B2** | 列读入（内部上拉） |
| COL3 | **C5** | 列读入（内部上拉） |
| COL4 | **B1** | 列读入（内部上拉） |
| COL5 | **A6** | 列读入（内部上拉） |
| WS2812 DIN | **E7** | `WS2812_DI_PIN`，DWT bit-bang 输出，直连矩阵板 DIN（12 颗，3.3V 供电） |
| BHQ UART TX | **PD8** | USART3，AF17，接桥 `D8`（=A8/PA8，RXD1） |
| BHQ UART RX | **PD9** | USART3，AF18，接桥 `D9`（=A9/PA9，TXD1） |
| USB 电源检测 | **C3** | 可选，见 §五；原 `A6` 已让位给 COL5 |
| 电池 ADC | — | 未启用（无 N32 电池后端） |

已核验：`E7` 属 LQFP64 已邦定的 `E7..E14` 组，`C4` 属 `C0..C4` 组，
`A5/A6/A7` 属 `A0..A9` 组，GPIOE/GPIOA 时钟由 PAL LLD 按需使能。

改引脚时同步三处：

- `keyboard.json` → `matrix_pins.rows` / `matrix_pins.cols`（**实际编译进去的值**）
- `config.h` → `TL12WL_ROW*_PIN` / `TL12WL_COL*_PIN`（速查用，防止两边漂移）
- `config.h` → `WS2812_DI_PIN`、`UART_TX_PIN` / `UART_RX_PIN`、`USB_POWER_SENSE_PIN`

---

## 二、为什么不能照抄 tl12blef103 的引脚

`tl12blef103` 用的是 Bluepill 的脚，其中**五个在 N32H481 上根本不能接矩阵**：

| tl12blef103 用途 | 引脚 | N32H481RE 上是什么 |
|---|---|---|
| COL0 | `B12` | **USB HS VBUS** |
| COL1 | `B14` | **USB HS D−** |
| COL5 | `B13` | **USB HS OTG ID** |
| ROW1 | `B15` | **USB HS D+** |
| COL4 | `A10` | **未邦定**（LQFP64 上物理不存在） |

N32H481 的 USB 是 **B14/B15**（不是 F103 的 A11/A12），照抄会直接把 USB 通信线接到矩阵上。
另外 `A13/A14` 是 SWD、`B7` 是 BOOT0、`H0/H1` 是 HSE 晶振，都不能当普通 IO。

未邦定脚（写了会读到幽灵电平）：
`A10 A11 A12`、`B6 B10 B11`、`C6..C15`、`D0..D7 D15`、`E0..E6 E15`、
`F0 F1 F2 F6 F11..F15`、整个 `GPIOG`。

可用的 47 个脚见 `bvbhu/n32h481_iotest/readme.md`（已按数据手册 LQFP64 逐脚核对）。

---

## 三、矩阵板侧（与 tl12blef103 完全相同）

```
   COL2ROW:  ROW ──►|── COL     二极管阳极朝 ROW、阴极朝 COL
                    ROW = 逐行拉低驱动，COL = 带上拉读入

             COL0  COL1  COL2  COL3  COL4  COL5
   ROW0 ──────┤K─────┤K─────┤K─────┤K─────┤K─────┤K
   ROW1 ──────┤K─────┤K─────┤K─────┤K─────┤K─────┤K
```

12 颗 WS2812 串联，`rgb_matrix.layout` 的序号与 tl12blef103 一致（即 DIN 端数起第 N 颗）。
**DIN 必须与主控共地**。

灯串供电按 **3.3 V**（业主 2026-11-07 确认）：数据电平与 VDD 同压，
`VOH≈3.3V > VIH=0.7×VDD`，**不需要任何电平转换**。

> **开发板自带的 3 颗灯（`RGB0/RGB1/RGB2`，原理图上挂 5V）不使用。**
> 它们和矩阵板的 12 颗**不是同一条链**，所以 §九 里看到的「5V」不适用于本键盘。
>
> ⚠️ 接线要点：矩阵板 `DIN` 必须**直接接 `E7`**（或排针上 `E7` 那一脚），
> **不要串过开发板的 `RGB0→RGB1→RGB2`**。理由：WS2812 是串联移位，
> 若从 `RGBOUT` 取信号，等于链首多了 3 颗（且那 3 颗未供电时根本不转发数据，
> 末端全黑）；即使供电，灯序也会整体偏移 3，`led_count` 得改 15 再空出前 3 颗。

---

## 四、BHQ 无线桥串口（USART3 on PD8/PD9）

| 键盘侧 | 桥侧（CH582F / CH591R） |
|---|---|
| `PD8` USART3 **TX**（AF17） | 桥 **D8**（= 芯片 `A8`/`PA8`，UART1 **RXD1**） |
| `PD9` USART3 **RX**（AF18） | 桥 **D9**（= 芯片 `A9`/`PA9`，UART1 **TXD1**） |
| GND | GND（必须共地） |
| 3V3 | VCC（桥供电，按桥板实际电压） |

桥侧引脚有**两套叫法**：模块/排针丝印写 `D8`/`D9`，芯片手册里是 `PA8`/`PA9`，是同一只脚。
方向由桥固件决定，已实证：

```
wch-ble-bridge/APP/include/uart_bridge.h:12
    PA8PA9   UART1 默认脚   RXD1=PA8   TXD1=PA9
wch-ble-bridge/config.h
    #define USE_UART_PIN PA8PA9
```

所以 **主控 PD8(TX) → 桥 D8(A8/RXD1)**、**桥 D9(A9/TXD1) → 主控 PD9(RX)**，交叉接线。

### 为什么是 USART3 而不是 USART2

`PD8/PD9` 在固定复用表里**没有** USART2 功能（SDK 里只有 `ATIM1_CH3/CH3N`、`GTIM9/10`、
`SPI3`、`I2S3`、`FEMC`、`ETH`），但 N32H48x 给 **USART3 / UART5 / UART8 做了「全引脚映射」**：

> 数据手册特性页：「TX/RX of USART3/UART5/UART8 can be mapped to all pins」
> 参考手册 **Table 5-29 USART3 Alternate Function Remapping**：
> `USART3_TX` → `PAx/PBx/PCx/PDx/PEx/PFx/PGx/PHx` 任意脚，**AF17**
> `USART3_RX` → 同上任意脚，**AF18**

也就是说这两脚只要把 `AFSEL` 写成 17 / 18 就能当串口用，**不需要任何 AFIO 重映射寄存器**。

固件侧已按官方 SDK 逐项核对：

```
Nations.N32H47x_48x_Library.1.2.0/firmware/CMSIS/device/n32h47x_48x.h
    USART3_BASE = APB1PERIPH_BASE + 0x1000
    RCC_APB1PCLKEN_USART3EN / RCC_APB1PRST_USART3RST = 0x00000008
    USART3_IRQn = 39  ->  VectorDC = 0x40 + 39*4 = 0xDC
```

`USART3_IRQn = 39 → VectorDC` 与本仓库 `n32_isr.h` 的 `N32_USART3_HANDLER = VectorDC`、
`hal_serial_lld.c` 里 `SD3` 的 `N32_PCLK1` / RCC 位完全一致。

### 换脚很容易

USART3 的全引脚映射意味着**任何已邦定的 GPIO 都能当桥串口**，只需改两个引脚宏，
AF 编号永远保持 17 / 18：

```c
#define UART_TX_PIN D8   /* 任意已邦定脚 */
#define UART_RX_PIN D9
#define TL12WL_UART_TX_AF 17
#define TL12WL_UART_RX_AF 18
```

波特率 **128000 8N1**，交叉接线，固件两侧各自配置。桥侧代码见 `wch-ble-bridge/`。

---

## 五、矩阵/灯之外的必接线

### 5.1 USB（芯片固定，不可改）

| 引脚 | 接到 |
|---|---|
| `B14` | USB **D−** |
| `B15` | USB **D+** |
| `B12` | USB **VBUS**——本固件的 USBOTGv1 LLD **完全不读** VBUS（无主机/OTG 角色），悬空也能枚举；按硬件规范接 5V 分压更稳 |
| `B13` | OTG ID，本固件不读，Type-C UFP 可悬空 |
| GND / 5V | 连接器 GND / VBUS 供电 |

USB HS 是**内置全速 PHY 走 HS 引脚**，不需要外部 1.5k 上拉（PHY 内部集成）；
D+/D− 走线要短、尽量等长、不要跨分割地。

### 5.2 烧录与调试（必须，否则板子只能烧一次）

| 引脚 | 作用 |
|---|---|
| `A13` | SWDIO |
| `A14` | SWCLK |
| `NRST` | 复位（强烈建议引出，配合 J-Link 的 `connect under reset`） |
| GND / 3V3 | 调试器参考 |

⚠️ 本构建**不会**关闭 SWD：`platforms/chibios/boards/GENERIC_N32H481XE/board/board.c`
的 `boardInit()` 是空的，`tl12wlh481.c` 也没有动 `SWJ_CFG`，所以烧录后 J-Link 仍可
直接连上反复烧写（早前版本曾在 `boardInit()` 里 `SWJ_CFG=DISABLE`，现已移除）。
仍建议引出 `NRST`，方便固件跑飞时用 `connect under reset` 兜底。

### 5.3 BOOT0

`B7` 必须 **10kΩ 下拉到 GND**（从 Flash 启动）。悬空可能进 ROM 下载模式，板子不跑固件。

### 5.4 时钟

本构建 `N32_PLLSRC = HSI`（`mcuconf.h`），**不需要外部晶振**。
`H0/H1` 可以完全悬空不接；若要更稳的 USB 时钟再补 8MHz 晶振 + 两个负载电容。

### 5.5 电源与地

- `VDD` 各脚并 100nF，`VDDA` 并 100nF + 1µF（本构建未用 ADC，但仍建议保留）
- **主控 GND、WS2812 GND、桥 GND 必须共地**，否则 WS2812 乱闪、串口收不到
- 矩阵板灯串 **3.3V 供电** → DIN 高电平门限 = 0.7×3.3 ≈ 2.3V，3.3V 数据直接满足，
  **不需要电平转换**。若哪天把灯改回 5V，就必须补 74HCT125/1N4148 抬升

### 5.6 可选的 USB 插入检测（`C3`）

`bhq_common_init()` 会把 `USB_POWER_SENSE_PIN`（现为 `C3`）配成内部下拉输入，用来判
「插了 USB 没有」。本构建 LPM/电池都没启用，`usb_power_connected()` 会被 `--gc-sections`
裁掉，**不接也完全不影响使用**。要接就按分压接：

```
VBUS 5V ──100k── C3 ──100k── GND
```

> 注意：`A6` 现在是 COL5，**不能**再当 USB 检测脚。`bhq_common_init()` 会无条件把
> `USB_POWER_SENSE_PIN` 设成输入下拉，若这里填 A6 会把 COL5 拉成输入、整列失效。

---

## 六、编译与烧录

```sh
# QMK MSYS，vial 分支
cd '/d/projects/vial wireless ec/vial-qmk-stg'
make bvbhu/tl12wlh481:default -j4
```

产物：`bvbhu_tl12wlh481_default.bin`（2026-11-07 实测 **61,612 B**）。

N32H481 **没有 QMK bootloader**（`bootloader: custom`），固件向量表直接链接在
`0x08000000`，用 J-Link 烧到 `0x08000000`：

```sh
# 1) 复制到无空格路径（关键，见下方说明）
mkdir -p /c/Users/Administrator/AppData/Local/Temp/tl12flash
cp .build/bvbhu_tl12wlh481_default.hex \
   /c/Users/Administrator/AppData/Local/Temp/tl12flash/tl12wlh481.hex

# 2) 写 J-Link 命令脚本
printf 'r\nloadfile C:/Users/Administrator/AppData/Local/Temp/tl12flash/tl12wlh481.hex\nr\ng\nqc\n' \
   > /tmp/flash.jlink

# 3) 烧录
"/c/Program Files/SEGGER/JLink_V990/JLink.exe" \
   -device N32H481RE -if SWD -speed 4000 -autoconnect 1 \
   -CommanderScript /tmp/flash.jlink
```

成功时输出 `O.K.` 与 `Program & Verify speed`，随后板子复位运行。

### ⚠️ 三个实测踩过的坑

1. **路径里有空格 → `loadfile` 报 `Error while parsing parameter <Addr>`**。
   J-Link CommanderScript 按空格切参数，`D:/projects/vial wireless ec/...`
   会被拆成多段。**先把固件复制到无空格路径再烧**（上面的做法）。
2. **`loadfile` 的地址参数要用逗号分隔**：`loadfile <file>, <Addr>`；
   更省事的是直接烧 `.hex`（内含绝对地址，不带地址参数）。
3. **`RDP2 protection is active, not possible to unsecure the device!` 是误报**，
   出现这句不代表烧录失败——本机 J-Link V9.90 随后正常完成擦写与校验。

### 回读校验（推荐每次都做）

```sh
printf 'r\nsavebin C:/Users/Administrator/AppData/Local/Temp/tl12flash/readback.bin 0x08000000 0xF0AC\nqc\n' > /tmp/rb.jlink
"/c/Program Files/SEGGER/JLink_V990/JLink.exe" -device N32H481RE -if SWD -speed 4000 \
   -autoconnect 1 -CommanderScript /tmp/rb.jlink
```

再把 `readback.bin` 与构建出的 `.bin` 逐字节比对，应完全一致。
2026-11-07 实测：**61,612 字节全部一致**。

### 烧录后的验证（2026-11-07 实测）

| 检查项 | 结果 |
|---|---|
| J-Link `connect` | VTref 3.300V，识别 Cortex-M4 r0p1，`Device "N32H481RE" selected` |
| `loadfile` | `O.K.`，`Program & Verify speed: 77 KB/s` |
| `savebin` 回读比对 | 61,612 B **逐字节一致** |
| J-Link `regs` | PC=`0x0800A9CA`（`usb_lld_start`/`chSysPolledDelayX`）、`IPSR=0`（线程模式，未进异常） |
| Windows 枚举 | **`VID_544C&PID_1201`**，复合设备 + 4 个 HID 接口全部 `Status=OK` |

> ⚠️ **不要用 J-Link 读 USBHS 寄存器来判断是否枚举成功**。
> J-Link 的 `connect` 会隐含一次 `AIRCR.SYSRESETREQ` 复位，复位瞬间 USB 核心被
> 软复位、`DCFG`（`0x40040A00`）清零，读出来是 `0`，看起来像"没枚举"，
> 其实是复位瞬态。判断枚举看 Windows 设备管理器即可。

> USB 序列号 `vial:f64c2b3c` 是 `builddefs/build_vial.mk` 里**全局写死**的，
> 本 fork 所有键盘共用；真正区分设备的是 **VID/PID + `VIAL_KEYBOARD_UID`**
> （本板为 `{0x54,0x4C,0x31,0x32,0x57,0x4C,0x48,0x81}`）。

---

## 七、上电前检查清单

- [ ] `keyboard.json` 的 `matrix_pins` 与 `config.h` 区块一致
      （当前：ROW `B0/C4`，COL `A5/A7/B2/C5/B1/A6`，RGB `E7`）
- [ ] 矩阵线没有接到 `B12/B13/B14/B15`（USB HS）、`A13/A14`（SWD）、`B7`（BOOT0）
- [ ] 没有使用任何未邦定脚（含 `E0..E6`）
- [ ] 二极管方向：阳极朝 ROW
- [ ] WS2812 DIN **直连 `E7`**（不经开发板 `RGB0-2` 链路），矩阵板 12 颗灯 **3.3V** 供电、与主控**共地**
- [ ] `PD8 → 桥 D8(A8)`、`PD9 ← 桥 D9(A9)`，交叉、共地
- [ ] `B7` 已 10k 下拉
- [ ] USB `B14/B15/B12` 已接，D+/D− 未接反
- [ ] `C3` 若接了 VBUS 分压，确认不是 `A6`
- [ ] 用 J-Link 而非 USB bootloader 烧录

---

## 八、尚未明确的接线（需要业主定）

固件能定的脚都已经定了（§一）。下面这些**要么是纯硬件选择、要么固件里还没有对应引脚**，
目前属于「未拍板」，不是「已默认」：

| # | 项目 | 现状 | 影响 | 建议 |
|---|---|---|---|---|
| 1 | **WS2812 供电电压** | **已定 3.3 V**（矩阵板 12 颗）。原理图上的 5V 是开发板自带 3 颗灯，不参与 | 3.3 V 供电时数据同压，免电平转换 | 矩阵板 VDD 接 3V3；`E7` 直连矩阵板 DIN |
| 2 | **灯串电源开关（RGB_EN）** | 开发板上没有灯电源 MOS；矩阵板也未确认 | 无线键盘通常用 MOS 切断灯电源省电；没有就只能靠软件待机 | 若矩阵板有 MOS 控制脚，定一个空闲 GPIO（如 `C2`/`F3`）再补代码 |
| 3 | **USB-C CC1/CC2** | 未定 | Type-C UFP 必须各接 **5.1kΩ 下拉到 GND**，否则 C-to-C 线不供电、只认 A-to-C | 连接器处加两颗 5.1k |
| 4 | **`B12` VBUS 分压** | 固件不读 | USBOTGv1 LLD 不看 VBUS，悬空也能枚举 | 不接即可；想规范就 `VBUS—100k—B12—100k—GND` |
| 5 | **`B13` OTG ID** | 固件不读 | 无 | UFP 悬空；做 OTG 才需要 |
| 6 | **`B7` BOOT0 下拉** | 文档要求 10k，硬件**未确认** | 悬空可能进 ROM 下载模式，板子不跑固件 | 10kΩ 到 GND，别留悬空 |
| 7 | **`NRST` 是否引出** | 未定 | 固件跑飞时只能断电重插 | 建议引出测试点，配合 `connect under reset` |
| 8 | **桥模块供电电压 / 烧录口** | 只定了 UART + 3V3 + GND | CH582F/CH591R 自己要烧录（USB 或 SWD），没留测试点就换不了桥固件 | 桥的 USB D+/D−、`BOOT`、`RESET` 至少留焊盘 |
| 9 | **`C3` USB 插入检测** | 已定义、未接线 | 本构建 LPM/电池关闭，`usb_power_connected()` 被裁掉，不接无影响 | 要「插 USB 自动切有线」再接 `VBUS—100k—C3—100k—GND` |
| 10 | **LPM 唤醒线 `A0`/`A1`** | 固件当前**不使用**（`KB_LPM_ENABLED=no`） | 将来开低功耗要 `A0`(桥唤醒主控) / `A1`(主控唤醒桥) 两根 | 现在**别把 `A0`/`A1` 挪作他用**，留着 |
| 11 | **电池方案** | 未定（USB only？还是锂电 + 充电 IC？） | 注意 `battery.c` 默认 `BATTERY_ADC_PIN = B1`，**B1 现在是 COL4**；将来启用电池必须先改这个宏，否则会把列线当 ADC | 先按 USB 供电做；要电池另定一路 ADC 脚（如 `A4`） |
| 12 | **矩阵板侧丝印 ↔ ROW/COL 对应** | 你给的是主控脚名，板子上哪一脚是 ROW0 仍需对实物 | 接反的典型症状：整行恒触发 / 整列无响应 | 上电前用万用表二极管档逐点确认 |
| 13 | **LED 链 DIN 方向** | 未确认 | `rgb_matrix.layout` 序号是「从 DIN 端数第 N 颗」，接反则灯序整体镜像 | 先点亮一颗验证方向 |
| 14 | **数据线串阻 + 灯串去耦** | 建议项 | 无串阻时长线易振铃、首颗灯易误码 | `E7 —330Ω— DIN`，每颗灯 VDD-GND 并 100nF |
| 15 | **3V3 供电能力 vs 12 颗灯** | 板上 LDO 是 `XC6206P332MR-G`，**额定 250mA**；12 颗 WS2812 全白约 180–720mA（视批次/电压） | 亮度拉满时会把 3V3 拉垮 → 掉压、MCU 复位、灯闪 | ① 灯串单独供 3.3V（另加 LDO/DCDC）；或 ② 设 `RGB_MATRIX_MAXIMUM_BRIGHTNESS`（当前未设，默认 255）限到 120–160 以内 |

### 已确认不需要接的

- `H0/H1`：`N32_PLLSRC = HSI`，不需要外部晶振，可完全悬空
- `VDDA`：本构建不用 ADC，但按常规仍建议 100nF + 1µF
- 桥的 USB：有线模式走主控自带 USB，桥的 USB 不需要接（除非要给它烧录）

### 一个非接线但会踩的坑

`keyboard.json` 里 `"bootmagic": true`，但 `bootloader: custom` 对应
`platforms/chibios/bootloaders/custom.c` 的 `bootloader_jump()` / `mcu_reset()` **都是空实现**，
所以「上电按住左上角」和 `QK_BOOT` 都不会有任何反应（不会进 bootloader，也不会复位）。
不影响使用，但别指望它救砖——重烧只能靠 J-Link。

---

## 九、原理图核对记录（`SCH_Schematic1_1-P1_2026-11-07`）

按图逐块读出来的结果（1920×1305，逐区域放大核对）：

| 图纸区块 | 图上实际内容 |
|---|---|
| 主控 `U1` | `N32H481REL7K`（LQFP64），VDD/VSS 接 `3V3`/`GND`；`NRST`、`PH0/PH1` 均有引出 |
| 引脚网络命名 | **网络名 = 主控脚名**：`A5 A6 A7 C4 C5 B0 B1 B2 E7`，与 §一 的矩阵定义一一对上（`A5→COL0`、`A6→COL5`、`A7→COL1`、`C4→ROW1`、`C5→COL3`、`B0→ROW0`、`B1→COL4`、`B2→COL2`、`E7→RGB`） |
| 5V→3.3V LDO | `XC6206P332MR-G`，输入 `VIN`（5V 经 FUSE），输出 `3V3` + `1uF` |
| VDDIO/VDDA 供电 | 第二片 `XC6206P332MR-G`，`VIN → VDDA`，带 `VREF+/VREF-` |
| USB 保护 | `FUSE` + `PRTR5V0U2X`（`C53416675`）ESD 阵列，`VBUS`↔`5V` |
| **WS2812 链** | **只有 3 颗**：`RGB0`/`RGB1`/`RGB2`，4 脚（`I`=3 `V`=2 `G`=4 `O`=1）；`RGB → RGB0.I`，`RGB0.O → RGB1.I → RGB2.I`，`RGB2.O → RGBOUT`；**`V` 全部接 `5V`**，`G` 接 `GND` |
| 背光支路 | `LED+ → LED1 → 1K → LED-`（单色 LED，非 WS2812） |
| 霍尔 | 3 脚器件：`1 VCC=3V3`、`2 OUT=HALL0`、`3 GND` |
| 单键 | `U2`：`KEY1 —(1)(2)— KEY2` |
| 对外排针 | `U3` 6 pin：`1 RGB`、`2 KEY1`、`3 KEY2`、`4 LED+`、`5 LED-`、`6 HALL0` |
| GPIO 引出排针 | `H1` 15 pin：`A8 A9 A13 A14 A15 B3 B4 B5 BOOT B8 B9 F3 F4 F5 F7`；`H2` 15 pin：`E12 E13 E14 B12 B13 DN DP D8 D9 D10 D11 D12 D13 D14` —— **桥用的 `D8/D9` 就在这里**，`DN/DP` 是 USB |

### tl12 用这块开发板上的哪些部分（业主 2026-11-07 定）

| 开发板资源 | tl12 是否使用 |
|---|---|
| 主控 `U1` + USB + LDO | ✅ 使用（3.3V 由板上 `XC6206P332MR-G` 供） |
| GPIO 排针 `H1`/`H2` | ✅ 矩阵 8 根、`E7` 灯线、`D8/D9` 桥串口都从这里取 |
| 自带 3 颗灯 `RGB0-2`（5V） | ❌ **不用**（所以图上的 5V 不适用于本键盘） |
| 背光 `LED1 + 1K`（`LED+/LED-`） | ❌ 不用（固件 `backlight: false`） |
| 霍尔 `HALL0` | ❌ 不用（本键盘纯机械矩阵，无 ADC 霍尔代码） |
| 单键 `U2`（`KEY1/KEY2`） | ❌ 不用（按键全部来自 TL12 矩阵板） |
| `bvbhu/n32h481_demo` 的按键板 | ❌ **不用**（那是 demo 自带 2 键小板，`matrix_pins.direct = [[A0, A4]]`，与 TL12 无关） |
| 矩阵板 | ✅ TL12 板：2×6 机械矩阵 + **12 颗 WS2812**，`rgb_matrix.layout` 与 tl12blef103 逐字段一致 |

### 由此确定的接线结论

1. **灯数 = 12**，与固件 `led_count: 12` 一致，不需要偏移、不需要改代码。
2. **灯供电 = 3.3V**，数据同压，**不要**加电平转换。
3. **矩阵板 DIN 直连 `E7`**（排针上标 `E7` 的那一脚），
   不经过 `RGB0→RGB1→RGB2` 这条板载链。走 `RGBOUT` 会多 3 颗、灯序整体偏移，且那 3 颗
   未供电时根本不转发数据，末端全黑。
4. 开发板的 `U3` 6 pin 是**霍尔单键演示口**，tl12 不用它接矩阵板。

> 结论：**主控侧的 9 根矩阵/灯信号与固件完全一致**（网络名就是脚名，已逐条对上）；
> 图上多出来的 3 颗灯、背光、霍尔、单键、U3 全部属于开发板自带演示电路，与 tl12 无关。
