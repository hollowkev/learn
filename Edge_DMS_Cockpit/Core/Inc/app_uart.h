#ifndef __APP_UART_H__
#define __APP_UART_H__

#include "main.h" 
#include "FreeRTOS.h"        // 引入原生 FreeRTOS
#include "message_buffer.h"  // 引入消息流专用的头文件

#define RX_BUF_SIZE 128
extern uint8_t g_uart_rx_buf[RX_BUF_SIZE]; 

// 【新增】向外暴露消息流句柄
extern MessageBufferHandle_t UartMessageBuffer;

void App_UART_Init(void);

#endif