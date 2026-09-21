# Vial Analog Protocol Extension (磁轴 / 静电容)

> 契约文档：定义固件侧 `vial-qmk-wireless` 与 GUI/Web 侧之间的"模拟行程"通信协议。
> **行程量纲是 `0..ANALOG_MAX_TRAVEL` 的整数刻度（kb 编译期可配，默认 255），不使用 mm。**
> 固件实现见 `quantum/vial.c`(协议层) 与 `quantum/analog/analog_core.h`(核心层)，
> 命令号枚举见 `quantum/vial.h`。协议版本 `VIAL_ANALOG_PROTOCOL_VERSION = 1`。

- **基座**：Vial Raw HID，固定 32 字节包，`msg[0]=0xFE` 前缀，`msg[1]=命令号`。
- **命令号选址**：基础 Vial 子命令 `0x00`–`0x0D` 已占用且随上游从低位继续增长；
  本扩展刻意取高位段 `0xF0`–`0xF6`，远离增长区，规避上游未来占用低位的冲突。
- **设计原则**：轴体无关——Hall(磁轴) 与 EC(静电容) 在后端都由各自的键程模型映射为
  0..`ANALOG_MAX_TRAVEL` 的行程刻度；上层协议只谈"阈值"，两种轴共用同一套命令。

---

## 1. 概念模型

### 1.1 推送式状态机（push model）——与核心状态机的关系

**固件是按下状态的唯一权威**，上报是被动查询，不是协议驱动状态机：

```
kb matrix_scan()                      核心 (quantum/analog)
  ├─ adc_read()            取原始 ADC
  ├─ absv = |adc - 2048|   转成差值域        ← kb 决定，核心不碰 ADC
  ├─ analog_model_sw(ki, absv) → sw 0..M    ← 键程模型(kb 选编，全 weak)
  └─ analog_step_key(ki, sw) ──────────────→ 状态机推进，返回 true = 按下状态翻转
                                             └─ kb 据此翻 matrix[row] 的位
```

要点：

- **核心只见 `0..ANALOG_MAX_TRAVEL` 行程域**，`top_reading`/`bottom_reading` 是键程模型在
  **原始 ADC 域**的校准端点，核心只负责存储、持久化与在变更时回调模型（`analog_backend_calibration_changed`）。
- 行程映射**不是**协议层的线性归一化，而是 kb 选定的模型（见 §4.1）。协议只传校准端点与阈值。
- GUI 的 `0xF3` 实时查询与 `0xF1` 配置读写**都不会**改变按下状态。

### 1.2 线格式每键配置 `vial_analog_wire_config_t`（12 / 16 字节，0xF1/0xF2 用）

字段宽度随最大键程值 `M = ANALOG_MAX_TRAVEL` 走：4 项阈值就是核心的 `analog_travel_t`，
其余字段定长。`M <= 255` 时 4 项阈值是 `uint8_t`（**12 字节**，窄格式），
否则是 `uint16_t`（**16 字节**，宽格式）。两种宽度都列在下面。

窄格式（`M <= 255`，`sizeof == 12`）：

```c
typedef struct __attribute__((packed)) {
    uint8_t  actuation_point;   // [0]     触发阈值 0..M（上穿即注册按下）
    uint8_t  release_point;     // [1]     断开阈值 0..M（下穿即释放，≤触发阈值形成死区）
    uint8_t  rt_down;           // [2]     RT 触发距离 0..M（0=该方向不跟踪）
    uint8_t  rt_up;             // [3]     RT 释放距离 0..M（0=该方向不跟踪）
    uint8_t  flags;             // [4]     见下
    uint8_t  reserved;          // [5]
    uint16_t raw_rest;          // [6..7]  校准端点：初始校准读数（小端）
    uint16_t raw_full;          // [8..9]  校准端点：触底校准读数（小端）
    uint16_t reserved2;         // [10..11]
} vial_analog_wire_config_t;    // 12 字节
```

宽格式（`M > 255`，`sizeof == 16`）：

```c
typedef struct __attribute__((packed)) {
    uint16_t actuation_point;   // [0..1]    触发阈值 0..M（小端）
    uint16_t release_point;     // [2..3]    断开阈值 0..M（小端）
    uint16_t rt_down;           // [4..5]    RT 触发距离 0..M（小端）
    uint16_t rt_up;             // [6..7]    RT 释放距离 0..M（小端）
    uint8_t  flags;             // [8]       见下
    uint8_t  reserved;          // [9]
    uint16_t raw_rest;          // [10..11]  校准端点：初始校准读数（小端）
    uint16_t raw_full;          // [12..13]  校准端点：触底校准读数（小端）
    uint16_t reserved2;         // [14..15]
} vial_analog_wire_config_t;    // 16 字节
```

> 采用哪种宽度由固件编译期决定，并在 `0xF0` 的 `msg[6]`（= `sizeof`）与 `msg[8..9]`
> （= 最大键程值）里声明。**GUI 必须先读 caps 再解析，不得写死字段偏移或包长**：
> 同一串字节在两种宽度下的字段位置完全不同。
> 两条声明必须自洽：`msg[6]` 只能是 `msg[8..9]` 推导出的那一个宽度对应的字节数
> （`M <= 255` → 12，否则 16），固件侧由 `vial.c` 的静态断言绑死，GUI 侧在
> `analog_get_caps()` 里回验 `config_bytes_ok`，不一致一律拒绝接管该标签页。

`flags` 位定义（对应 `VIAL_ANALOG_FLAG_*`）：

| bit | 名称 | 含义 |
|-----|------|------|
| 0 | `RT_ENABLED` | 本键启用 Rapid Trigger（`rt_down`/`rt_up` 只在死区带内生效）|
| 1 | `ACTUATION_OVERRIDE` | 1 = 本键自定义阈值；**0 = 本键归全局管**（写 0 会让该键回到跟随全局）|
| 2 | `CONTINUOUS` | **预留**：当前仅被持久化并原样回报，不参与 `0xF3` 节流（见 §0xF3）。GUI 无任何入口设置它，写方应保持 0；在节流语义落地前，读方不得依赖它 |
| 3..7 | 预留 | 恒 0 |

> `ACTUATION_OVERRIDE` 与核心内部位**极性相反**——固定句式：
> **线上 1 = 自定义，核心 1 = 跟随，极性相反**。核心位即 `ANALOG_FLAG_FOLLOW_GLOBAL`，
> 翻译在 `quantum/vial.c`。

### 1.3 阈值字段 ↔ 核心字段映射

| 线格式 | 核心 (`analog_key_t` / `analog_global_t`) |
|--------|------------------------------------------|
| `actuation_point` | `actuation_threshold` |
| `release_point` | `release_threshold` |
| `rt_down` | `actuation_offset` |
| `rt_up` | `release_offset` |
| `flags.bit0` | `ANALOG_FLAG_RT_ENABLED`（同义）|
| `flags.bit1` | 取反后 = `ANALOG_FLAG_FOLLOW_GLOBAL` |
| `flags.bit2` | `ANALOG_FLAG_CONTINUOUS` |
| `raw_rest` | `top_reading` |
| `raw_full` | `bottom_reading` |

> 左列 4 项阈值与核心 4 项一一对应，**二者宽度都是 `analog_travel_t`**（§1.5），
> 所以线格式的宽度问题只在打包/解包处出现，核心内部无宽度换算。

### 1.4 状态机（核心逐键运行，`analog_step_key`）

核心维护每键 `pressed` 位与 RT 极值 `extremum`：

**死区带外（先判，优先于 RT）**
- `sw < release_threshold` → 强制释放
- `sw > actuation_threshold` → 强制触发
- 若 `release_threshold > actuation_threshold`，两者交换后使用（容忍 GUI 传反）

**死区带内 `[release, actuation]`**：仅当 `flags.RT_ENABLED` 才继续
- 已按下：跟踪峰值；自峰值回落 ≥ `rt_up` → 释放，`rt_up==0` 表示该方向不跟踪
- 未按下：跟踪谷值；自谷值上行 ≥ `rt_down` → 触发，`rt_down==0` 表示该方向不跟踪
- 翻转时把 RT 极值重置为当前 `sw`

> RT 极值 `extremum` 只记一个量：按下态记**峰值**（最深行程）、释放态记**谷值**（最浅行程），
> 翻转即在两者间切换。出厂 `flags = FOLLOW_GLOBAL`（RT 关），即默认纯阈值死区。

### 1.5 最大键程与行程域动态宽度

`ANALOG_MAX_TRAVEL` 是**行程域的最大键程值**，在 kb `config.h` 定义
（`quantum/analog/analog_core.h` 里 `#ifndef` 默认 255）。语义：

- 行程域是 `0..ANALOG_MAX_TRAVEL` 的整数刻度，`0` = 顶部（静置/释放）、
  `M = ANALOG_MAX_TRAVEL` = 触底。
- 编译期断言：`1 <= ANALOG_MAX_TRAVEL <= 65535`；另外出厂触发阈值、出厂断开阈值、
  出厂 RT 触发/释放距离四条都断言 `<= ANALOG_MAX_TRAVEL`——把最大键程值调小却忘了同步
  `ANALOG_DEFAULT_*`，出厂态就是"阈值高于最大键程值"，直接编译失败而不是静默失灵。

**动态数据类型**：最大键程值装得下 `uint8` 时行程一律用 `uint8_t`（默认板零开销、零布局变化），
只有超过 255 才升到 `uint16_t`：

| 判据 | `analog_travel_t` | `ANALOG_TRAVEL_WIDE` |
|------|-------------------|----------------------|
| `ANALOG_MAX_TRAVEL <= 255` | `uint8_t` | `0` |
| `ANALOG_MAX_TRAVEL > 255` | `uint16_t` | `1` |

> 判据写的是 **`<= 255`**（不是 `< 255`）：`0..255` 本就装得下 `uint8`，若把默认值 255
> 判成 `uint16`，默认板的协议包与 EEPROM 记录会无谓地整体膨胀。**默认 255 走窄格式。**

`analog_travel_t` 是"域宽度"而不是"值域上限"：值为 `ANALOG_MAX_TRAVEL` 时仍可能用 `uint8_t`
装。**所有 clamp 必须比较 `ANALOG_MAX_TRAVEL`，绝不能比较本类型的最大值**，否则最大键程值偏小的板会放过越界值、偏大的板会提前截断。

宽度决定的下游差异（全部由同一条判据推导，固件各处以静态断言绑死）：

| 项 | 窄（`M <= 255`） | 宽（`M > 255`） | 依据 |
|----|------------------|-----------------|------|
| `analog_key_t` | 10 字节 | 16 字节 | 核心静态断言 |
| `analog_global_t` | 6 字节 | 10 字节 | 核心静态断言 |
| `analog_record_t`（EEPROM 每键记录）| 8 字节 | 12 字节 | 核心静态断言 |
| `vial_analog_wire_config_t`（`0xF1`/`0xF2`）| 12 字节 | 16 字节 | 协议层静态断言 |
| `0xF3` 单条读数 | 3 字节 | 4 字节 | `VIAL_ANALOG_READING_ENTRY_BYTES` |
| `0xF3` 单包条数 | 10 | 7 | `31 / 条目字节数` |
| ISF 派生参数（`isf_scalar_t`）| `int16_t`（`D/K` 装得下时） | `int32_t` | 生成器按最坏评估采样点 `D/K` 量级 |
| `LINEAR_FAST` 的 `K` | `uint16_t` | `uint32_t` | `linfast_k_t` |

> 参考板 `keyboards/bvbhu/tl96mgf072/config.h` 取 `ANALOG_MAX_TRAVEL = 255`（窄格式一档），
> `keyboards/bvbhu/kbd67ble_ec/config.h` 取 `4000`（宽格式一档）——两块板刚好各覆盖一种宽度。
> 改成另一种只需动 kb `config.h` 这一行（GUI 侧全部由 `0xF0` 上报值驱动，无需重编）。
> 宽域实测代价（`tl96mgf072`，96 键）：`analog_key_t` 每键 +6 字节、ISF 的 `D[]`/`K[]`
> 每键 +4 字节，合计约 +960 字节 RAM；该板在此之下已无 RAM 余量，加宽前需先腾出空间
> （线性兜底/LINEAR_FAST 无 `D[]`/`K[]`，只多 ~576 字节）。

---

## 2. 命令定义（`msg[1]`，`0xFE` 前缀）

所有请求 `msg[0]=0xFE`、`msg[1]=cmd`、`msg[2..]=参数`；响应写回 `msg[0..31]`（同 32 字节包）。
`VIAL_ANALOG_PROTOCOL_VERSION = 1`。

> **版本基线**：本基线从 **1** 起算，固件与 GUI 均不保留任何历史形态的分支。
> GUI 侧 `constants.py` 的 `ANALOG_PROTOCOL_VERSION` 与固件**等值匹配**，不等值即拒绝接管该标签页
> （不支持 analog 的固件会把请求包原样回显，版本号是唯一可靠的挡板）。
> 此外 GUI 还校验 `msg[6]` 与 `msg[8..9]` 是否自洽（见 §1.2），不一致同样拒绝接管——
> 版本号相同但线格式宽度声明矛盾时，绝不带着错位偏移去解析。
> `0xF6` 显式保存不是"某版本新增"，它就是当前协议的一部分。

**版本史（唯一真源：固件 `quantum/analog/analog_core.h` 的 `VIAL_ANALOG_PROTOCOL_VERSION`）**

| 版本 | 日期 | 变更点 | 线格式宽度 |
|---|---|---|---|
| 1 | 2026-09-12 | 基线：`0xF0`–`0xF6` 子命令、推送式状态机、`0xF2` 只写 RAM + `0xF6` 显式落盘、最大键程值动态宽度 | 窄 12 / 宽 16 |

> **维护约定**：任何改动线格式或语义（字段含义、包长、命令号、节流规则）都必须同时
> ①改固件 `VIAL_ANALOG_PROTOCOL_VERSION` ②改 GUI `protocol/constants.py` 的同名常量
> ③在本表追加一行。GUI 侧有一条显式断言 `ANALOG_PROTOCOL_VERSION == 1`
> （`test_gui.py::test_analog_protocol_version_pinned`），bump 时会先红——这就是提醒同步的信号。
> 本表只记当前存在过的版本。

### 0xF0 `vial_analog_get_caps` —— 取能力

- req: `[FE][F0][00]`
- resp:
  | 偏移 | 含义 |
  |------|------|
  | msg[0] | analog 协议版本 (=1) |
  | msg[1..2] | 总键数 `num_keys`（小端；`num_keys <= 255`，故高字节恒 0）|
  | msg[3] | `axis_type`：1=磁轴(Hall)，2=静电容(EC)。仅供显示 |
  | msg[4] | `caps_flags`：见下 |
  | msg[5] | 单包最大读数条数 = 10（窄）/ 7（宽）（`31 / 条目字节数`，`0xF3` 一次返回几个键）|
  | msg[6] | `config_size` = 12（窄）/ 16（宽）（每键配置字节数，= `sizeof(vial_analog_wire_config_t)`）|
  | msg[7] | 触底校准模式当前运行态：`1`=开，`0`=关（GUI 重启后据此对上开关）|
  | msg[8..9] | 最大键程值 `ANALOG_MAX_TRAVEL`（小端）|
  | msg[10..31] | 0 |

- **GUI 必须先读本文应答再解析任何后续包**：字段宽度、包长、条数上限、控件量程全部由
  `msg[6]`（配置字节数）与 `msg[8..9]`（最大键程值）决定，**不得写死偏移或尺寸**。
  最大键程值是编译期常量、连接期不变，故只在 caps 里分发一次，不进每键 config。
- `caps_flags` 位：bit0 每键触发阈值、bit1 Rapid Trigger、bit2 校准、bit3 实时读数、
  bit4 每键断开阈值、bit5 `BOTTOM_OUT_CAL`（支持 `0xF4` mode4/5 触底校准开关）
  → 当前上报 `0x3F`。bit6 `AUTO_CAL` **预留未实现**（未置位，`AUTO_PEAK` mode3 返错）。
- `axis_type` 由编译期 `ANALOG_PROTOCOL_AXIS_TYPE` 推导（默认磁轴；定义
  `ANALOG_MODEL_EC` 时为静电容），kb 可覆盖。核心层不持轴概念。

### 0xF1 `vial_analog_get_key_config` —— 取单键配置+校准端点

- req: `[FE][F1][ki_lo][ki_hi]`（`ki` 小端，0 起；`0xFFFF`=全局默认槽）
- resp: `msg[0..11]`（窄）/ `msg[0..15]`（宽）= 对应宽度的 `vial_analog_wire_config_t`，其余不变
- `ki=0xFFFF` 返回 **EEPROM 存储的全局默认配置**，其 `raw_rest`/`raw_full` 回
  编译期默认校准值（全局槽不持有校准端点）
- 越界 `ki` → `msg[0]=1`（且不发配置）
  > 注意：合法响应也以 `msg[0]=actuation_point` 开头，其值可能是 1，故该错误码
  > **不可靠**。越界请求属于 GUI 侧不应发出的输入，GUI 应自查 `ki < num_keys`。

### 0xF2 `vial_analog_set_key_config` —— 写单键配置

- req: `[FE][F2][ki_lo][ki_hi][12 或 16 字节 config...]`（宽度同 `0xF0` 的 `msg[6]`）
- resp: `msg[0]=0` 成功 / 非 0 错误码（仅越界 `ki` 报 1）
- **语义：只改 RAM，不产生任何 EEPROM 写入**——命令处理全程处于落盘抑制态
  （`analog_set_persist_suppress(true)` 包住整个写入），状态机与级联即时生效，
  但脏位不置，`analog_task()` 不会因此落盘。落盘由 GUI 点"保存"经 `0xF6` 显式提交。
- `ki=0xFFFF` → 写**全局默认槽**：写入后由核心**级联刷新所有跟随全局键**的
  4 项阈值+RT 位，GUI 无需（也不应）逐键补写
- `flags.ACTUATION_OVERRIDE=0` → 该键**回到跟随全局**：取全局阈值与 RT 位、重新置
  `FOLLOW_GLOBAL`；**校准端点不动**（校准端点是本键物理量，与阈值无关）
- `flags.ACTUATION_OVERRIDE=1` → 该键**转为自定义**：写入 4 项阈值并清除 `FOLLOW_GLOBAL`
- 无论哪条分支，`raw_rest`/`raw_full` 都会按传入值写入校准端点（`0xF4` 之外显式写校准端点的
  另一条合法途径）。注意这些校准端点写入同样**暂不落盘**，等 `0xF6` 提交
- 写权限：与 Vial 其它写命令一致，**不要求 unlock**——unlock 只拦"改键位定义/动态配置"。
  Vial 经 WebHID 直连本机，非远程攻击面。

### 0xF3 `vial_analog_get_key_readings` —— 实时行程批量上报（可视化用）

- req: `[FE][F3][start_lo][start_hi]`
- resp:
  - `msg[0]` = 本包条数 `n`（窄 `0..10` / 宽 `0..7`）
  - 条目 `i` 从 `msg[1 + i*E]` 开始，`E` = 单条字节数（窄 3 / 宽 4）：
    - 窄 `E=3`：`[sw][raw_lo][raw_hi]`（`sw` 单字节）
    - 宽 `E=4`：`[sw_lo][sw_hi][raw_lo][raw_hi]`（`sw` 小端 2 字节）
    - 两种宽度下 `raw` 都是小端 16 位，且都排在 `sw` 之后
  - 条目从 `start_ki` 起连续排列
- 取值来源：`raw` = kb 钩子 `analog_backend_get_raw_adc(ki)`；
  `sw` = `analog_model_sw(ki, raw)` **即时换算**（非读取状态机内部值，故与触发判定同源）
- kb 后端不可用（`raw < 0`）该条记 `sw=0, raw=0`
- `start_ki >= num_keys` → `n=0`（GUI 据此结束轮询）
- GUI 轮询：从 ki=0 起连续请求直到 `n=0`，刷新率由 GUI 控制（建议 60–100Hz 分批）
- 节能：`flags.CONTINUOUS` 当前**不参与**本命令节流（核心状态机不读它，仅持久化+回报）。
  该位是预留：GUI 既不设置也不展示，仅在 `.vil` 导出/导入时原样保留。
  将来若实现节流（只上报置位了的键），须同时 bump 协议版本并更新 §2 版本史表。

### 0xF4 `vial_analog_calibrate` —— 校准采样

- req: `[FE][F4][mode][ki_lo][ki_hi]` ← `mode` 在 `msg[2]`，`ki` 在 `msg[3..4]`
- `ki=0xFFFF` = 全部键
- resp: `msg[0]` = 错误码

  | mode | 动作 | 写入字段 |
  |------|------|----------|
  | 0 | `SAMPLE_REST` | 以各键当前 `raw` 写 `top_reading`（要求用户松开所有键）|
  | 1 | `SAMPLE_FULL` | 以各键当前 `raw` 写 `bottom_reading`（要求用户按下到底）|
  | 2 | `RESET_CAL` | 校准端点恢复编译期默认校准值 `ANALOG_TOPREADING_MAX`/`ANALOG_BOTTOMREADING_MIN` |
  | 3 | `AUTO_PEAK` | **未实现**，返回错误码 1（`caps` 不报 `AUTO_CAL` 位）|
  | 4 | `BOTTOM_OUT_ON` | 开**触底校准模式**（纯运行态，不落盘）：扫描侧抑制全部键输出
        （等效 `KC_NO`）、状态机不推进，同时把"比当前 `bottom_reading` 更深"的读数
        喂回 `bottom_reading`（只允许推高）|
  | 5 | `BOTTOM_OUT_OFF` | 关触底校准模式，恢复正常输出 |

  错误码：`0` 成功；`1` 未知模式或参数越界；`2` kb 后端不可用（全部键 `raw<0`）
- `mode=0/1` 成功时 `msg[1..2]` = 本次首个成功采样键的 `raw`（小端），便于 GUI 即时显示
- `mode=2` 恢复的是**默认校准值**而非 0/255：校准端点在原始 ADC 域，写 0 会让键程模型失效
  直到下次开机重采
- `mode=4/5` 只看 `mode`、忽略 `ki`；当前状态随 `0xF0` 的 `msg[7]` 回报
- **触底校准模式语义**：GUI 把"重新校准触底读数"做成开关——开启期间用户逐个把每个键
  按到底（此时键盘没有任何输出，不会误触），扫描侧持续把各键 `bottom_reading` 顶到最深；
  关闭即结束。模式**纯运行态**：不进 EEPROM、开机默认关，上一次忘关也不会把键盘留成
  "砖"。扫描期间 `last_absv` 照常更新，所以 `0xF3` 的行程/读数在模式内仍然有效
- **校准不变量（校准端点安全边界）**：`analog_set_top_reading` / `analog_set_bottom_reading`
  对入参做 **range clamp** 到 kb 声明的校准端点区间——`ANALOG_TOPREADING_MIN`（未定义则取
  `1`）`<= top_reading <= ANALOG_TOPREADING_MAX`、`ANALOG_BOTTOMREADING_MIN <=
  bottom_reading <= ANALOG_BOTTOMREADING_MAX`，越界值视为噪声回落到边界。典型越界
  来源是"触底校准时有个别键没按"，那批键读到的是静置值，若收下会把 `bottom` 压到
  `top` 附近甚至倒挂，直接造成误触发。区间两端宏（`TOPREADING_MAX`/`BOTTOMREADING_MIN`）
  就是默认校准值，故出厂态恰好压在边界上；因为 `BOTTOMREADING_MIN > TOPREADING_MAX`
  （`analog_core.h` 有静态断言），clamp 之后 `top < bottom` 恒成立。区间倒挂
  （`TOPREADING_MAX < TOPREADING_MIN` 等）由 `analog_core.c` 的静态断言在编译期拦下。
  `persist_load` 也过同一个 clamp（旧固件可能写过越界校准端点）

### 0xF5 `vial_analog_reset_key` —— 复位键配置

- req: `[FE][F5][ki_lo][ki_hi]`
- resp: `msg[0]=0` 成功 / `1` 失败（越界）
- 单键（`ki` 有效）：**回全局并重新跟随**——4 项阈值与 RT 位取全局槽内容、置
  `FOLLOW_GLOBAL`；**保留本键校准端点**（`top_reading` 每次开机重采，`bottom_reading`
  是本键物理量）
- `ki=0xFFFF`：**出厂重置**——全局槽 + 所有键全部回编译期默认值，**连校准端点一起**回
  `ANALOG_TOPREADING_MAX`/`ANALOG_BOTTOMREADING_MIN`，并**立即落盘**（不等防抖）

### 0xF6 `vial_analog_persist_commit` —— 显式保存

- req: `[FE][F6][00]`（无参数）
- resp: `msg[0]=0` 成功
- 动作：`analog_persist_commit()` → 全量提交——写全局段 + **每条**键记录 + 重写头校验和，
  等价于出厂重置那条"立即落盘"路径。**不做参数差异**：提交的是 RAM 当前态。
- 磨损：逐条记录经 `eeprom_update_block` 读比对写，内容与 EEPROM 相同则不产生擦写；
  跟随全局键的记录恒等于全局推导值，重复保存不额外磨损 flash。
- GUI 语义：版本等值匹配通过即支持本命令，"保存到 EEPROM"按钮可用；拖动期间 `0xF2`
  零写入，点一次保存=一次提交。校准/出厂重置不依赖本命令（固件侧自带即时落盘）。

---

## 3. EEPROM 持久化布局

区由 `quantum/nvm/eeprom/nvm_eeprom_analog_internal.h` 统一描述，**分配链与寻址共用它**，
kb `config.h` 无需参与：

```c
#define VIAL_ANALOG_EEPROM_SIZE (ANALOG_PERSIST_SIZE)              /* ANALOG_MODEL 未声明时为 0 */
#define VIAL_ANALOG_EEPROM_ADDR (TOTAL_EEPROM_BYTE_COUNT - VIAL_ANALOG_EEPROM_SIZE)
```

- **缩让动作**在 `nvm_dynamic_keymap.c`：动态宏区尾部让出
  `VIAL_ANALOG_EEPROM_SIZE` 字节，并静态断言宏区剩余 ≥100 字节。
  不声明 `ANALOG_MODEL` 时为 0，EEPROM 布局与原版完全一致。
- **区首锚在 EEPROM 末尾向前数**，不依赖 VIA/动态键位是否启用。

### 3.1 布局与偶地址约定

区总长由 `ANALOG_PERSIST_SIZE` 推导，三段都随行程域宽度变化：

| 段 | 字节 | 说明 |
|----|------|------|
| 头 | 8 | 定长，见 §3.2 |
| 记录 | `ANALOG_PERSIST_RECORD_BYTES` × `num_keys` = 8 或 12 × `num_keys` | 每键一条，`record_bytes = sizeof(analog_record_t)` |
| 全局 | `sizeof(analog_global_t)` = 6（窄）/ 10（宽）| 全局槽 |
| **合计** | **`ANALOG_PERSIST_SIZE` = 头 + 记录 + 全局** | 窄宽各自恒为偶 |

区首 = `TOTAL - SIZE`，故 **SIZE 为偶 ⇒ 区首为偶 ⇒ 每条 8/12 字节记录都落在偶地址**。
这个偶性是靠"三段字节数各自为偶"**天然成立**的，`analog_core.h` §9 用四条静态断言把关
（头为偶、记录为偶、全局段为偶、总长为偶），**不再用 `(SIZE+1)&~1` 掩码**——掩码只会
掩盖"尺寸为奇"这个事实，而真正要守的是区首地址为偶。

`analog_global_t` 的第 6/10 字节 `reserved` 就是为此补齐的（写盘恒 0）。

实算示例（96 键 = 6×16，`TOTAL_EEPROM_BYTE_COUNT = 4096`，例如 tl96mgf072
走 STM32F072xB 的 vendor EEPROM 仿真：`FEE_PAGE_COUNT(4) × FEE_PAGE_SIZE(2048) / 2`）：

- 窄（`M <= 255`）：`SIZE = 8 + 96×8 + 6 = 782`，`ADDR = 4096 - 782 = 3314`，正好贴住末尾。
- 宽（`M > 255`）：`SIZE = 8 + 96×12 + 10 = 1170`，`ADDR = 4096 - 1170 = 2926`。

其余板按 `ANALOG_PERSIST_SIZE` 与各自的 `TOTAL_EEPROM_BYTE_COUNT` 自行推导，本文不列。

### 3.2 头部与记录

```c
typedef struct {          // 8 字节头（定长）
    uint32_t magic;       // 0x474E4156 "VANG"，小端
    uint8_t  version;     // VIAL_ANALOG_PROTOCOL_VERSION = 1（空口线格式与 EEPROM 落盘布局共用同一编号）
    uint8_t  num_keys;    // 单字节（静态断言 num_keys ≤ 255）
    uint8_t  record_bytes;// sizeof(analog_record_t)：8（窄）/ 12（宽）
    uint8_t  checksum;    // 数据段(记录+全局)逐字节 XOR
} analog_persist_header_t;

typedef struct {          // 记录：8 字节（窄）/ 12 字节（宽），字段顺序即落盘顺序
    analog_travel_t actuation_threshold;  // 窄 uint8_t / 宽 uint16_t
    analog_travel_t release_threshold;
    analog_travel_t actuation_offset;
    analog_travel_t release_offset;
    uint16_t bottom_reading;   // 小端
    uint8_t  flags;
    uint8_t  reserved;
} analog_record_t;
```

- **版本编号统一**：空口线格式与 EEPROM 落盘布局**共用同一个编号** `VIAL_ANALOG_PROTOCOL_VERSION`
  （真源 `quantum/analog/analog_core.h`，见 §2）。bump 一次即同时改变两侧：旧编号的 EEPROM 区
  在下面这条 `version` 判据上**整区作废**并回退到出厂默认（校准端点回默认校准值）。
  「两个宏语义无关、取值不同」的旧说明已作废，不再成立。
- **不持久化**：`top_reading`（每次开机重采）、`extremum`（运行态）。
- **跟随全局键不冗余存储**：它们的 4 项阈值加载时从全局记录推导；只有全局记录进落盘数据。
- **作废条件**：magic/version/`num_keys`/`record_bytes` 任一不符，或校验和不过 → 整区作废，
  开机写回出厂默认（`analog_init` 的 `persist_load()` 失败路径）。
  `version` 写的就是 `VIAL_ANALOG_PROTOCOL_VERSION`(= 1)，`record_bytes` 随宽度写成 8/12；
  两者任一不符（含此前开发期写过的其它编号）开机即被这条判据整区作废重写，
  不会读到错位的旧数据。
- **落盘时机**：`analog_task()`（由 `housekeeping_task` 每循环调用）检查脏位图，**防抖
  `ANALOG_PERSIST_FLUSH_MS`（默认 500 ms）** 后一次性提交脏记录 + 全局 + 重写头校验和。
  实时校准会在扫描里反复推高 `bottom_reading`，逐次写会撑爆 FEE 写日志，故必须攒批。
  显式用户动作（`0xF5` 出厂重置、`0xF6` 保存）走 `persist_flush_all()` 立即落盘。
- **落盘抑制（`0xF2` 调参）**：`0xF2` 调参全程包在 `analog_set_persist_suppress(true)` 里，脏位
  不置 → 拖动期间零 flash 写入，直到 GUI 点"保存"发 `0xF6`。抑制只包住 0xF2 的
  处理窗口，校准/复位/kb 实时校准（推高 `bottom_reading`）照旧即时标脏；若一次
  校准触发的落盘顺带持久化了此前未保存的阈值改动，属预期内行为（不丢数据、不额外磨损）。

---

## 4. 固件侧 API（`quantum/analog/analog_core.h`）

```c
/* 生命周期 */
void     analog_init(void);                       // 填出厂默认 + 加载 EEPROM(失败则写回)；重算模型派生参数
void     analog_task(void);                       // 周期落盘入口(脏位 + 防抖)

/* 推送式状态机(kb 扫描逐键调用) */
bool     analog_step_key(uint16_t ki, analog_travel_t sw); // 返回 true = 按下状态翻转
bool     analog_get_pressed(uint16_t ki);

/* 校准端点(会回调模型层重算派生参数) */
void     analog_set_top_reading(uint16_t ki, uint16_t value);
void     analog_set_bottom_reading(uint16_t ki, uint16_t value);

/* 配置 */
void     analog_set_key_config(uint16_t ki, const analog_travel_t params[4], bool rt_on); // 清 FOLLOW_GLOBAL
void     analog_set_global(const analog_global_t *g);                            // 级联刷新跟随键
bool     analog_reset_key(uint16_t ki);            // ki=0xFFFF 出厂重置
bool     analog_key_is_customized(uint16_t ki);
void     analog_mark_dirty(uint16_t ki);           // 协议层直接改 flags 后标脏
void     analog_set_persist_suppress(bool on);     // on 时 mark_dirty/级联标脏全部吞掉(0xF2 调参用)
void     analog_persist_commit(void);              // 全量立即落盘(0xF6"保存"按钮)

/* 触底校准模式(运行态，见 0xF4 mode4/5) */
void     analog_set_bottom_out_mode(bool on);
bool     analog_get_bottom_out_mode(void);
void     analog_force_release(uint16_t ki);        // 只清按下位，不通知模型层

/* 单键实时跟踪(核心只留一份 sw，不常驻 sw[96]) */
void     analog_set_tracked_key(uint16_t ki);
analog_travel_t analog_get_tracked_sw(void);
int16_t  analog_get_tracked_raw(void);

/* 键程模型层标准钩子(全 weak，kb 可强覆盖) */
analog_travel_t analog_model_sw(uint16_t ki, uint16_t absv);
void     analog_backend_calibration_changed(uint16_t ki, uint16_t top, uint16_t bottom);

/* kb 实现：返回该键最近一次真实 ADC 读数；<0 = 不可用。weak 默认 -1 */
int16_t  analog_backend_get_raw_adc(uint16_t ki);
```

实例：`g_analog_key[ANALOG_NUM_KEYS]`、`g_analog_global`、`g_analog_pressed_bits[]`、
`g_analog_tracked_key`。

**行程域钳位**：核心只认 `0..ANALOG_MAX_TRAVEL` 这一个值域，所有进入行程域的写入都过
`analog_core.c` 内部的 `clamp_travel()`——EEPROM 装载时的全局 4 项与每键 4 项阈值、
`analog_set_key_config()` 的入参、`analog_set_global()` 的级联源（先钳再级联，避免把
越界值复制到所有跟随键）。它比较的是 `ANALOG_MAX_TRAVEL`，**不是** `analog_travel_t`
的类型上限：最大键程值小于类型上限时（例如 M=300 而类型已经是 `uint16_t`）类型本身拦不住
越界值，而越界的阈值会让该键永远触发不了。校准端点（`top_reading`/`bottom_reading`）不走这
条钳位，它们是原始 ADC 域物理量，另有 §0xF4 的校准端点区间 clamp。

### 4.1 键程模型选编

模型选编由**构建系统**完成：kb `rules.mk` 声明 `ANALOG_MODEL = <name>`，`build_vial.mk` 据此把 `analog_model_<name>.c` 编入 SRC（`analog_core.c` 只依赖 `analog_core.h` 里的钩子声明，不再 `#include` 模型头）：

- `ANALOG_MODEL = isf` → 编入 `analog_model_isf.c`（平方反比-快速，磁轴）
  - 模型 `absv = k / (d - sw)²` ⇒ `sw = D - K·V`，其中
    `D`（映射参数D）`= M·t/(t-1)`、`t = sqrt(bottom/top)`；
  - `K`（映射系数K）`= sqrt(top)·D` 是**逐键**量，`D`、`K` 都在校准回调
    `analog_backend_calibration_changed()` 里重算；
  - `V[i] = 1/sqrt(absv)`（表项内平均）是编译期常量表，热路径用定点：
    `V` 存 `round(2^F/sqrt(absv))`（`uint16_t`），
    `sw = D - ((K·V + 2^(F-1)) >> F)`（`int32` 中间量）。
    相比旧"`K/=SCALE`、`V=SCALE/sqrt`"方案，`K` 不再因除 `SCALE` 丢低位、
    `V` 用 `2^F` 定标（窄域 F=15 时填满 `uint16`），舍入误差显著下降；
  - `F`、`V` 表、`D/K` 出厂默认值与 `isf_scalar_t` 类型都由
    `util/analog_isf_gen.py` 按 kb 校准端点区间生成，产物 `analog_isf_table.inc`
    随构建落到 `$(INTERMEDIATE_OUTPUT)/src/`；
  - `D/K` 最大值落在**极端点**（`top=TOPREADING_MAX`、`bottom=BOTTOMREADING_MIN`）——
    它必为最坏工况：`D/K` 在校准端点区间上单调，极值只能取在端点组合处，
    生成器据此决定 `isf_scalar_t` 是 `int16_t` 还是 `int32_t`，并选 `F ≤ 15`
    使 `K·V + 2^(F-1)` 恒不溢出 `int32`；`V` 恒为 `uint16_t`；
  - 校准回调只接受合法校准端点域（`top ≤ ANALOG_TOPREADING_MAX` 且
    `bottom ≥ ANALOG_BOTTOMREADING_MIN`，见 `analog_core.h` §6）。该域同时把
    `d` 界住（域内 `t ≥ sqrt(BOTTOMREADING_MIN/TOPREADING_MAX)`），因此不再需要
    对 `d` 单独做数值钳位。**无效读数保留上一次的校准值**，不清零——
    清零会让该键退化成恒定输出；
  - 派生参数类型（`isf_scalar_t`）由生成器按最坏评估采样点 `D/K` 量级选择：`int16_t` / `int32_t`。
- `ANALOG_MODEL = linear_fast` → 编入 `analog_model_linear_fast.c`
  （线性，与 `linear` 同曲线；校准时预算 8.8 定点倒数乘子 K[ki]，扫描里用
  "乘法+移位"替代 32 位除法，Cortex-M0 上省掉一次软除法。精度 ≤1 LSB。）
  - `K[ki] = round((M << 8) / span)`，`span = bottom - top`，`M = ANALOG_MAX_TRAVEL`；
  - `K` 的类型随宽度选：窄 `uint16_t` / 宽 `uint32_t`（`linfast_k_t`），
    热路径 `((absv-top) * K) >> 8` 用 `uint32_t` 中间量，封顶到 `M`（不是 255）。
- `ANALOG_MODEL = linear` → 编入 `analog_model_linear.c`：
  `sw = clamp((absv - top) * M / (bottom - top), 0, M)`（`uint32_t` 中间量）

每个模型 `.c` 各自以 **weak** 定义两个钩子（`analog_model_sw` /
`analog_backend_calibration_changed`），声明在 `analog_core.h`；kb `.c` 可强符号
覆盖单个钩子（ELF 语义）。模型互斥由构建保证（`ANALOG_MODEL` 只能取一个值）。

---

## 5. kb 接入清单

1. `rules.mk`：`ANALOG_MODEL = <name>`（启用本扩展与核心层；name 取 `isf`/`linear_fast`/`linear`）。
   它同时派生上游 `ANALOG_DRIVER_REQUIRED`，让 QMK 编入平台 ADC 驱动（`adc_read`/`pinToMux`）。
2. `config.h`：定义 `ANALOG_TOPREADING_MAX`/`ANALOG_BOTTOMREADING_MIN`（默认校准值，即出厂静置/触底校准端点，
   必需）；`ANALOG_MAX_TRAVEL`（最大键程值，见 §1.5）。ISF 还可给校准端点区间
   `ANALOG_TOPREADING_MIN`/`ANALOG_BOTTOMREADING_MAX`（仅生成器入参）。
3. kb `matrix.c` 扫描里逐键：`adc_read` → `absv` → `analog_model_sw()` →
   `analog_step_key()` → 据返回值翻矩阵位；实时校准调 `analog_set_top/bottom_reading()`。
4. 实现 `analog_backend_get_raw_adc()`（供 `0xF3` 显示原始值）。
5. 若用 `0xF4` mode4/5 触底校准开关：扫描里在 `analog_get_bottom_out_mode()` 为真时
   跳过 `analog_step_key()`，改为清矩阵位 + `analog_force_release(ki)`，并用
   `analog_set_bottom_reading()` 只推高本键 `bottom_reading`（详见参考实现）。

参考实现：`keyboards/bvbhu/tl96mgf072/tl96mgf072_matrix.c`。
> `0xF0`–`0xF6` 全部命令都在 `quantum/vial.c` 的 `#ifdef ANALOG_MODEL` 段内，
> 未启用 analog 的板子不会编入这些代码。

---

## 6. 无线兼容（BHQ）

- 协议包走标准 Vial raw HID。下行：BHQ 空口数据经 `wireless.c` 喂入 `raw_hid_receive()`，
  其中 `via_command_bhq()`（VIA 顶层命令 0xF1/0xF2，USB/OTA 切换）仅在 `raw_hid_receive()`
  的 switch 之前做一次 early-return 拦截，与本扩展的 Vial 子命令（`msg[0]=0xFE` 命名空间）
  正交；上行：`raw_hid_send()` 已经 `bluetooth_send_raw_hid()` → `bhq_send_hid_raw()`
  自动路由到空口。本扩展命令**无需额外改动**即可经蓝牙透传。
- 带宽：`0xF3` 每包窄域 10 键 / 宽域 7 键；96 键窄域需 10 包/帧、宽域需 14 包/帧。
  60Hz 刷新下窄域约 600 包/s，BT 下需实测。

