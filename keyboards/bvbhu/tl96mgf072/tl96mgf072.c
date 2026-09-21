#include "tl96mgf072.h"

void keyboard_post_init_kb()
{
    calibrate_matrix(); // 校准矩阵：重置底部读数+采样初始校准读数+重计算参数
    keyboard_post_init_user();
}
