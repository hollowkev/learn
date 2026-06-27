#ifndef __BSP_STEPPER_H
#define __BSP_STEPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ========== 步进电机相关宏定义 ========== */
// 假设使用 28BYJ-48 步进电机，减速比 1:64，一圈约 4096 步
#define STEPPER_MAX_STEPS 4096  // 车窗全开(100%)需要的总步数

// 步进电机控制引脚定义 (匹配 CubeMX 的命名)
// 请确保在 CubeMX 里配置的引脚标签为 M1, M2, M3, M4
#define IN1_PORT M1_GPIO_Port
#define IN1_PIN  M1_Pin

#define IN2_PORT M2_GPIO_Port
#define IN2_PIN  M2_Pin

#define IN3_PORT M3_GPIO_Port
#define IN3_PIN  M3_Pin

#define IN4_PORT M4_GPIO_Port
#define IN4_PIN  M4_Pin

/* ========== 函数声明 ========== */

/**
 * @brief  步进电机初始化
 */
void BSP_Stepper_Init(void);

/**
 * @brief  设置车窗开度
 * @param  percentage: 目标开度百分比 (0-100)，0表示全关，100表示全开
 */
void BSP_Stepper_SetWindowPosition(uint8_t percentage);

/**
 * @brief  获取当前车窗开度
 * @return 当前开度百分比 (0-100)
 */
uint8_t BSP_Stepper_GetWindowPosition(void);


#ifdef __cplusplus
}
#endif

#endif /* __BSP_STEPPER_H */
