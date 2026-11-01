/* 模拟矩阵 ADC 分辨率设为12位(0-4095)
 *
 * 注意写法：F072 走 ADCv1，头文件里有 ADC_CFGR1_RES_12BIT 这个符号；
 * F411 走 ADCv2，那个符号不存在，analog.c 的 ADCv2 分支把
 * ADC_CFGR1_RES_12BIT 等改定义成了纯数字 12/10/8/6（见
 * platforms/chibios/drivers/analog.c:76 的 bodge），F4 的 ADC 本身也是 12 位固定。
 * 故这里直接写数值 12 —— analog_core.h §6.5 与 util/analog_isf_extract.py
 * 都认「标识符或数字」两种写法。写 ADC_CFGR1_RES_12BIT 在 F411 上会让
 * #if ADC_RESOLUTION == ... 的两个未定义标识符各自取 0 而静默判成 12 位，
 * 一旦哪天默认值变了就会算错位宽，所以这里用明确的数字。 */
#define ADC_RESOLUTION 12

/* 实时校准：读数持续偏离校准读数超过它才更新 */
#define CALIBRATION_THRESHOLD 50


/* ---- kb 出厂默认 ----
 * 阈值/RT 是行程域内的无量纲约定值，可沿用核心层兜底。
 * 校准端点是磁轴的初始校准读数/触底校准读数的原始 ADC 值——**本板相关物理量，必须在本文件定义**，
 * 核心层不做兜底(缺定义直接编译报错)。取参考实现的典型值。 */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 200
#define ANALOG_DEFAULT_RELEASE_THRESHOLD   192
#define ANALOG_TOPREADING_MIN    200
#define ANALOG_TOPREADING_MAX    350
#define ANALOG_BOTTOMREADING_MIN 700
#define ANALOG_BOTTOMREADING_MAX 850

/* Vial 显示的最大键程值 */
#define ANALOG_MAX_TRAVEL 255

/* WS2812 时序参数 */
#define WS2812_T0H 200
