#include "radar.h"
#include "main.h"
#include "cmsis_os.h" 
#include "usart.h"
#include "ringbuffer.h"
#include <string.h>
#include <stdio.h>
#include <math.h>


#define SOF_BYTE 0x01
#define MAX_PAYLOAD 128


RingBuffer_t  radar_rb={0}; //环形缓冲区
uint8_t radar_dma[128]={0};
static RadarData radar_data = {0}; //一直在维护的临时变量值
osSemaphoreId_t radarRxSemHandle = NULL;



// 解析状态
typedef enum {
    STATE_WAIT_SOF,
    STATE_ID1,
    STATE_ID2,
    STATE_LEN1,
    STATE_LEN2,
    STATE_TYPE1,
    STATE_TYPE2,
    STATE_HEAD_CKSUM,
    STATE_DATA,
    STATE_DATA_CKSUM
} ParserState;

// 内部变量
 ParserState parser_state = STATE_WAIT_SOF;
static uint16_t frame_id = 0;
static uint16_t frame_len = 0;
static uint16_t frame_type = 0;
static uint8_t head_cksum = 0;
static uint8_t calc_head_cksum = 0;
static uint8_t payload[MAX_PAYLOAD];
static uint16_t payload_idx = 0;
static uint8_t data_cksum = 0;
static uint8_t calc_data_cksum = 0;


// 初始化雷达驱动
void Radar_Init(void) {
	
	if ( (radarRxSemHandle = osSemaphoreNew(1, 0, NULL))==NULL) {
       
    }
	 parser_state = STATE_WAIT_SOF;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2,radar_dma,127);

}

// 处理完整的数据包 (逻辑保持不变)
static void process_radar_frame(uint16_t type, uint8_t* data, uint16_t len) {

    if (type == 0x0F09) {
        uint16_t is_human = (data[1] << 8) | data[0];
        radar_data.presence = is_human ;
    }
    else if (type == 0x0A14) {
				 float temp;
        if (len >= 4) memcpy(&temp, &data[0], 4);
			  radar_data.breath_rate=(uint8_t)temp;
    }
    else if (type == 0x0A15) {
			 float temp=0.0f;
        if (len >= 4) memcpy(&temp, &data[0], 4);
			radar_data.heart_rate=(uint8_t)temp;
    }
else if (type == 0x0A04) {
        // 0x0A04：上报所有目标信息。既然只要第一个目标，去掉for循环，直接算！
        if (len >= 16) { // 4字节 target_count + 第一个目标的 X,Y,Z (各4字节，共12字节)
            uint32_t target_count = 0;
            memcpy(&target_count, &data[0], 4);
            
            // 只要有目标，我们就直接提取 offset 为 4, 8, 12 位置的 X, Y, Z
            if (target_count > 0) {
                float x, y, z;
                memcpy(&x, &data[4], 4);
                memcpy(&y, &data[8], 4);
                memcpy(&z, &data[12], 4);
                
                // 计算直线距离存入全局结构体
                radar_data.distance = sqrtf(x*x + y*y + z*z);
            }
        }
    }
    else if (type == 0x0A16) {
        // 0x0A16：雷达直接报告距离 (这是最省事的包)
        if (len >= 8) {
            uint32_t flag;
            float range_cm;
            memcpy(&flag, &data[0], 4);
            memcpy(&range_cm, &data[4], 4);
            
            // flag == 1 表示目标有效
            if (flag == 1) {
                radar_data.distance = range_cm / 100.0f; // 厘米转米
            }
        }
    }
    else if (type == 0x0A17) {
        // 0x0A17：直接报告跟踪目标的坐标
        if (len >= 8) {
            float x, y, z = 0.0f; // 默认 z 为 0，防只发了 XY 的情况
            memcpy(&x, &data[0], 4);
            memcpy(&y, &data[4], 4);
            
            if (len >= 12) {
                memcpy(&z, &data[8], 4);
            }
            
            radar_data.distance = sqrtf(x*x + y*y + z*z);
        }
    }
}

// 处理单个字节的状态机核心 (逻辑保持不变)
static void process_byte_internal(uint8_t byte) {
		 
    switch (parser_state) {
        case STATE_WAIT_SOF:
            if (byte == SOF_BYTE) {								
                parser_state = STATE_ID1;
                calc_head_cksum = SOF_BYTE;
            }
            break;
        case STATE_ID1:
            frame_id = (byte << 8);
            calc_head_cksum ^= byte;
            parser_state = STATE_ID2;
				 
            break;
        case STATE_ID2:
            frame_id |= byte;
            calc_head_cksum ^= byte;
            parser_state = STATE_LEN1;
            break;
        case STATE_LEN1:
            frame_len = (byte << 8);
            calc_head_cksum ^= byte;
            parser_state = STATE_LEN2;
            break;
        case STATE_LEN2:
            frame_len |= byte;
            calc_head_cksum ^= byte;
            parser_state = STATE_TYPE1;
            break;
        case STATE_TYPE1:
            frame_type = (byte << 8);
            calc_head_cksum ^= byte;
            parser_state = STATE_TYPE2;
            break;
        case STATE_TYPE2:
            frame_type |= byte;
            calc_head_cksum ^= byte;
            parser_state = STATE_HEAD_CKSUM;
            break;
        case STATE_HEAD_CKSUM:
            head_cksum = byte;
            if (head_cksum == (uint8_t)(~calc_head_cksum)) {
                if (frame_len == 0) {
                    process_radar_frame(frame_type, NULL, 0);
                    parser_state = STATE_WAIT_SOF;
                } else if (frame_len <= MAX_PAYLOAD) {
                    payload_idx = 0;
                    calc_data_cksum = 0;
                    parser_state = STATE_DATA;
                } else {
                    parser_state = STATE_WAIT_SOF;
                }
            } else {
                parser_state = STATE_WAIT_SOF;
            }
            break;
        case STATE_DATA:
            payload[payload_idx++] = byte;
            calc_data_cksum ^= byte;
            if (payload_idx >= frame_len) {
                parser_state = STATE_DATA_CKSUM;
            }
            break;
        case STATE_DATA_CKSUM:
            data_cksum = byte;
            if (data_cksum == (uint8_t)(~calc_data_cksum)) {
							
								
                process_radar_frame(frame_type, payload, frame_len);
            }
            parser_state = STATE_WAIT_SOF;
            break;
        default:
            parser_state = STATE_WAIT_SOF;
            break;
    }
}
// 雷达处理函数（在任务中调用）
void Task_Radar(void) 
{
	int len = 0;
		
    uint8_t process_byte_buf[128]; 

    for(;;) 
    {
			
			
      osSemaphoreAcquire(radarRxSemHandle, osWaitForever);
			while ((len = RingBuffer_Read(&radar_rb, process_byte_buf, 127)) > 0) 
        {
					  
            for(int i = 0; i < len; i++) {
 
                process_byte_internal(process_byte_buf[i]);          
            }					
    } 
				
							
}
}


void Radar_UART_RxCallback(uint16_t Size)
{
//	printf("size：%d\n",Size);
//	for(int i = 0; i < Size; i++) {            
//                 printf("%02X ", radar_dma[i]);          
//            }
//	printf("\n");
	RingBuffer_Write(&radar_rb,radar_dma,Size);
	osSemaphoreRelease(radarRxSemHandle);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2,radar_dma,127);	
}

void Radar_GetData(RadarData *out_data)
{
	memcpy(out_data,&radar_data,sizeof(RadarData));    
}
