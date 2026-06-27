#include "app_hardware.h"
#include "protocol.h"
#include "cmsis_os2.h"
#include "radar.h"
#include "bsp_mq2.h"
#include "bsp_sht20.h"
#include "bsp_sgp30.h"
#include "bsp_stepper.h"
#include <stdio.h>
//#include "radar_driver.h"

uint8_t flag_auto_mode =0;//自动模式判断位
// 引入全局数据总线和互斥锁
extern CabinData_t g_cabin_data;
extern osMutexId_t DataBusMutexHandle;

void App_Hardware_UpdateAllToBus(void)
{
    // =======================================================
    // 第一步：无锁采集区 (绝不阻塞系统，耗时再长也不怕)
    // =======================================================
    
    // 1. 烟雾传感器 (补上了 Update 函数！) 
    uint16_t temp_smoke = BSP_MQ2_GetSmokePPM();

    // 2. 温湿度传感器  
    SHT20_TemRH_Val sht20_str = BSP_SHT20_GetData(); 
    
    // 3. 二氧化碳传感器
    SGP30_AirQuality_Data temp_sgp30;      
    BSP_SGP30_Measure(&temp_sgp30);
        
    // 4. 方向盘/座椅压力传感器
    uint8_t temp_fsr_wheel = !HAL_GPIO_ReadPin(GPIOC, FSR_WHEEL_Pin);
    uint8_t temp_fsr_seat  = !HAL_GPIO_ReadPin(GPIOC, FSR_SEAT_Pin);

    // 5. 执行器状态真实引脚回读
    uint8_t temp_act_ignition = HAL_GPIO_ReadPin(GPIOA, LIGHT_Pin); // 注意引脚名
    uint8_t temp_act_fan      = HAL_GPIO_ReadPin(GPIOA, FAN_Pin);
    uint8_t temp_act_heater   = HAL_GPIO_ReadPin(GPIOA, RLY1_Pin);
    uint8_t temp_act_cooler   = HAL_GPIO_ReadPin(GPIOC, RLY2_Pin);
    uint8_t temp_act_buzzer   = HAL_GPIO_ReadPin(GPIOA, BUZZER_Pin);
    uint8_t temp_act_window   = BSP_Stepper_GetWindowPosition(); 
		// 补充雷达
		RadarData temp_radar;
		Radar_GetData(&temp_radar);

    // =======================================================
    // 第二步：极速加锁拷贝区 (微秒级完成，绝不卡顿通信)
    // =======================================================
    osMutexAcquire(DataBusMutexHandle, osWaitForever);

    g_cabin_data.gas_smoke = temp_smoke;
    
    // 温湿度放大 100 倍转为整型
    g_cabin_data.sht20_temp = (int16_t)(sht20_str.Tem * 100);
    g_cabin_data.sht20_humi = (uint16_t)(sht20_str.Hum * 100);
    
    g_cabin_data.gas_co2 = temp_sgp30.eCO2;

    g_cabin_data.fsr_wheel = temp_fsr_wheel;
    g_cabin_data.fsr_seat  = temp_fsr_seat;

    g_cabin_data.act_ignition = temp_act_ignition;
    g_cabin_data.act_fan      = temp_act_fan;
    g_cabin_data.act_heater   = temp_act_heater;
    g_cabin_data.act_cooler   = temp_act_cooler;
    g_cabin_data.act_buzzer   = temp_act_buzzer;
    g_cabin_data.act_window   = temp_act_window;

    // 【雷达预留打桩】：
  g_cabin_data.radar_presence = temp_radar.presence;
  g_cabin_data.radar_breath   = temp_radar.breath_rate;
  g_cabin_data.radar_heart    = temp_radar.heart_rate;
  g_cabin_data.radar_distance = temp_radar.distance;
	//新增自动模式
	
    g_cabin_data.auto_mode = flag_auto_mode;
    osMutexRelease(DataBusMutexHandle);
}