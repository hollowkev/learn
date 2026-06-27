#include "bsp_stepper.h"
#include <stdlib.h>
//四相八拍
// 记录当前的车窗开度 (0-100)
static uint8_t current_window_pos = 0; 
// 记录当前的电气相位索引 (0-7)
static uint8_t current_phase_index = 0;

// 四相八拍驱动表
// 顺序: A, AB, B, BC, C, CD, D, DA
// 对应 IN1, IN2, IN3, IN4
const uint8_t phase_table[8] = {
    0x01, // 1: A
    0x03, // 3: A+B
    0x02, // 2: B
    0x06, // 6: B+C
    0x04, // 4: C
    0x0C, // 12: C+D
    0x08, // 8: D
    0x09  // 9: D+A
};

/**
 * @brief  底层引脚控制函数
 * @param  phase_val: 从 phase_table 取出的 4位控制值
 */
static void Set_Stepper_Pins(uint8_t phase_val)
{
    HAL_GPIO_WritePin(IN1_PORT, IN1_PIN, (phase_val & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_PORT, IN2_PIN, (phase_val & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN3_PORT, IN3_PIN, (phase_val & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_PORT, IN4_PIN, (phase_val & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief  步进电机初始化
 */
void BSP_Stepper_Init(void)
{
    // 假设引脚在 main.c/CubeMX 已经配置为 GPIO_Output

    // 1. 强行输出第0相的电平，让电机转子被磁力“吸”到已知的起点
    current_phase_index = 0;
    Set_Stepper_Pins(phase_table[current_phase_index]); 
    
    // 2. 延时给转子一点时间去对齐
    HAL_Delay(10); 
    
    // 3. (因为是光杆演示) 假设当前上电时的位置，就是车窗全关(0%)的位置
    current_window_pos = 0; 
}

/**
 * @brief  设置车窗开度
 * @param  percentage: 目标开度百分比 (0-100)
 */
void BSP_Stepper_SetWindowPosition(uint8_t percentage)
{
    // 参数保护
    if (percentage > 100) {
        percentage = 100;
    }

    // 如果目标位置和当前位置一样，不需要动作
    if (percentage == current_window_pos) {
        return;
    }

    // 计算需要转动的百分比差值
    int diff = percentage - current_window_pos;
    
    // 计算需要走的总步数 (使用在头文件中定义的 STEPPER_MAX_STEPS)
    // 假设 100% 对应 STEPPER_MAX_STEPS 步
    uint32_t steps_to_move = (abs(diff) * STEPPER_MAX_STEPS) / 100;
    
    // 根据差值的正负决定方向
    int8_t dir = (diff > 0) ? 1 : -1; // 1为开窗(正转)，-1为关窗(反转)

    for (uint32_t i = 0; i < steps_to_move; i++) {
        // 更新相位索引，使用 +8 再 %8 保证结果为正
        current_phase_index = (current_phase_index + dir + 8) % 8;
        
        // 输出对应的电平
        Set_Stepper_Pins(phase_table[current_phase_index]);
        
        // 延时，控制电机转速。通常28BYJ-48最快大概只能做到2ms每步，太快会堵转
        HAL_Delay(1); 
    }
    
    // 停止输出，防止电机发热烧毁 (重要!)
    Set_Stepper_Pins(0x00);

    // 更新当前位置
    current_window_pos = percentage;
}

/**
 * @brief  获取当前车窗开度
 * @return 当前开度百分比 (0-100)
 */
uint8_t BSP_Stepper_GetWindowPosition(void)
{
    return current_window_pos;
}
