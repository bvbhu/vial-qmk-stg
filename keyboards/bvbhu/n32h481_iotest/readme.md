# N32H481 IO Test

N32H481REL7K（LQFP64）的**引脚虚焊测试板**：把每个可用的 IO 都配成
「接地按键」（引脚内部上拉，短接到 GND 即为按下），键码从 **26 个字母 + 10 个数字**
里循环挑选（36 个一组，用完了从头再取）。**哪个脚能输入字符，就说明哪个脚没虚焊。**

> 与 `bvbhu/testf411` 同一套思路，只是换成了 N32H481 和这块板子的排针。
> 引脚丝印来源：`_pcb.png`（左右两排 2.54mm 排针）。

## 排除的引脚

| 引脚 | 原因 |
|---|---|
| `H0` / `H1` | 32.768kHz 低速晶振 OSC_IN / OSC_OUT（丝印 `H0/OSCI`、`H1/OSCO`） |
| `B14` / `B15` | 用户指定排除 |
| `A11` / `A12` | USB DM / DP，被键盘的 USB 功能占用 |
| `A13` / `A14` | SWDIO / SWCLK，焊了按键就不能再 SWD 调试 |
| `NRST` / `VDD` / `VSS` / `VCAP` / `BOOT0` | 非 IO |

> 另外 GPIOG / GPIOH 整组不参与（LQFP64 未引出，板级 `board.h` 已置 `N32_HAS_GPIOG/H` 为 FALSE）。

## 引脚分配

矩阵全部放在**第 0 行**，列号 = `keyboard.json` 里 `matrix_pins.direct` 数组的序号。
每个脚一个键码，按 `A..Z, 0..9` 循环：

| 引脚组 | 引脚 | 列号 | 键码 |
|---|---|---|---|
| C 口 | `C0`–`C15` | 0–15 | `KC_A`…`KC_P` |
| A 口 | `A0`–`A10`、`A15` | 16–27 | `KC_Q`、`KC_R`、`KC_S`、`KC_T`、`KC_U`、`KC_V`、`KC_W`、`KC_X`、`KC_Y`、`KC_Z`、`KC_1`、`KC_2` |
| B 口 | `B0`–`B13` | 28–40 | `KC_3`、`KC_4`、`KC_5`、`KC_6`、`KC_7`、`KC_8`、`KC_9`、`KC_0`、`KC_A`、`KC_B`、`KC_C`、`KC_D`、`KC_E` |
| E 口 | `E2`–`E15` | 41–55 | `KC_F`…`KC_T` |
| D 口 | `D0`–`D15` | 56–71 | `KC_U`、`KC_V`、`KC_W`、`KC_X`、`KC_Y`、`KC_Z`、`KC_1`…`KC_0` |
| F 口 | `F2`–`F15` | 72–85 | `KC_A`…`KC_N` |

完整对应关系见 [`keymaps/default/keymap.c`](keymaps/default/keymap.c)（按端口分段注释）。

> ⚠️ `A11/A12`（USB DM/DP）和 `A13/A14`（SWDIO/SWCLK）**没有**配按键：
> 前者被 USB 占用，后者焊了键就失去 SWD 调试口，只能用 J-Link JTAG 或串口烧录。

## 编译

```sh
qmk compile -kb bvbhu/n32h481_iotest -km default
```

或（QMK MSYS 里直接 make）：

```sh
cd /d/projects/vial\ wireless\ ec/vial-qmk-stg-n32h481
make bvbhu/n32h481_iotest:default
```

产物：`bvbhu_n32h481_iotest_default.bin`。

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
* 86 个脚一次全测不现实，建议**按排针一列一列测**（C 口 16 个 → A 口 → B 口 → …），
  或者配合 Vial 改键位来标注当前测的是哪一列。

## 硬件假设

* 芯片 N32H481RE（LQFP64），排针直接引出全部 IO，见 `_pcb.png`。
* 按键引脚内部上拉，按键一端接引脚、另一端接 GND，不需要外部电阻和二极管。
* USB 的 DM/DP 接在 `A11/A12`。
