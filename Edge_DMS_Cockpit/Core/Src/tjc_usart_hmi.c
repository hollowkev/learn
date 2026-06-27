/**
 * @brief TJC串口屏驱动实现文件
 * 
 * 使用说明:
 *     1. 将 tjc_usart_hmi.c 和 tjc_usart_hmi.h 添加到项目中
 *     2. 在需要使用的文件中包含头文件: #include "tjc_usart_hmi.h"
 *     3. 使用前需初始化环形缓冲区: initRingBuffer()
 *     4. 使用前需启动UART接收中断: HAL_UART_Receive_IT(&TJC_UART, RxBuffer, 1)
 * 
 * 通信协议:
 *     发送格式: objname.attribute=value\xff\xff\xff
 *     文本格式: objname.attribute="text"\xff\xff\xff
 */


#include "tjc_usart_hmi.h"

/* 环形缓冲区结构体定义 */
typedef struct
{
    uint16_t Head;           /* 缓冲区头指针（读取位置） */
    uint16_t Tail;           /* 缓冲区尾指针（写入位置） */
    uint16_t Length;         /* 当前数据长度 */
    uint8_t  Ring_data[RINGBUFFER_LEN];  /* 缓冲区数据存储 */
} RingBuffer_t;

/* 全局变量 */
RingBuffer_t ringBuffer;    /* 环形缓冲区实例 */
uint8_t RxBuffer[1];        /* UART接收缓冲区（单字节） */

/**
 * @brief 整数转字符串函数
 * 
 * 将整数转换为ASCII字符串，支持负数转换。
 * 
 * @param num 要转换的整数
 * @param str 输出字符串缓冲区（需足够大）
 */
void intToStr(int num, char* str) {
    int i = 0;
    int isNegative = 0;

    /* 处理负数 */
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    /* 逐位提取数字 */
    do {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num);

    /* 如果是负数，添加负号 */
    if (isNegative) {
        str[i++] = '-';
    }

    /* 添加字符串结束符 */
    str[i] = '\0';

    /* 反转字符串 */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/**
 * @brief 发送单个字符到串口
 * 
 * 阻塞方式发送单个字符，确保字符发送完成后返回。
 * 
 * @param ch 要发送的字符
 */
void uart_send_char(char ch)
{
    uint8_t ch2 = (uint8_t)ch;
    /* 等待上一次发送完成 */
    while (__HAL_UART_GET_FLAG(&TJC_UART, UART_FLAG_TC) == RESET);
    /* 阻塞发送1字节 */
    HAL_UART_Transmit(&TJC_UART, &ch2, 1, HAL_MAX_DELAY);
}

/**
 * @brief 发送字符串到串口（无结束符）
 * 
 * 逐字符发送字符串，不添加协议结束符。
 * 
 * @param str 要发送的字符串
 */
void uart_send_string(char* str)
{
    /* 遍历字符串，逐个发送字符 */
    while (*str != 0 && str != 0)
    {
        uart_send_char(*str++);
    }
}

/**
 * @brief 发送原始字符串到串口屏
 * 
 * 将字符串发送到串口屏，自动添加协议结束符(0xff 0xff 0xff)。
 * 
 * @param str 要发送的字符串
 * @example tjc_send_string("page main");  // 切换到main页面
 */
void tjc_send_string(char* str)
{
    /* 发送字符串内容 */
    while (*str != 0 && str != 0)
    {
        uart_send_char(*str++);
    }
    /* 添加协议结束符 */
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
}

/**
 * @brief 发送文本属性到串口屏控件
 * 
 * 设置串口屏上指定控件的文本属性。
 * 
 * @param objname 控件名称（如"t0"）
 * @param attribute 属性名称（如"txt"）
 * @param txt 文本内容
 * @example tjc_send_txt("t0", "txt", "Hello World");  // 设置t0控件的文本为"Hello World"
 */
void tjc_send_txt(char* objname, char* attribute, char* txt)
{
    uart_send_string(objname);        /* 发送控件名 */
    uart_send_char('.');              /* 发送分隔符 */
    uart_send_string(attribute);      /* 发送属性名 */
    uart_send_string("=\"");          /* 发送赋值符号和引号 */
    uart_send_string(txt);            /* 发送文本内容 */
    uart_send_char('\"');             /* 发送结束引号 */
    /* 添加协议结束符 */
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
}

/**
 * @brief 发送整数值到串口屏控件
 * 
 * 设置串口屏上指定控件的数值属性。
 * 
 * @param objname 控件名称（如"n0"）
 * @param attribute 属性名称（如"val"）
 * @param val 整数值
 * @example tjc_send_val("n0", "val", 100);  // 设置n0控件的数值为100
 */
void tjc_send_val(char* objname, char* attribute, int val)
{
    /* 拼接命令格式: objname.attribute=value */
    uart_send_string(objname);        /* 发送控件名 */
    uart_send_char('.');              /* 发送分隔符 */
    uart_send_string(attribute);      /* 发送属性名 */
    uart_send_char('=');              /* 发送赋值符号 */
    
    /* 将整数转换为字符串 */
    char txt[12] = "";
    intToStr(val, txt);
    uart_send_string(txt);            /* 发送数值字符串 */
    
    /* 添加协议结束符 */
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
}

/**
 * @brief 发送浮点数值到串口屏控件
 * 
 * 设置串口屏上指定控件的浮点数值属性，保留2位小数。
 * 
 * @param objname 控件名称（如"n0"）
 * @param attribute 属性名称（如"val"）
 * @param val 浮点数值
 * @example tjc_send_float("n1", "val", 25.67);  // 设置n1控件的数值为25.67
 */
//void tjc_send_float(char* objname, char* attribute, float val)
//{
//    uart_send_string(objname);        /* 发送控件名 */
//    uart_send_char('.');              /* 发送分隔符 */
//    uart_send_string(attribute);      /* 发送属性名 */
//    uart_send_char('=');              /* 发送赋值符号 */
//    
//    /* 将浮点数转换为字符串（保留2位小数） */
//    char txt[20] = "";
//    sprintf(txt, "%.2f", val);
//    uart_send_string(txt);            /* 发送数值字符串 */
//    
//    /* 添加协议结束符 */
//    uart_send_char(0xff);
//    uart_send_char(0xff);
//    uart_send_char(0xff);
//}

/**
 * @brief 发送指定长度字符串到串口屏
 * 
 * 发送指定长度的字符串到串口屏，自动添加协议结束符。
 * 
 * @param str 要发送的字符串
 * @param str_length 字符串长度
 */
void tjc_send_nstring(char* str, unsigned char str_length)
{
    /* 发送指定长度的字符 */
    for (int var = 0; var < str_length; ++var)
    {
        uart_send_char(*str++);
    }
    /* 添加协议结束符 */
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
}

/**
 * @brief TJC串口接收中断处理函数
 * 
 * 当UART4接收到1字节数据时调用，将数据写入环形缓冲区。
 * 
 * @param huart UART句柄
 */
void tjc_uart_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == TJC_UART_INS)
    {
        write1ByteToRingBuffer(RxBuffer[0]);
        HAL_UART_Receive_IT(&TJC_UART, RxBuffer, 1);
    }
}

/**
 * @brief 初始化环形缓冲区
 * 
 * 将环形缓冲区的头指针、尾指针和长度都初始化为0。
 */
void initRingBuffer(void)
{
    ringBuffer.Head = 0;
    ringBuffer.Tail = 0;
    ringBuffer.Length = 0;
}

/**
 * @brief 向环形缓冲区写入1字节数据
 * 
 * 将单字节数据写入环形缓冲区，如果缓冲区已满则丢弃数据。
 * 
 * @param data 要写入的数据
 */
void write1ByteToRingBuffer(uint8_t data)
{
    /* 检查缓冲区是否已满 */
    if (ringBuffer.Length >= RINGBUFFER_LEN)
    {
        return;
    }
    /* 写入数据到当前尾指针位置 */
    ringBuffer.Ring_data[ringBuffer.Tail] = data;
    /* 移动尾指针（循环） */
    ringBuffer.Tail = (ringBuffer.Tail + 1) % RINGBUFFER_LEN;
    /* 增加数据长度 */
    ringBuffer.Length++;
}

/**
 * @brief 从环形缓冲区删除指定字节数
 * 
 * 从缓冲区头部删除指定数量的字节数据。
 * 
 * @param size 要删除的字节数
 */
void deleteRingBuffer(uint16_t size)
{
    /* 如果要删除的数量大于等于当前数据长度，清空缓冲区 */
    if (size >= ringBuffer.Length)
    {
        initRingBuffer();
        return;
    }
    /* 逐字节删除 */
    for (int i = 0; i < size; i++)
    {
        ringBuffer.Head = (ringBuffer.Head + 1) % RINGBUFFER_LEN;
        ringBuffer.Length--;
    }
}

/**
 * @brief 从环形缓冲区读取指定位置的1字节数据
 * 
 * 根据偏移位置读取环形缓冲区中的数据。
 * 
 * @param position 相对于头指针的偏移位置
 * @return 读取到的字节数据
 */
uint8_t read1ByteFromRingBuffer(uint16_t position)
{
    /* 计算实际读取位置 */
    uint16_t realPosition = (ringBuffer.Head + position) % RINGBUFFER_LEN;
    /* 返回数据 */
    return ringBuffer.Ring_data[realPosition];
}

/**
 * @brief 获取环形缓冲区当前数据长度
 * 
 * @return 当前数据长度
 */
uint16_t getRingBufferLength()
{
    return ringBuffer.Length;
}

/**
 * @brief 检查环形缓冲区是否溢出
 * 
 * @return 0: 缓冲区未满, 1: 缓冲区已满（溢出）
 */
uint8_t isRingBufferOverflow()
{
    return ringBuffer.Length >= RINGBUFFER_LEN;
}

/**
 * @brief 检查UART是否就绪
 * 
 * 检查UART实例是否有效且状态为READY。
 * 
 * @return 0: UART未就绪, 1: UART就绪
 */
uint8_t tjc_uart_is_ready(void)
{
    /* 检查UART实例是否为空 */
    if (TJC_UART.Instance == NULL)
    {
        return 0;
    }
    /* 检查UART状态是否为READY */
    if (TJC_UART.gState == HAL_UART_STATE_READY)
    {
        return 1;
    }
    return 0;
}