// 这是一个头文件，用来声明一些供其他文件使用的内容（像是一个目录）
#ifndef __BSP_SHT20_H__ // 如果之前没有定义过 __BSP_SHT20_H__ 这个宏（防止被多次包含报错）
#define __BSP_SHT20_H__ // 那么现在定义它

#include "main.h" // 包含工程的主头文件，里面有HAL库的基础定义

// SHT20传感器在 I2C 总线上的设备地址（厂家规定的固定编号）
#define SHT20_ADDR 0x40

// STM32的HAL库要求把7位地址左移1位，变成8位，最低位用来区分是读还是写
#define SHT20_ADDR_READ  0x81 // 1000 0001 (读地址：最低位是1)
#define SHT20_ADDR_WRITE 0x80 // 1000 0000 (写地址：最低位是0)

// 传感器内部定义的一些“命令字”（你发这个数字，它就知道该干嘛）
// Hold Master: 意思是测量期间传感器会拉低总线，让单片机等着它测完
#define SHT20_HOLD_M_READ_T 	0xE3    // 读温度命令 (1110 0011)
#define SHT20_HOLD_M_READ_RH 	0xE5    // 读湿度命令 (1110 0101)

// No Hold Master: 单片机发完命令可以去干别的，过一会儿再回来读结果
#define SHT20_NOHOLD_M_READ_T 	0xF3  	// 读温度命令 (1111 0011)
#define SHT20_NOHOLD_M_READ_RH 	0xF5  	// 读湿度命令 (1111 0101)

// typedef struct 用来定义一个“结构体”（相当于把几个相关的变量打包成一个整体）
typedef struct
{
  float	Tem;  // 用来存放算好的温度值（float表示带小数点的浮点数）
  float Hum;  // 用来存放算好的湿度值
}SHT20_TemRH_Val; // 这个打包后的新类型名字叫 SHT20_TemRH_Val

// 下面是两个函数的声明，告诉别人“我这里有这两个功能可以使用”
// 传入一个8位的命令字，返回一个16位的读取结果
uint16_t BSP_SHT20_Read(uint8_t sht20_cmd);
// 调用这个函数，它会帮你把温湿度测好算好，打包成上面的结构体返回给你
SHT20_TemRH_Val BSP_SHT20_GetData(void);

#endif /* __BSP_SHT20_H__ */ // 头文件结束标志
