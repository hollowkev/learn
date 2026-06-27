#ifndef __APP_HARDWARE_H__
#define __APP_HARDWARE_H__

#include "main.h"

// 暴露出这个总线更新函数，给包工头 Task_Sensor 调用
void App_Hardware_UpdateAllToBus(void);
extern uint8_t flag_auto_mode ;
#endif