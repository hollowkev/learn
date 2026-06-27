#ifndef __BSP_SGP30_H__ // 防止头文件被重复包含
#define __BSP_SGP30_H__

#include "main.h" // 包含HAL库基础定义

// SGP30 传感器在 I2C 总线上的固定地址（7位地址）
#define SGP30_I2C_ADDR 0x58

// SGP30 使用的是 16 位长度的命令字（前面SHT20是8位），这是厂家规定的
#define SGP30_CMD_INIT_AIR_QUALITY    0x2003 // 初始化空气质量算法命令
#define SGP30_CMD_MEASURE_AIR_QUALITY 0x2008 // 测量空气质量命令
#define SGP30_CMD_GET_BASELINE        0x2015 // 获取当前基线值（相当于记忆值）
#define SGP30_CMD_SET_BASELINE        0x201E // 设置基线值
#define SGP30_CMD_GET_FEATURE_SET     0x202F // 获取特性集
#define SGP30_CMD_MEASURE_TEST        0x2032 // 传感器自测命令
#define SGP30_CMD_GET_SERIAL_ID       0x3682 // 获取传感器的出厂序列号

// 定义一个结构体，用来打包我们关心的空气质量数据
typedef struct {
  uint16_t eCO2;  // 等效二氧化碳浓度 (equivalent CO2)
  uint16_t TVOC;  // 总挥发性有机物浓度 (Total Volatile Organic Compounds)
} SGP30_AirQuality_Data;

// 定义一个结构体，用来存 6 个字节长度的序列号
typedef struct {
  uint8_t serial[6];  // 存放 48 位 (6个字节) 序列号的数组
} SGP30_Serial;

// 下面是对外提供的功能函数声明：
// HAL_StatusTypeDef 是 HAL 库定义的一种返回值类型，通常用来表示“成功(HAL_OK)”还是“失败(HAL_ERROR)”

// 传感器上电后，必须先调这个函数来初始化内部的算法
HAL_StatusTypeDef BSP_SGP30_Init(void);
// 每次要读数据时调这个函数，把刚才定义的结构体地址传进去，它会把读到的数据装进去
HAL_StatusTypeDef BSP_SGP30_Measure(SGP30_AirQuality_Data *data);
// 获取内部维护的基线（算法越跑越准，最好定期把基线存起来，下次开机直接用）
HAL_StatusTypeDef BSP_SGP30_GetBaseline(uint16_t *eco2_baseline, uint16_t *tvoc_baseline);
// 开机时把以前存的基线写回给传感器
HAL_StatusTypeDef BSP_SGP30_SetBaseline(uint16_t eco2_baseline, uint16_t tvoc_baseline);
// 读传感器的序列号（每颗芯片都不一样，类似身份证）
HAL_StatusTypeDef BSP_SGP30_GetSerial(SGP30_Serial *serial);

#endif
