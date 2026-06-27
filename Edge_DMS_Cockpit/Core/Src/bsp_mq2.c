#include "bsp_mq2.h"
#include "adc.h"    // 里面有我们要用的硬件 ADC 接口 hadc2
#include "stdio.h"  // 用来打印调试信息

// 声明外部定义的 ADC2 句柄（通常在 adc.c 里自动生成了）
extern ADC_HandleTypeDef hadc2;

// ==========================================================
// 【终极封装】：把结构体、状态机和底层宏定义全部藏在 .c 文件里！
// 外部文件根本不需要知道这些细节，实现了完全的“信息隐藏”。
// ==========================================================

#define MQ2_ADC_CHANNEL ADC_CHANNEL_13
#define MQ2_VCC_VOLTAGE 5.0f
#define MQ2_ADC_RESOLUTION 4095.0f
#define MQ2_CALIB_SAMPLES 50
#define MQ2_FILTER_SAMPLES 10
#define MQ2_WARMUP_TIME_MS 5000

// 定义传感器当前所处的内部状态
typedef enum {
  MQ2_STATE_WARMUP = 0,   
  MQ2_STATE_CALIBRATING,  
  MQ2_STATE_READY         
} MQ2_State;

// 传感器运行时状态记忆
typedef struct {
  MQ2_State state;                         
  float zero_offset;                       
  float scale_factor;                      
  float raw_buffer[MQ2_FILTER_SAMPLES];    
  uint8_t buffer_index;                    
  float smoke_ppm;                         
  float last_smoke_ppm;                    
  uint32_t warmup_timer;                   
} MQ2_HandleTypeDef;

// 【单例模式】：加了 static 后，这个变量就彻底变成了这个文件的“私有财产”
// 别人拿不到，也改不了，只能乖乖调用你提供的函数！
static MQ2_HandleTypeDef g_mq2;

// ==========================================================

// 初始化函数
HAL_StatusTypeDef BSP_MQ2_Init(void)
{
  g_mq2.state = MQ2_STATE_WARMUP;
  
  g_mq2.zero_offset = 0.0f;
  g_mq2.scale_factor = 1.0f;
  g_mq2.buffer_index = 0;
  g_mq2.smoke_ppm = 0.0f;
  g_mq2.last_smoke_ppm = 0.0f;
  g_mq2.warmup_timer = HAL_GetTick(); // HAL_GetTick() 会返回开机到现在过了多少毫秒
  
  // 专门存最近数据的数组全部清零
  for(uint8_t i = 0; i < MQ2_FILTER_SAMPLES; i++) {
    g_mq2.raw_buffer[i] = 0.0f;
  }
  
  return HAL_ADC_Start(&hadc2);
}

// 检查传感器是否就绪
uint8_t BSP_MQ2_IsReady(void)
{
  return (g_mq2.state == MQ2_STATE_READY) ? 1 : 0;
}

// 底层读 ADC 的原语函数
static uint32_t BSP_MQ2_ReadRaw(void)
{
  HAL_ADC_Start(&hadc2);
  HAL_ADC_PollForConversion(&hadc2, 100);
  uint32_t value = HAL_ADC_GetValue(&hadc2);
  HAL_ADC_Stop(&hadc2); 
  return value;
}

// 滑动平均算法：用来让数据变得平滑，不要上蹿下跳
static float MQ2_MovingAverage(float new_value)
{
  float sum = 0.0f; 
  
  g_mq2.raw_buffer[g_mq2.buffer_index] = new_value;
  g_mq2.buffer_index = (g_mq2.buffer_index + 1) % MQ2_FILTER_SAMPLES;
  
  for(uint8_t i = 0; i < MQ2_FILTER_SAMPLES; i++) {
    sum += g_mq2.raw_buffer[i];
  }
  
  return sum / MQ2_FILTER_SAMPLES;
}

// 核心换算公式：把 ADC 数字变成 ppm 浓度
static float MQ2_ConvertToPPM(float adc_value, float zero_offset, float scale_factor)
{
  if (zero_offset < 100.0f) zero_offset = 100.0f;
  
  // 算出干净空气下的传感器基准电阻
  float volt_clean = (zero_offset / MQ2_ADC_RESOLUTION) * MQ2_VCC_VOLTAGE;
  float rs_clean = (5.0f - volt_clean) / volt_clean;
  float ro = rs_clean / 9.8f;

  // 算出当前环境下的实际电压和传感器电阻
  float voltage = (adc_value / MQ2_ADC_RESOLUTION) * MQ2_VCC_VOLTAGE;
  if(voltage < 0.1f) voltage = 0.1f; 
  float rs = (5.0f - voltage) / voltage;
  
  // 真实的阻值比
  float rs_ro_ratio = rs / ro;
  if(rs_ro_ratio < 0.1f) rs_ro_ratio = 0.1f;
  
  // 经验公式拟合
  float ppm = (600.0f / (rs_ro_ratio * rs_ro_ratio * rs_ro_ratio)) * scale_factor;
  
  if(ppm < 0) ppm = 0;
  if(ppm > 10000) ppm = 10000;
  
  return ppm;
}

// 【状态机主引擎】：主函数直接周期性调用这个就行了
void BSP_MQ2_Update(void)
{
  switch(g_mq2.state) 
  {
    case MQ2_STATE_WARMUP:
      if((HAL_GetTick() - g_mq2.warmup_timer) < MQ2_WARMUP_TIME_MS) {
        // 【修复打印Bug】：之前用取余数 % 5000 < 100 的写法，
        // 在 FreeRTOS 每次延时 1000ms 时，很容易因为误差直接跳过 0~99 的窗口导致不打印。
        // 现在改成记录上次打印的时间，保证每 5 秒必定打印一次。
        static uint32_t last_print_time = 0;
        if (HAL_GetTick() - last_print_time >= 5000) {
            uint32_t remaining = MQ2_WARMUP_TIME_MS - (HAL_GetTick() - g_mq2.warmup_timer);
            printf("MQ2 warming up, remaining: %lu ms\r\n", remaining);
            last_print_time = HAL_GetTick();
        }
      } else {
        printf("MQ2 Warmup completed. Entering calibration...\r\n");
        g_mq2.state = MQ2_STATE_CALIBRATING;
      }
      break;

    case MQ2_STATE_CALIBRATING:
    {
      uint32_t raw_sum = 0;
      for(uint16_t i = 0; i < MQ2_CALIB_SAMPLES; i++) {
        raw_sum += BSP_MQ2_ReadRaw();
        HAL_Delay(10); 
      }
      
      g_mq2.zero_offset = (float)(raw_sum / MQ2_CALIB_SAMPLES);
      g_mq2.scale_factor = 1.0f; 
      
      printf("MQ2 Calibration done: Zero = %.2f\r\n", g_mq2.zero_offset);
      g_mq2.state = MQ2_STATE_READY;
      break;
    }

    case MQ2_STATE_READY:
    {
      uint32_t raw_value = BSP_MQ2_ReadRaw();
      float filtered_value = MQ2_MovingAverage((float)raw_value);
      float ppm = MQ2_ConvertToPPM(filtered_value, g_mq2.zero_offset, g_mq2.scale_factor);
      
      g_mq2.last_smoke_ppm = g_mq2.smoke_ppm;
      g_mq2.smoke_ppm = ppm;                 
      break;
    }
  }
}

// 专门给外部拿最终浓度值的接口
float BSP_MQ2_GetSmokePPM(void)
{
	BSP_MQ2_Update();
  if(g_mq2.state != MQ2_STATE_READY) return 0.0f;
  return g_mq2.smoke_ppm;
}
