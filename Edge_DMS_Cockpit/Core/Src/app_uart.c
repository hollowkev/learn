#include "app_uart.h"
#include "radar.h"
#include "tjc_usart_hmi.h"
//#include "radar_driver.h"

// 必须包含 task.h，否则 BaseType_t 和 portYIELD_FROM_ISR 会报红//就是中断函数
#include "task.h" 
#include "asr_uart3.h"
#include <stdio.h>
// 如果你需要使用 huart1，确保引入了对应的外设头文件或在这里 extern
extern uint8_t radar_dma[128];
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern uint8_t  asr_rx_byte;
//extern uint8_t rx_byte ;

// ==========================================
// 全局变量定义
// ==========================================
uint8_t g_uart_rx_buf[RX_BUF_SIZE]; 

// 消息流管道实体定义
MessageBufferHandle_t UartMessageBuffer = NULL;

//==========================================
// 初始化函数 (请在 main.c 的 main 循环前调用)
// ==========================================
void App_UART_Init(void) {
    // 1. 创建消息流管道，总容量 512 字节 (足够装几十条连续指令)
    UartMessageBuffer = xMessageBufferCreate(512);

    // 2. 启动第一次不定长 DMA 接收
    if (UartMessageBuffer != NULL)
			{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, RX_BUF_SIZE);				 
    }		
}
// ==========================================
// 串口空闲中断回调函数 (极速处理，绝不拖泥带水)
// ==========================================
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{  


    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			    
					 printf("debug receive\n");
            xMessageBufferSendFromISR(UartMessageBuffer, g_uart_rx_buf, Size, &xHigherPriorityTaskWoken);
        //  瞬间重启 DMA，指针强行归零，准备覆盖接收下一条指令
					
			
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, RX_BUF_SIZE);

        // 3. 如果流里有了数据，且解析任务优先级较高，立刻触发任务调度切换
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
		if (huart->Instance == USART2) {
        
       Radar_UART_RxCallback(Size);
			 printf("debug receive\n");
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    
     if (huart->Instance == USART3) {
        ASR_UART3_IRQHandler_Callback();
    } 
		
		if (huart->Instance == UART4) {
        tjc_uart_rx_callback(huart);
    }

    
   
}
//void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
//	
//	  // 终止串口接收
//    HAL_UART_AbortReceive(huart);
//    
//    // （可选）如果是 DMA 接收，确保 DMA 也被强制停止，防止通道被占死
//    if (huart->hdmarx != NULL) 
//    {
//        HAL_DMA_Abort(huart->hdmarx);
//    }
//		if (huart->Instance == USART1) 
//    {
//        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_uart_rx_buf, RX_BUF_SIZE);
//    }
//    if (huart->Instance == USART2) 
//    {
//        HAL_UARTEx_ReceiveToIdle_DMA(huart, radar_dma, 128);
//    
//			if (huart->Instance == USART3) 
//    {
//        HAL_UART_Receive_IT(&huart3, &asr_rx_byte, 1);
//    }
//	  }
//		
//}