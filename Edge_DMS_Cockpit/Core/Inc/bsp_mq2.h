#ifndef __BSP_MQ2_H__
#define __BSP_MQ2_H__

#include "main.h"

// ==========================================================
// 【终极封装】：因为单片机上通常只有一个 MQ2 传感器，
// 所以我们把结构体和状态机彻底藏进 .c 文件里！
// 主函数再也不需要定义变量、传参数了，直接无脑调下面这几个函数！
// ==========================================================

// 初始化传感器并启动 ADC
HAL_StatusTypeDef BSP_MQ2_Init(void);

// 【主循环无脑调用】：内部自动处理 预热 -> 标定 -> 正常采样
void BSP_MQ2_Update(void);

// 获取当前的烟雾浓度，只有在 READY 状态下拿到的数据才是准的
float BSP_MQ2_GetSmokePPM(void);

// 检查传感器是否已经进入 READY 工作状态
uint8_t BSP_MQ2_IsReady(void);

#endif
