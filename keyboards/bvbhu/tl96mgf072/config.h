/* 模拟矩阵 ADC 分辨率设为12位(0-4095) */
#define ADC_RESOLUTION ADC_CFGR1_RES_12BIT

/* 实时校准：读数持续偏离锚点超过它才更新(防噪声抖锚点) */
#define CALIBRATION_THRESHOLD 50

/* 键程模型：平方反比-快速(磁轴)；不定义则默认线性映射 */
#define ANALOG_MODEL_ISF

/* ---- 出厂默认(覆盖 quantum/analog/analog_core.h 的通用值) ----
 * 阈值取参考实现；锚点是 Hall 轴的典型静置/触底读数 */
#define ANALOG_DEFAULT_ACTUATION_THRESHOLD 200
#define ANALOG_DEFAULT_RELEASE_THRESHOLD   192
#define ANALOG_DEFAULT_TOP_READING         375
#define ANALOG_DEFAULT_BOTTOM_READING      675

/* 持久化：analog 区直接编入 quantum/nvm/eeprom 的 EEPROM 分配链
 * (nvm_dynamic_keymap.c 把动态宏区尾部让出，大小按 ANALOG_PERSIST_SIZE
 * 自动推导)，板级无需配置。 */

/* WS2812 时序参数 */
#define WS2812_T0H 200
