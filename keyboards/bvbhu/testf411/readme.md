# testf411

一块 STM32F411 的引脚测试板：**B12 输出 WS2812 灯带信号（3 颗灯，开机自动彩色变幻）**，其余 22 个引脚全部配成
「接地按键」（引脚内部上拉，短接到 GND 即为按下），键码从 **W / Q / E / R** 里循环挑，
按键有反应即说明该引脚可用。

只有 **一个 `keymaps/vial`**：灯效、键码都不写死在固件里，全部在
[Vial](https://vial.rocks) 网页版里按需改（改完断电不丢，存在芯片内部 Flash）。

## 引脚分配

B12 是 WS2812 数据脚，不参与按键矩阵；其余 22 个脚全部是按键，**没有功能层，A14 也只是一个普通测试键**。

| # | 芯片引脚 | 矩阵位置 | 键码 | 说明 |
|---|---------|---------|------|------|
| 0 | C14 | [0,0] | `KC_W` | 接地按键 |
| 1 | C15 | [0,1] | `KC_Q` | 接地按键 |
| 2 | A0 | [0,2] | `KC_E` | 接地按键 |
| 3 | A1 | [0,3] | `KC_R` | 接地按键 |
| 4 | A2 | [0,4] | `KC_W` | 接地按键 |
| 5 | A3 | [0,5] | `KC_Q` | 接地按键 |
| 6 | A4 | [0,6] | `KC_E` | 接地按键 |
| 7 | A5 | [0,7] | `KC_R` | 接地按键（SPI1 SCK，本板未用） |
| 8 | A6 | [0,8] | `KC_W` | 接地按键（SPI1 MISO，本板未用） |
| 9 | A7 | [0,9] | `KC_Q` | 接地按键（SPI1 MOSI，本板未用） |
| 10 | B0 | [0,10] | `KC_E` | 接地按键 |
| 11 | B1 | [0,11] | `KC_R` | 接地按键 |
| 12 | B2 | [0,12] | `KC_W` | 接地按键 |
| 13 | B10 | [0,13] | `KC_Q` | 接地按键 |
| — | **B12** | — | — | **WS2812 数据输出，3 颗灯** |
| 14 | B13 | [0,14] | `KC_E` | 接地按键 |
| 15 | B14 | [0,15] | `KC_R` | 接地按键 |
| 16 | B15 | [0,16] | `KC_W` | 接地按键 |
| 17 | A8 | [0,17] | `KC_Q` | 接地按键 |
| 18 | A9 | [0,18] | `KC_E` | 接地按键（USART1 TX，本板未用） |
| 19 | A10 | [0,19] | `KC_R` | 接地按键（USART1 RX，本板未用） |
| 20 | A13 | [0,20] | `KC_W` | 接地按键（SWDIO，接了按键就不能再连 SWD 调试） |
| 21 | A14 | [0,21] | `KC_Q` | 接地按键（SWCLK） |

未使用的引脚都是不能动的：`A11/A12` = USB D-/D+，以及 `VDD/VSS/VCAP/BOOT0/NRST`。

> ⚠️ **A14 = SWCLK，A13 = SWDIO**，两个脚都接了按键就等于放弃 SWD 调试口，
> 以后只能靠 DFU（按住 BOOT0 复位）烧录。要是想保留调试口，把 `keyboard.json` 里
> `matrix_pins.direct` 的 `"A14"`（和/或 `"A13"`）删掉重编即可。

同样的表也在 [pins.json](file:///D:/projects/vial%20wireless%20ec/pins.json) 里（`"引脚\n键码"` 数组）。

## 灯效

三颗 WS2812 开机即亮，`RGB_MATRIX_DEFAULT_MODE = RGB_MATRIX_CYCLE_ALL`，
**默认就是彩色循环变幻**，速度 `RGB_MATRIX_DEFAULT_SPD = 128`、亮度 `120`。
`keyboard.json` 里开了 39 个动画（彩虹、螺旋、彩色左右/上下扫、雨点、按压喷溅、星光、河流等）。

**按需切换**有两种方式：

1. 打开 [Vial](https://vial.rocks) → 左侧 **Lighting / RGB** 页，直接选动画、拖动色相/亮度/速度，
   还能逐个拖三颗灯的颜色。改完立刻生效、断电不丢。
2. Vial 里把任意一个测试键改成 `RGB_MOD`（下一个动画）或 `RGB_TOG`（开关），
   然后碰那个脚到 GND 就能换灯效 —— 这样连电脑都不用开。

> 注意：这个 Vial 分支的 rgb_matrix 动画一共 [35 个](../../../quantum/rgb_matrix/animations/)，
> 而且**名字和上游 QMK 不完全一样 —— 没有 `rainbow_mood` / `rainbow_swirl`**。
> 名字写错 QMK 不报错，只是静默忽略，所以 `keyboard.json` 里只放真实存在的名字。
> 另外 3 颗灯的布局是一条直线（x = 0/32/64），左右扫、螺旋、风车这类
> 「位置类」动画在 3 颗灯上看起来都差不多，**最能体现「彩色变幻」的是 `CYCLE_ALL`
> （默认）、`RAINBOW_MOVING_CHEVRON`、`HUE_WAVE`、`PIXEL_FLOW`**。

## 硬件假设

* 芯片：STM32F411（`STM32F411xE` 链接脚本，DFU 引导程序，`0x1FFF0000`）。
* **HSE 晶振 16 MHz**，见 [board.h](board.h)；如果板子是 25 MHz 晶振，把 `board.h` 里的
  `16000000` 换成 `25000000` 再重新编译，否则 USB 枚举不了。
* PLL：16 MHz / 16 = 1 MHz，×384 = 384 MHz VCO，/4 = **96 MHz SYSCLK**，/8 = 48 MHz USB，
  见 [mcuconf.h](mcuconf.h)。
* WS2812 用 `bitbang` 驱动（`ws2812.driver`），B12 直连灯带 DIN；`RGB_MATRIX_LED_COUNT = 3`。
* 按键引脚靠内部上拉，按键一端接引脚、另一端接 GND，不需要二极管和外部电阻。

## 编译

```sh
qmk compile -kb bvbhu/testf411 -km vial
```

烧录：按住 BOOT0 再复位进入 DFU，然后

```sh
qmk flash -kb bvbhu/testf411 -km vial
```

## 上电后看到什么

* 三颗 WS2812 开机即亮并且**自己彩色变幻**（`RGB_MATRIX_DEFAULT_ON` + `RGB_MATRIX_DEFAULT_MODE = RGB_MATRIX_CYCLE_ALL`）。
* 用一根杜邦线把任意按键引脚碰一下 GND，电脑就会输入对应的 W/Q/E/R —— 有输入就说明该引脚正常。
  A14 碰一下就是 `Q`，和其它脚一样，不再是功能键。

## Vial 使用注意

* Vial 需要 `VIAL_KEYBOARD_UID`，`keymaps/vial/config.h` 里的那 8 个字节是本板唯一的；
  **改了它 Vial 就打不开原来的键位配置了**。
* `keymaps/vial/rules.mk` 里开了 `VIAL_INSECURE`，所以进 Vial 不用按住解锁组合键；
  想恢复常规保护就把这一行删掉。
* `lighting: vialrgb`：Vial 左侧会多出 Lighting 页，可以直接拖三颗灯的颜色/亮度。
* 动态键位存在 STM32F411 的内部 Flash（wear-leveling），Vial 改键后断电不丢。
