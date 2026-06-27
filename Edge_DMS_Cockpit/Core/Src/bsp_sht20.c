#include "bsp_sht20.h" // 包含刚刚我们写的头文件
#include "i2c.h"       // 包含 I2C 的配置，里面有 hi2c1 这个单片机的硬件接口
#include "stdio.h"     // 包含标准的输入输出库，比如 printf

// 定义一个全局变量，用来保存最近一次换算后的温湿度结果
// volatile 关键字的意思是告诉编译器：“这个变量可能会被随时改变，不要随便优化它”
volatile SHT20_TemRH_Val gTemRH_Val;

// 这是一个专门用来给传感器发命令并读回数据的底层函数
// uint16_t 表示它会返回一个无符号的 16 位整数（2个字节大小）
uint16_t BSP_SHT20_Read(uint8_t sht20_cmd)
{
    uint16_t sht20_reg_val = 0; // 准备一个空变量，用来存放最终拼好的 16 位数据
    uint8_t sht20_reg_buff[2]={0x00,0x00}; // 准备一个有两个小格子的数组，用来接收传感器发回来的 2 个字节
    
    // 第一步: 单片机通过 I2C 把命令发给传感器
    // 参数说明: &hi2c1(硬件接口), SHT20_ADDR_WRITE(写地址), &sht20_cmd(要发的命令), 1(发1个字节), 100(最多等100毫秒，超时算失败)
    HAL_I2C_Master_Transmit(&hi2c1, SHT20_ADDR_WRITE, &sht20_cmd, 1, 100);
    
    // 给传感器一点时间去测量（死等 10 毫秒）
    HAL_Delay(10);
    
    // 第二步: 单片机去问传感器要刚才测量的结果
    // 参数说明: &hi2c1(硬件接口), SHT20_ADDR_READ(读地址), sht20_reg_buff(拿个数组去接数据), 2(要读2个字节), 100(超时时间)
    HAL_I2C_Master_Receive(&hi2c1, SHT20_ADDR_READ, sht20_reg_buff, 2, 100);
    
    // 第三步: 传感器会先发高八位，再发低八位。我们需要把它们拼成一个 16 位的数据
    // sht20_reg_buff[0] << 8 意思是把第一个字节往左移 8 位（相当于放到高位去）
    // 然后用 | (按位或) 和第二个字节合并在一起
    sht20_reg_val = (sht20_reg_buff[0] << 8) | sht20_reg_buff[1];	
    
    return (sht20_reg_val); // 把拼好的结果返回给调用它的人
}

// 这是一个给外面用的函数，用来获取最终的温湿度物理量
SHT20_TemRH_Val BSP_SHT20_GetData(void)
{
    uint16_t  pTem=0, pHum=0; // 准备两个变量，用来存读回来的原始温度和湿度数据
    
    // 分别发命令去读温度和湿度
    pTem = BSP_SHT20_Read(SHT20_HOLD_M_READ_T);  // 读温度
    pHum = BSP_SHT20_Read(SHT20_HOLD_M_READ_RH); // 读湿度
    
    // 传感器发回来的湿度数据里，最低的 2 位其实是状态信息（不是湿度的一部分）
    // &= 0xFFFC (1111 1111 1111 1100) 的作用就是把最后两位强制清零（屏蔽掉）
    pHum &= 0xFFFC;
    
    // 下面这两行是芯片厂家提供的数据换算公式，照着套进去就能把原始数字变成真正的摄氏度和百分比
    // / 65536 是因为 16 位数据的最大值就是 65536，这是一个比例换算
    // (float) 是强制类型转换，把整数变成带小数点的数，防止除法的时候把小数丢了
    gTemRH_Val.Tem = -46.85f + 175.72f * ((float)pTem / 65536);
    gTemRH_Val.Hum = -6 + 125 * ((float)pHum / 65536);
    
    return gTemRH_Val; // 把算好的包含温湿度的结构体返回去
}
