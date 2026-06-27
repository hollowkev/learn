//定义了数据结构体  对外声明 extern CabinData_t g_cabin_data; 声明了 crc算法  实际都在.c里面
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>

// 强制 1 字节对齐
#pragma pack(push, 1)

// 传感器数据总线结构体 (CMD: 0x01)
typedef struct {
    // --- 之前已有的字段 ---
    uint8_t  radar_presence;   // 1字节
    uint8_t  radar_breath;     // 1字节
    uint8_t  radar_heart;      // 1字节
    uint16_t radar_distance;   // 2字节
    uint16_t gas_co2;          // 2字节
    uint16_t gas_smoke;        // 2字节
    int16_t  sht20_temp;       // 2字节
    uint16_t sht20_humi;       // 2字节
    uint8_t  act_fan;          // 1字节
    uint8_t  act_heater;       // 1字节
    uint8_t  act_cooler;       // 1字节
    uint8_t  act_window;       // 1字节
    uint8_t  act_ignition;     // 1字节
    uint8_t  act_buzzer;       // 1字节    
    uint8_t  fsr_wheel;        // 1字节：0=未脱手，1=脱手（或根据你的硬件逻辑定义）
    uint8_t  fsr_seat;         // 1字节：0=无人，1=有人离座
    uint8_t  auto_mode;        // 自动模式
} CabinData_t;

// Linux 控制指令结构体 (CMD: 0x02)
typedef struct {
    uint16_t cmd_key;     
    uint8_t  cmd_action;  
    uint8_t  cmd_param;   
} CtrlCmd_t;

#pragma pack(pop)

// 【关键】声明全局数据总线，让其他文件能通过 include 用它
extern CabinData_t g_cabin_data;

// CRC16 算法声明
uint16_t Calculate_CRC16(const uint8_t *data, uint16_t length);

#endif /* __PROTOCOL_H__ */