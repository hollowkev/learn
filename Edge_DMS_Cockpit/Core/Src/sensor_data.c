#include "sensor_data.h"
#include "r60abd1.h"
#include "usart.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bsp_stepper.h"

/* ========== UART1 接收缓冲区 ========== */

static uint8_t  uart1_rx_buf[UART1_RX_BUF_SIZE];
static uint16_t uart1_rx_idx = 0;
static uint8_t  uart1_rx_byte;
static volatile uint8_t uart1_cmd_ready = 0;

/* ========== 传感器全局变量定义 ========== */

uint8_t  g_emergency_mode   = 0;
static uint32_t left_behind_timer = 0; // 遗留人员计时器 (秒)

uint8_t  g_radar_presence   = 0;
float    g_radar_breath     = 0.0f;
float    g_radar_heart      = 0.0f;
float    g_radar_distance   = 0.0f;

float    g_gas_co2          = 0.0f;
float    g_gas_smoke        = 0.0f;

uint8_t  g_fsr_wheel        = 0;
uint8_t  g_fsr_seat         = 0;

float    g_sht20_temp       = 0.0f;
float    g_sht20_humi       = 0.0f;

uint8_t  g_act_fan          = 0;
uint8_t  g_act_heater       = 0;
uint8_t  g_cooling_chip     = 0;
uint8_t  g_act_window       = 0;
uint8_t  g_act_ignition     = 0;
uint8_t  g_act_buzzer       = 0;

/* ========== 传感器数据更新函数 ========== */





/* ========== JSON 组包并通过 UART 透传发送 ========== */

void Sensor_ReportStatusJson(void)
{
    char json_buf[360];
    int len;

    len = sprintf(json_buf, "{\"radar_presence\":%u,\"radar_breath\":%.1f,\"radar_heart\":%.1f,\"radar_distance\":%.2f,\"gas_co2\":%.1f,\"gas_smoke\":%.1f,\"fsr_wheel\":%u,\"fsr_seat\":%u,\"sht20_temp\":%.1f,\"sht20_humi\":%.1f,\"act_fan\":%u,\"act_heater\":%u,\"cooling_chip\":%u,\"act_window\":%u,\"act_ignition\":%u,\"act_buzzer\":%u}\r\n",
        g_radar_presence,
        g_radar_breath,
        g_radar_heart,
        g_radar_distance,
        g_gas_co2,
        g_gas_smoke,
        g_fsr_wheel,
        g_fsr_seat,
        g_sht20_temp,
        g_sht20_humi,
        g_act_fan,
        g_act_heater,
        g_cooling_chip,
        g_act_window,
        g_act_ignition,
        g_act_buzzer
    );
    if (len > 0 && len < (int)sizeof(json_buf))
    {    
				HAL_UART_Transmit_DMA(&huart1, (uint8_t *)json_buf, (uint16_t)len);
    }
}

/* ========== UART1 接收和控制函数实现 ========== */

void UART1_InitReceive(void)
{
    uart1_rx_idx = 0;
    uart1_cmd_ready = 0;
    memset(uart1_rx_buf, 0, UART1_RX_BUF_SIZE);
    HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1);
}

void UART1_IRQHandler_Callback(void)//处理接收到的控制命令，并存到缓冲区
{
    // 调试打印：严禁在中断中使用 printf，会导致串口 ORE 溢出死锁！
    // printf("RxByte: %c\r\n", uart1_rx_byte);
    
    if (uart1_rx_idx < UART1_RX_BUF_SIZE - 1)
    {
        uart1_rx_buf[uart1_rx_idx++] = uart1_rx_byte;
        
        if (uart1_rx_byte == '\n')
        {
            uart1_rx_buf[uart1_rx_idx] = '\0';
            uart1_cmd_ready = 1;
        }
    }
    else
    {
        uart1_rx_idx = 0;
        memset(uart1_rx_buf, 0, UART1_RX_BUF_SIZE);
    }
    
    HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1);
}

void UART1_ProcessRxData(void)
{
    if (uart1_cmd_ready)
    {
        printf("%s",uart1_rx_buf);
        UART1_ParseCommand((char *)uart1_rx_buf);
        uart1_rx_idx = 0;
        uart1_cmd_ready = 0;
        memset(uart1_rx_buf, 0, UART1_RX_BUF_SIZE);
    }
}

void UART1_ParseCommand(char *cmd)
{
    char actuator[32];
    int value;
    char *cmd_ptr = cmd;
    char *cmd_start;
    
    while (1)
    {
        cmd_start = strstr(cmd_ptr, "CMD:");
        if (cmd_start == NULL)
        {
            break;
        }
        
        char *equal_sign = strchr(cmd_start + 4, '=');
        if (equal_sign == NULL)
        {
            cmd_ptr = cmd_start + 4;
            continue;
        }
        
        int name_len = equal_sign - (cmd_start + 4);
        if (name_len <= 0 || name_len >= (int)sizeof(actuator))
        {
            cmd_ptr = equal_sign + 1;
            continue;
        }
        
        strncpy(actuator, cmd_start + 4, name_len);
        actuator[name_len] = '\0';
        
        if (sscanf(equal_sign + 1, "%d", &value) != 1)
        {
            cmd_ptr = equal_sign + 1;
            continue;
        }
        
        if (strcmp(actuator, "fan") == 0)
        {
            if (value == 0 || value == 1)
            {
                HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
        else if (strcmp(actuator, "heater") == 0)
        {
            if (value == 0 || value == 1)
            {
                HAL_GPIO_WritePin(RLY1_GPIO_Port, RLY1_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
        else if (strcmp(actuator, "cooling") == 0)
        {
            if (value == 0 || value == 1)
            {
                HAL_GPIO_WritePin(RLY2_GPIO_Port, RLY2_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
        else if (strcmp(actuator, "window") == 0)
        {
            if (value >= 0 && value <= 100)
            {
                BSP_Stepper_SetWindowPosition(value);
            }
        }
        else if (strcmp(actuator, "ignition") == 0)
        {
            if (value == 0 || value == 1)
            {
                HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
        else if (strcmp(actuator, "buzzer") == 0)
        {
            if (value == 0 || value == 1)
            {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
        
        cmd_ptr = equal_sign + 1;
    }
}

void UART1_SendCommand(const char *actuator, int value)
{
    char cmd[64];
    
    sprintf(cmd, "CMD:%s=%d\r\n", actuator, value);
    UART1_ParseCommand(cmd);
}

/* ========== 紧急安全保底逻辑实现 ========== */

void Sensor_CheckEmergency(void)
{
    // 判断是否有人 (雷达检测到有人 或 座椅压力检测到有人)
    uint8_t has_people = (g_radar_presence == 1) || (g_fsr_seat == 1);
    
    // 判断是否有危险气体
    uint8_t co2_danger = (g_gas_co2 > CO2_DANGER_THRESHOLD);
    uint8_t smoke_danger = (g_gas_smoke > SMOKE_DANGER_THRESHOLD);

    // 判断生命体征异常 (前提是雷达数据有效，即 > 0)
    uint8_t heart_danger = (g_radar_heart > 0.0f) && 
                           (g_radar_heart < HEART_RATE_LOW_THRESHOLD || g_radar_heart > HEART_RATE_HIGH_THRESHOLD);
    
    // 为了避免雷达丢信号导致误报，仅判断呼吸率过高的情况
    uint8_t breath_danger = (g_radar_breath > 0.0f) && 
                            (g_radar_breath > BREATH_RATE_HIGH_THRESHOLD);

    // 遗留人员判断：熄火状态 且 有人
    // 注意：Sensor_CheckEmergency 在 freertos.c 中每 1000ms (1秒) 被调用一次
    if (g_act_ignition == 0 && has_people) {
        left_behind_timer++;
    } else {
        left_behind_timer = 0; // 点火或没人时，计时器清零
    }
    uint8_t left_behind_danger = (left_behind_timer >= LEFT_BEHIND_TIME_THRESHOLD);

    // 综合危险标志
    EmergencyType_t current_emergency = EMERGENCY_NONE;

    // 优先级判断：烟雾(火灾) > 生命体征异常 > CO2超标 > 遗留人员
    if (smoke_danger) {
        current_emergency = EMERGENCY_SMOKE;
    } else if (heart_danger || breath_danger) {
        current_emergency = EMERGENCY_VITAL_SIGNS;
    } else if (co2_danger) {
        current_emergency = EMERGENCY_CO2;
    } else if (left_behind_danger) {
        current_emergency = EMERGENCY_LEFT_BEHIND;
    }

    if (current_emergency != EMERGENCY_NONE)
    {
        if (has_people)
        {
            // 有人 且 环境/生理危险：触发紧急避险
            g_emergency_mode = 1;
            Sensor_EmergencyHandler(current_emergency);
        }
        else
        {
            // 无人 且 危险：
            // 预留给你后续写其他判断逻辑（例如发紧急通知但不一定开窗）
            g_emergency_mode = 0; 
            
            // 如果是火灾，即便没人也应该触发一些操作
            if (current_emergency == EMERGENCY_SMOKE) {
                // TODO: 烟雾报警，即便没人也要处理
            }
        }
    }
    else
    {
        // 环境与生理安全，解除紧急模式
        g_emergency_mode = 0;
    }
}

void Sensor_EmergencyHandler(EmergencyType_t type)
{
    // 强制开启蜂鸣器报警 (所有紧急情况都开启)
    g_act_buzzer = 1;
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);

    switch (type)
    {
        case EMERGENCY_SMOKE:
            // 烟雾超标：强制断电/熄火、全开窗逃生，但不强制开风扇(防止助长火势)
            g_act_ignition = 0;
            HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
            
            g_act_window = 100;
            BSP_Stepper_SetWindowPosition(g_act_window);
            break;
            
        case EMERGENCY_VITAL_SIGNS:
            // 生命体征异常：强制开风扇换气、半开窗
            g_act_fan = 1;
            HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
            
            if (g_act_window < 50) {
                g_act_window = 50;
                BSP_Stepper_SetWindowPosition(g_act_window);
            }
            break;
            
        case EMERGENCY_CO2:
            // CO2超标：强制开风扇换气、半开窗
            g_act_fan = 1;
            HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
            
            if (g_act_window < 50) {
                g_act_window = 50;
                BSP_Stepper_SetWindowPosition(g_act_window);
            }
            break;
            
        case EMERGENCY_LEFT_BEHIND:
            // 遗留人员：防止窒息和高温，开风扇换气，半开窗，可以考虑打开冷气(若硬件允许熄火开冷气)
            g_act_fan = 1;
            HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
            
            if (g_act_window < 50) {
                g_act_window = 50;
                BSP_Stepper_SetWindowPosition(g_act_window);
            }
            
            // 可选：如果熄火状态下制冷片还能工作，可以打开降温
            // g_cooling_chip = 1;
            // HAL_GPIO_WritePin(RLY2_GPIO_Port, RLY2_Pin, GPIO_PIN_SET);
            break;
            
        default:
            break;
    }
}