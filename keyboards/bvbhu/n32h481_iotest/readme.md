# N32H481 IO Test

N32H481REL7K（LQFP64）的**引脚虚焊测试板**：把每个可用的 IO 都配成
「接地按键」（引脚内部上拉，短接到 GND 即为按下），键码从 **26 个字母 + 10 个数字**
里循环挑选（36 个一组，用完了从头再取）。**哪个脚能输入字符，就说明哪个脚没虚焊。**

> 与 `bvbhu/testf411` 同一套思路，只是换成了 N32H481 和这块板子的排针。
> 引脚丝印来源：`_pcb.png`（左右两排 2.54mm 排针）。

## 实测引脚数：47

引脚表按 N32H481 数据手册 **Table 3-1（LQFP64）** 逐脚核对：
本封装只邦定了 **56 个 GPIO**，去掉板载必需功能后剩 **47 个可测脚**。

> ⚠️ 本目录早期的引脚表有 **86** 个脚，把 `C6..C15`、`D0..D7`、`E0..E6` 等
> 未邦定脚也写了进去，且从未编译成功过。现已按数据手册重建。

## 排除的引脚

| 引脚 | 原因 |
|---|---|
| `H0` / `H1` | **8MHz 高速晶振 HSE** OSC_IN / OSC_OUT（不是 32.768kHz） |
| `B12` / `B13` | USB HS VBUS / OTG ID |
| `B14` / `B15` | **USB HS D- / D+**（键盘通信必需，注意不是 A11/A12） |
| `A13` / `A14` | SWDIO / SWCLK，保留 SWD 以便反复烧录 |
| `B7` | BOOT0（硬件启动模式脚） |
| `NRST` / `VDD` / `VSS` / `VDDA` / `VSSA` / `VREF+` | 非 IO |

本封装**未邦定**（物理上不存在，写了会读到幽灵电平）：

```
A10 A11 A12、B6 B10 B11、C6..C15、D0..D7 D15、E0..E6 E15、
F0 F1 F2 F6 F11..F15，以及整个 GPIOG
```

## 矩阵布局：2 行 × 24 列

`keyboard.json` 的 `matrix_pins.direct` 是 **2 行**：

| 行 | 引脚（共 47 个） |
|---|---|
| 第 0 行（24 个） | `A0..A9`、`A15`、`B0..B5`、`B8`、`B9`、`C0..C4` |
| 第 1 行（23 个 + 1 个 `null`） | `C5`、`D8..D14`、`E7..E14`、`F3`、`F4`、`F5`、`F7..F10` |

**为什么要拆成两行**（两个都是 QMK 自身的限制）：

1. `quantum/matrix.h:28-35`：`matrix_row_t` 最宽只有 `uint32_t`，`MATRIX_COLS > 32`
   直接 `#error "MATRIX_COLS: invalid value"`；47 个脚放不进一行。
2. `lib/python/qmk/constants.py:135` 的
   `COL_LETTERS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijilmnopqrstuvwxyz'`
   在第 34/35 位（`...hijilmn...`）重复了 `i`、还漏了 `k`。生成的 `LAYOUT` 宏参数名
   用 `k{ROW_LETTERS[row]}{COL_LETTERS[col]}`，单行列数超过 ~36 就会出现
   两个 `k0i`，编译报 `error: duplicate macro parameter 'k0i'`。

第 1 行末位的 `null` 会生成 `NO_PIN`，运行时被 `quantum/matrix.c` 的
`if (pin != NO_PIN)` 安全跳过，不占实际引脚。

键码按 `A..Z, 0..9` 循环（第 0 行 24 个 → `KC_A`..`KC_X`；第 1 行 23 个 →
`KC_Y, KC_Z, KC_1..KC_0, KC_A..KC_K`）。完整对应关系见
[`keymaps/default/keymap.c`](keymaps/default/keymap.c)（按端口分段注释）。

## 编译

```sh
qmk compile -kb bvbhu/n32h481_iotest -km default
```

或（QMK MSYS 里直接 make）：

```sh
cd /d/projects/vial\ wireless\ ec/vial-qmk-stg-n32h481
make bvbhu/n32h481_iotest:default
```

产物：`bvbhu_n32h481_iotest_default.bin`（当前约 25KB）。

## 烧录

板子没有 QMK bootloader（`bootloader: custom`），用 **J-Link** 配合
`N32H481\N32H48x_FLASH_Programming_Algorithm.0.7.0` 烧录：

```powershell
& "C:\Program Files\SEGGER\JLink_V7xx\JLink.exe" -device N32H481RE -if SWD -speed 4000 `
    -CommanderScript flash.jlink
```

`flash.jlink`：

```
connect
loadfile bvbhu_n32h481_iotest_default.bin 0x0
r
go
exit
```

详细环境搭建见 [`docs/n32h481-dev-env-setup.md`](../../../../docs/n32h481-dev-env-setup.md)。

## 上电后看到什么

* USB 枚举成一个标准键盘（VID `0x544C` / PID `0x4812`）。
* 用一根杜邦线把某个排针碰一下 `GND`，电脑就输入该脚对应的字母/数字。
  **有输入 = 该 IO 与焊盘连通、没虚焊；没输入 = 虚焊或该脚被其他功能占用。**
* 47 个脚一次全测不现实，建议**按端口一组一组测**（C 口 → A 口 → B 口 → …），
  或者配合 Vial 改键位来标注当前测的是哪一列。

## 硬件假设

* 芯片 N32H481RE（LQFP64），排针直接引出全部 IO，见 `_pcb.png`。
* 按键引脚内部上拉，按键一端接引脚、另一端接 GND，不需要外部电阻和二极管。
* USB HS 的 DM/DP 接在 `B14`/`B15`（不是 A11/A12）。
* `A0`(RGB) / `A1`,`A2`(KEY) / `A3`(LED+) / `A4`(LED-) 挂着板载外设网络，
  单独测试时读数可能受影响，但短接 GND 依然能可靠触发。
