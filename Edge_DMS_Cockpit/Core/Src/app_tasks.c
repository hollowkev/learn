#include <stdio.h>
#include <string.h>
#include "app_tasks.h"
#include "protocol.h"     // 包含你的全局字典 g_cabin_data 和 CRC 算法
#include "app_uart.h"     // 包含 UartMessageBuffer
#include "cmsis_os2.h"    // 包含互斥锁等 CMSIS-V2 API
#include "message_buffer.h"
#include "app_hardware.h"
#include "tjc_usart_hmi.h"
#include "bsp_stepper.h"


//
// ==========================================
// 🚨 智能座舱紧急情况安全阈值定义
// ==========================================
#define ALARM_SMOKE_PPM         500     // 烟雾浓度报警线 (ppm)
#define ALARM_FIRE_TEMP         6000    // 火灾温度报警线 (60.0℃ -> 6000)
#define ALARM_CO2_PPM           2000    // CO2 窒息报警线 (ppm)
#define ALARM_HEATSTROKE_TEMP   3500    // 驻车遗留防热射病温度 (35.0℃ -> 3500)

// 假设我们的巡检任务每 100ms 跑一次
#define HANDS_OFF_TICK_LIMIT    30      // 方向盘脱手报警时间 (30 * 100ms = 3秒)


// ==========================================
// 外部句柄引入 (由 CubeMX 或其他文件生成)
// ==========================================
extern UART_HandleTypeDef huart1;
extern osMutexId_t DataBusMutexHandle; 
extern MessageBufferHandle_t UartMessageBuffer;

// ==========================================
// 任务 1：解析任务 (消费者)
// ==========================================
void Task_Parse(void *argument)
{
    // 任务私有的“接水盆”，绝对不会被 DMA 覆盖
    uint8_t local_buf[128]; 
    
    while (1)
    {
			 //
				
				osDelay(100);
				
				//
        // 等待管子初始化完成
        if (UartMessageBuffer == NULL) {
            osDelay(10);
            continue;
        }
				 
        // 1. 阻塞从消息流中抽水。没有指令时，任务死等，CPU 占用率为 0。
        // rx_len 是拆开“信封”后，这帧数据的真实长度
        size_t rx_len = xMessageBufferReceive(UartMessageBuffer, local_buf, sizeof(local_buf), portMAX_DELAY);\
        // 2. 长度过滤 (一帧至少包含: 帧头2 + CMD1 + LEN1 + CRC2 = 6)
        if (rx_len < 6) continue;

        // 3. 在纯净副本 local_buf 里寻找帧头 0x5A 0xA5
        int frame_start = -1;
        for (int i = 0; i < rx_len - 1; i++) {
            if (local_buf[i] == 0x5A && local_buf[i+1] == 0xA5) {
                frame_start = i;
                break;
            }
        }
				
        if (frame_start == -1) continue; 
         
        // 4. 解析核心参数
        uint8_t *frame = &local_buf[frame_start];
        uint8_t cmd = frame[2];
        uint8_t payload_len = frame[3];
				
        // 5. 越界保护
        if (frame_start + 4 + payload_len + 2 > rx_len) continue;

        // 6. CRC16 校验 (CMD + 长度 + Payload)
        uint16_t calc_crc = Calculate_CRC16(&frame[2], 1 + 1 + payload_len);
        uint16_t recv_crc = frame[4 + payload_len] | (frame[4 + payload_len + 1] << 8); // 小端序解析
				printf("[Debug] STM32算出的CRC = %04X, 电脑发来的CRC = %04X\n", calc_crc, recv_crc);
        if (calc_crc != recv_crc) continue; // 校验失败，抛弃该帧
				
        // 7. ============ 校验通过，执行控制 ============
        if (cmd == 0x02 && payload_len == 4) 
        {
            CtrlCmd_t ctrl_cmd;
            memcpy(&ctrl_cmd, &frame[4], sizeof(CtrlCmd_t)); 
					
							printf("%d\n",ctrl_cmd.cmd_key);
            
            // 【极其重要】拿互斥锁，保护数据总线不被打断
            osMutexAcquire(DataBusMutexHandle, osWaitForever);
            
            // 路由物理硬件控制 (字典分发)
            switch (ctrl_cmd.cmd_key) 
            {
                case 5001: // 风扇控制 (动作：0关 / 1开)
                    g_cabin_data.act_fan = ctrl_cmd.cmd_action;
                    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, ctrl_cmd.cmd_action ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;
                    
                case 5002: // 加热器控制 (动作：0关 / 1开)
                    g_cabin_data.act_heater = ctrl_cmd.cmd_action;
                     HAL_GPIO_WritePin(RLY1_GPIO_Port, RLY1_Pin, ctrl_cmd.cmd_action ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;

                case 5003: // 制冷片控制 (动作：0关 / 1开)
                    g_cabin_data.act_cooler = ctrl_cmd.cmd_action;
                    HAL_GPIO_WritePin(RLY2_GPIO_Port, RLY2_Pin, ctrl_cmd.cmd_action ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;

                case 6001: // 车窗控制 (参数：0~100 的开度百分比)
                    g_cabin_data.act_window = ctrl_cmd.cmd_param;
                   BSP_Stepper_SetWindowPosition(ctrl_cmd.cmd_param);
                    break;

                case 7001: // 模拟点火开关 (动作：0熄火 / 1点火)
                    g_cabin_data.act_ignition = ctrl_cmd.cmd_action;
                     HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, ctrl_cmd.cmd_action ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;
								
                case 8001: // 蜂鸣器 (动作：0关 / 1开)
                    g_cabin_data.act_buzzer = ctrl_cmd.cmd_action;
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, ctrl_cmd.cmd_action ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;
								
                case 9001: // 手动模式
                    flag_auto_mode =  ctrl_cmd.cmd_action;
                    break;

                default:
                    // 收到了未知的指令键值，不做任何处理，直接丢弃
                    break;
            }
            
            // 释放互斥锁
            osMutexRelease(DataBusMutexHandle);
						//
						// 前面的 switch(ctrl_cmd.cmd_key) 控制代码 ...
            
            // 释放互斥锁
            osMutexRelease(DataBusMutexHandle);

            // ==========================================
            // 【新增】立刻回复 ACK 应答帧 (CMD = 0x82)
            // ==========================================
//            uint8_t ack_buf[10];
//            ack_buf[0] = 0x5A;
//            ack_buf[1] = 0xA5;
//            ack_buf[2] = 0x82; // 0x82 代表控制命令的应答
//            ack_buf[3] = 0x03; // Payload 长度 (2字节Key + 1字节执行结果)
//            
//            // 把刚才执行的零件 Key 原样塞回去，告诉 Linux "是谁" 执行完了
//            ack_buf[4] = ctrl_cmd.cmd_key & 0xFF;        // Key 低位
//            ack_buf[5] = (ctrl_cmd.cmd_key >> 8) & 0xFF; // Key 高位
//            ack_buf[6] = 0x01; // 0x01 代表执行成功 (0x00 代表失败/异常)
//            
//            // 计算 CRC 并填入尾部
//            uint16_t ack_crc = Calculate_CRC16(&ack_buf[2], 1 + 1 + 3);
//            ack_buf[7] = ack_crc & 0xFF;
//            ack_buf[8] = (ack_crc >> 8) & 0xFF;
//            
//            // 瞬间发回给 Linux
//            // 瞬间发回给 Linux (加入防撞车重试机制)
//            // 如果硬件正忙( != HAL_OK )，就主动挂起等 1 毫秒
//            while (HAL_UART_Transmit_DMA(&huart1, ack_buf, 9) != HAL_OK) 
//            {
//                osDelay(1); 
//            }
        }
    }
}

// ==========================================
// 任务 2：定时全量数据上报任务
// ==========================================
void Task_Report(void *argument)
{
    uint8_t tx_buf[64];
    uint32_t tick_delay = osKernelGetTickFreq() / 10; // 系统时钟节拍频率（每秒多少个 tick）
    uint32_t tick_last = osKernelGetTickCount();   //系统当前运行到第几个 tick
    
    while (1)
    {
        tx_buf[0] = 0x5A;
        tx_buf[1] = 0xA5;
        tx_buf[2] = 0x01; 
        tx_buf[3] = sizeof(CabinData_t); 
        
        // 拿锁，秒级拷贝当前所有传感器状态
        osMutexAcquire(DataBusMutexHandle, osWaitForever);
        memcpy(&tx_buf[4], &g_cabin_data, sizeof(CabinData_t));
        osMutexRelease(DataBusMutexHandle);
        
        uint16_t crc = Calculate_CRC16(&tx_buf[2], 2 + sizeof(CabinData_t));
        int crc_idx = 4 + sizeof(CabinData_t);
        tx_buf[crc_idx] = crc & 0xFF;           
        tx_buf[crc_idx + 1] = (crc >> 8) & 0xFF;  
        
        uint16_t total_len = 4 + sizeof(CabinData_t) + 2;
        // 发送全量传感器状态
        while (HAL_UART_Transmit_DMA(&huart1, tx_buf, total_len) != HAL_OK) 
        {
            osDelay(1);
        }
        osDelayUntil(tick_last + tick_delay);
        tick_last = osKernelGetTickCount();
    }
}
	void Task_Screen(void *argument)
{
    // 准备一个屏幕任务私有的“数据快照副本”，大小与总线完全一致
    CabinData_t screen_data_copy; 
    char num_str[16];
    
    // 给屏幕硬件留出 500ms 的上电初始化和Logo闪烁时间
    osDelay(500); 

    while (1)
    {
        // =======================================================
        // 1. 极速夺锁拷贝（光速完成，绝不影响传感器采集和串口通信）
        // =======================================================
        osMutexAcquire(DataBusMutexHandle, osWaitForever);
        memcpy(&screen_data_copy, &g_cabin_data, sizeof(CabinData_t));
        osMutexRelease(DataBusMutexHandle);

        // =======================================================
        // 2. 使用本地副本进行安全的 HMI 屏幕数据刷新
        // =======================================================
        
        // --- Page 1: 雷达与生命体征页面 ---
        // 【注：雷达目前处于预留打桩状态，读出默认为 0】
        tjc_send_txt("page1.t4", "txt", screen_data_copy.radar_presence ? "有人" : "无人");
        
        // 模拟心率和呼吸，由于雷达重写中，先转为 0.0 显示
        sprintf(num_str, "%d", screen_data_copy.radar_heart);
        tjc_send_txt("page1.t5", "txt", num_str);
        
        sprintf(num_str, "%d", screen_data_copy.radar_breath);
        tjc_send_txt("page1.t6", "txt", num_str);
        
        // --- Page 2: 驾乘姿态页面 (方向盘脱手与座椅压力) ---
        // 修正逻辑：注意你原文中 page2.t4 的判断是 g_radar_presence，这里我改成了更符合语义的 fsr_wheel 
        tjc_send_txt("page2.t4", "txt", screen_data_copy.fsr_wheel ? "离手" : "握手"); 
        tjc_send_txt("page2.t3", "txt", screen_data_copy.fsr_seat ? "有人" : "无人");
        
        // --- Page 3: 车内环境页面 (温湿度与气体浓度) ---
        sprintf(num_str, "%d", screen_data_copy.gas_co2);
        tjc_send_txt("page3.t4", "txt", num_str);
        
        sprintf(num_str, "%d", screen_data_copy.gas_smoke);
        tjc_send_txt("page3.t5", "txt", num_str);
        
        // 【核心修正】：将总线中放大 100 倍的整型还原为真实的浮点数显示！
        sprintf(num_str, "%.1f", screen_data_copy.sht20_temp / 100.0f);
        tjc_send_txt("page3.t6", "txt", num_str);
        
        sprintf(num_str, "%.1f", screen_data_copy.sht20_humi / 100.0f);
        tjc_send_txt("page3.t7", "txt", num_str);
        
        // --- Page 4: 执行器控制状态页面 ---
        tjc_send_txt("page4.t1", "txt", screen_data_copy.act_fan ? "ON" : "OFF");
        tjc_send_txt("page4.t2", "txt", screen_data_copy.act_heater ? "ON" : "OFF");
        tjc_send_txt("page4.t3", "txt", screen_data_copy.act_ignition ? "ON" : "OFF");
        tjc_send_txt("page4.t4", "txt", screen_data_copy.act_window > 0 ? "ON" : "OFF");
        tjc_send_txt("page4.t5", "txt", screen_data_copy.act_cooler ? "ON" : "OFF"); // 修正变量名对齐
        tjc_send_txt("page4.t6", "txt", screen_data_copy.act_buzzer ? "ON" : "OFF");

        // =======================================================
        // 3. 限制刷新频率（串口屏刷新很耗时，控制在 50~100ms 最佳）
        // =======================================================
        osDelay(1000); 
    }
}
void Cabin_Emergency_Process(CabinData_t *pData)
{
    // 静态变量：用于记录方向盘连续脱手的时间(Tick计数)
    static uint16_t hands_off_counter = 0; 

    // ===================================================
    // 🚨 场景一：火灾/毒气极高危告警（最高优先级，一票否决）
    // ===================================================
    if (pData->gas_smoke > ALARM_SMOKE_PPM || pData->sht20_temp > ALARM_FIRE_TEMP )
    {
        // 动作：狂暴鸣笛、强制开窗、风扇狂吹、关加热
        HAL_GPIO_WritePin(GPIOA, BUZZER_Pin, GPIO_PIN_SET);  // 蜂鸣器 ON
        HAL_GPIO_WritePin(GPIOA, FAN_Pin, GPIO_PIN_SET);     // 风扇 ON
        HAL_GPIO_WritePin(GPIOA, RLY1_Pin, GPIO_PIN_RESET);  // 加热器 OFF (切断隐患)
			  BSP_Stepper_SetWindowPosition(0);            // 车窗全开(伪代码,请换成你的步进电机开窗函数)
        
        return; // 发生火灾了，后面的普通报警不用看了，直接退出！
    }

    // ===================================================
    // 👶 场景四：驻车生命体遗留 & 防热射病保护
    // ===================================================
    // 条件：未点火 + (座椅有人 或 雷达有人) + 温度危险
    if (pData->act_ignition == 0) 
    {
        if ((pData->fsr_seat == 1 || pData->radar_presence == 1) && 
            (pData->sht20_temp > ALARM_HEATSTROKE_TEMP))
        {
            // 动作：开冷气/风扇、微开窗户、鸣笛求救
            HAL_GPIO_WritePin(GPIOC, RLY2_Pin, GPIO_PIN_SET); // 制冷/冷风 ON
            BSP_Stepper_SetWindowPosition(30);                // 车窗打开 30% 缝隙
            HAL_GPIO_WritePin(GPIOA, BUZZER_Pin, GPIO_PIN_SET); // 外部求救报警
        }
        else
        {
            // 安全状态下，确保执行器关闭，避免耗尽电瓶
            // HAL_GPIO_WritePin(GPIOC, RLY2_Pin, GPIO_PIN_RESET);
        }
        
        hands_off_counter = 0; // 既然没点火，脱手计数器清零
        return; // 驻车状态下，不检测后面的行驶安全逻辑
    }

    // ===================================================
    // 以下为行驶状态 (act_ignition == 1) 的安全检测
    // ===================================================

    // ⚠️ 场景三：危险驾驶/脱手告警 (DMS核心)
    // 条件：点火状态下，方向盘脱手持续超过 3 秒
    if (pData->fsr_wheel == 1) // 假设 1 代表脱手
    {
        hands_off_counter++;
        if (hands_off_counter >= HANDS_OFF_TICK_LIMIT)
        {
            HAL_GPIO_WritePin(GPIOA, BUZZER_Pin, GPIO_PIN_SET); // 蜂鸣器滴滴警告
            // 保持在最大值，防止溢出
            hands_off_counter = HANDS_OFF_TICK_LIMIT; 
        }
    }
    else
    {
        // 一旦双手握回方向盘，立刻清零警报
        hands_off_counter = 0; 
        HAL_GPIO_WritePin(GPIOA, BUZZER_Pin, GPIO_PIN_RESET);
    }

    // 😷 场景二：防窒息/疲劳预警
    // 条件：点火状态下，CO2 浓度超标
    if (pData->gas_co2 > ALARM_CO2_PPM)
    {
        HAL_GPIO_WritePin(GPIOA, FAN_Pin, GPIO_PIN_SET); // 强制开启换气风扇
        // 如果当前窗户关死了，稍微开一点缝隙
        if (pData->act_window == 0) {
            BSP_Stepper_SetWindowPosition(10); 
        }
    }
}
void Task_Sensor(void *argument)
{
    CabinData_t check_data_copy;
	// 系统刚上电时，给 SHT20、SGP30 等 I2C 传感器预留 200ms 的初始化准备时间
    osDelay(1000); 

    while (1)
    {
        // 1. 呼叫底层干活，内部自动完成采集和拿锁更新
      App_Hardware_UpdateAllToBus();
			//   拿锁拷贝副本
			osMutexAcquire(DataBusMutexHandle, osWaitForever);
      memcpy(&check_data_copy, &g_cabin_data, sizeof(CabinData_t));
      osMutexRelease(DataBusMutexHandle);			
			//紧急进程
			Cabin_Emergency_Process(&check_data_copy);       
      // 2. 挂起任务，把 CPU 让给解析和上报任务 (根据你的项目需求，100ms 或 200ms 均可)
      osDelay(500);
    }
}