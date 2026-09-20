/* 模拟矩阵 ADC 分辨率设为12位(0-4095) */
#define ADC_RESOLUTION ADC_CFGR1_RES_12BIT

/* 实时校准：读数持续偏离锚点超过它才更新(防噪声抖锚点) */
#define CALIBRATION_THRESHOLD 50


/* ---- kb 出厂默认 ----
 * 阈值/RT 是行程域内的无量纲约定值，可沿用核心层兜底。
 * 锚点是 Hall 轴的静置/触底原始 ADC 读数——**本板相关物理量，必须在本文件定义**，
 * 核心层不做兜底(缺定义直接编译报错)。取参考实现的典型值。 */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 200
#define ANALOG_DEFAULT_RELEASE_THRESHOLD   192
#define ANALOG_TOPREADING_MIN    200
#define ANALOG_TOPREADING_MAX    350
#define ANALOG_BOTTOMREADING_MIN 700
#define ANALOG_BOTTOMREADING_MAX 850

/* Vial 显示的行程域满量程 */
#define ANALOG_MAX_TRAVEL 255

/* WS2812 时序参数 */
#define WS2812_T0H 200
