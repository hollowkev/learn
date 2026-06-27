#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== 传感器全局变量声明 ========== */

/* --- 雷达传感器 (来自 R60ABD1 毫米波雷达) --- */
extern uint8_t  g_radar_presence;       /* 是否有人: 0=无人, 1=有人 */
extern float    g_radar_breath;         /* 呼吸速率 */
extern float    g_radar_heart;          /* 心率 */
extern float    g_radar_distance;       /* 第一个目标的直线距离 (米) */

/* --- 气体传感器 --- */
extern float    g_gas_co2;              /* CO2浓度 (ppm) */
extern float    g_gas_smoke;            /* 烟雾浓度 */

/* --- FSR 薄膜压力传感器 (GPIO 数字量) --- */
extern uint8_t  g_fsr_wheel;            /* 方向盘握持: 0=未握, 1=握持 */
extern uint8_t  g_fsr_seat;             /* 座椅占位: 0=无人, 1=有人 */

/* --- SHT20 温湿度传感器 (I2C) --- */
extern float    g_sht20_temp;           /* 温度 (°C) */
extern float    g_sht20_humi;           /* 湿度 (%RH) */

/* --- 执行器状态 --- */
extern uint8_t  g_act_fan;              /* 风扇: 0=关, 1=开 */
extern uint8_t  g_act_heater;           /* 加热器: 0=关, 1=开 */
extern uint8_t  g_cooling_chip;         /* 制冷片: 0=关, 1=开 */
extern uint8_t  g_act_window;           /* 车窗开度: 0-100 (%) */
extern uint8_t  g_act_ignition;         /* 点火状态: 0=熄火, 1=点火 */
extern uint8_t  g_act_buzzer;           /* 蜂鸣器: 0=关, 1=开 */

/* ========== 紧急安全保底逻辑 ========== */
#define CO2_DANGER_THRESHOLD       2000.0f  /* CO2 危险阈值 (ppm) */
#define SMOKE_DANGER_THRESHOLD     800.0f   /* 烟雾 危险阈值 (根据MQ2实际标定调整) */

#define HEART_RATE_LOW_THRESHOLD   40.0f    /* 心率过低阈值 (bpm) */
#define HEART_RATE_HIGH_THRESHOLD  120.0f   /* 心率过高阈值 (bpm) */
#define BREATH_RATE_HIGH_THRESHOLD 30.0f    /* 呼吸率过高阈值 (次/分) */

#define LEFT_BEHIND_TIME_THRESHOLD 180      /* 遗留人员判定时间 (秒)，默认3分钟 */

typedef enum {
    EMERGENCY_NONE = 0,
    EMERGENCY_CO2,         /* CO2 浓度过高 */
    EMERGENCY_SMOKE,       /* 烟雾浓度过高 (疑似火灾) */
    EMERGENCY_VITAL_SIGNS, /* 生命体征异常 (心率/呼吸率异常) */
    EMERGENCY_LEFT_BEHIND  /* 遗留人员 (熄火后长时间有人) */
} EmergencyType_t;

extern uint8_t  g_emergency_mode;        /* 紧急模式标志: 0=正常, 1=紧急(有人+危险状态) */

void Sensor_CheckEmergency(void);
void Sensor_EmergencyHandler(EmergencyType_t type);

/* ========== 通信协议定义 ========== */

#define UART1_RX_BUF_SIZE  128
#define CMD_MAX_LEN         64

/* 执行器控制指令格式: "CMD:actuator=value\r\n"
   例如:
   - CMD:fan=1\r\n        (风扇: 0=关, 1=低速, 2=中速, 3=高速)
   - CMD:heater=1\r\n     (加热器: 0=关, 1=开)
   - CMD:cooling=1\r\n    (制冷片: 0=关, 1=开)
   - CMD:window=50\r\n    (车窗: 0-100%)
   - CMD:ignition=1\r\n   (点火: 0=熄火, 1=点火)
   - CMD:buzzer=1\r\n     (蜂鸣器: 0=关, 1=开)
   - CMD:get_status\r\n    (查询所有状态)
*/

/* ========== 函数声明 ========== */

void Sensor_UpdateFromRadar(void);
void Sensor_UpdateFSR(void);
void Sensor_ReportStatusJson(void);

/* UART1 接收和控制相关函数 */
void UART1_InitReceive(void);
void UART1_IRQHandler_Callback(void);
void UART1_ProcessRxData(void);
void UART1_ParseCommand(char *cmd);
void UART1_SendCommand(const char *actuator, int value);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_DATA_H__ */