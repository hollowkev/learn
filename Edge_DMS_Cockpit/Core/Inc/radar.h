#ifndef __RADAR_H
#define __RADAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  presence;   // 有人/无人
    uint8_t  breath_rate;     // 呼吸
    uint8_t  heart_rate;      // 心率
    uint16_t distance;   // 距离 (mm) 建议单位与外部统一
} RadarData;



// 提供给外部调用的接口函数
void Radar_GetData(RadarData *out_data);


// 初始化雷达驱动
void Radar_Init(void);
// 雷达处理函数（在任务中调用）
void Task_Radar(void);
void Radar_UART_RxCallback(uint16_t Size);


#ifdef __cplusplus
}
#endif

#endif /* __RADAR_H */
