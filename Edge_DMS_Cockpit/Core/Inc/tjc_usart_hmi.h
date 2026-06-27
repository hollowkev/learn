#ifndef __TJCUSARTHMI_H__
#define __TJCUSARTHMI_H__

#include <stdio.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"

/**
 * @brief TJC串口屏驱动头文件
 * 
 * 本驱动用于STM32与TJC串口屏之间的通信，支持发送文本、数值和浮点数到串口屏控件。
 * 通信协议采用TJC串口屏标准协议，以0xff 0xff 0xff作为指令结束符。
 */

/* UART配置 */
#define TJC_UART       huart4    /* 使用的UART句柄 - 串口屏连接UART4 */
#define TJC_UART_INS   UART4     /* UART实例 */
extern UART_HandleTypeDef huart4;

/* 公共函数声明 */
void tjc_send_string(char* str);                  /* 发送原始字符串 */
void tjc_send_txt(char* objname, char* attribute, char* txt);  /* 发送文本属性 */
void tjc_send_val(char* objname, char* attribute, int val);    /* 发送整数值 */
//void tjc_send_float(char* objname, char* attribute, float val);/* 发送浮点数值 */
void tjc_send_nstring(char* str, unsigned char str_length);    /* 发送指定长度字符串 */
void initRingBuffer(void);                        /* 初始化环形缓冲区 */
void write1ByteToRingBuffer(uint8_t data);        /* 向环形缓冲区写入1字节 */
void deleteRingBuffer(uint16_t size);             /* 从环形缓冲区删除指定字节数 */
uint16_t getRingBufferLength(void);               /* 获取环形缓冲区数据长度 */
uint8_t read1ByteFromRingBuffer(uint16_t position);/* 从环形缓冲区读取1字节 */
uint8_t isRingBufferOverflow(void);               /* 检查环形缓冲区是否溢出 */
uint8_t tjc_uart_is_ready(void);                  /* 检查UART是否就绪 */
void tjc_uart_rx_callback(UART_HandleTypeDef *huart); /* UART4接收中断回调处理函数 */
HAL_StatusTypeDef tjc_uart_send_bytes(const uint8_t *pData, uint16_t Size); /* 发送字节数组（带错误返回） */

/* 环形缓冲区配置 */
#define RINGBUFFER_LEN    (500)   /* 环形缓冲区大小（字节） */

/* 便捷宏定义 */
#define usize           getRingBufferLength()
#define code_c()        initRingBuffer()
#define udelete(x)      deleteRingBuffer(x)
#define u(x)            read1ByteFromRingBuffer(x)

/* 外部变量声明 */
extern uint8_t RxBuffer[1];      /* 接收缓冲区 */

#endif