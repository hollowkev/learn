#include "bsp_sgp30.h"
#include "i2c.h"    // 里面有我们需要的硬件接口 hi2c1
#include "stdio.h"  // 包含 printf 等标准函数

// 这是一个内部使用的 CRC8 校验函数。
// 作用：检查数据在 I2C 传输过程中有没有发生错误。相当于给数据加了一层“防伪标签”。
// 参数：data 是数据数组的指针，len 是要校验的数据长度。
static uint8_t SGP30_CRC8(uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFF; // CRC 初始值
  uint8_t i;
  
  while(len--) // 遍历每一个字节
  {
    crc ^= *data++; // 把当前字节和 crc 异或，然后指针移到下一个字节
    for(i = 0; i < 8; i++) // 处理 8 个 bit
    {
      // 这是一套标准的 CRC 算法公式。左移并判断最高位，如果是 1 就异或一个多项式 0x31
      crc = (crc << 1) ^ ((crc & 0x80) ? 0x31 : 0x00);
    }
  }
  return crc; // 返回算出来的防伪码
}

// 初始化 SGP30 内部的空气质量算法
HAL_StatusTypeDef BSP_SGP30_Init(void)
{
  uint8_t cmd[2]; // 准备一个 2 个字节的小数组用来装命令
  
  // SGP30 的命令是 16 位的（比如 0x2003），但 I2C 每次只能发 8 位
  // 所以需要把它拆成高八位和低八位。
  // >> 8 把高八位移下来放到 cmd[0]，& 0xFF 把低八位截出来放到 cmd[1]
  cmd[0] = (SGP30_CMD_INIT_AIR_QUALITY >> 8) & 0xFF;
  cmd[1] = SGP30_CMD_INIT_AIR_QUALITY & 0xFF;
  
  // 把命令发给传感器。
  // 注意：SGP30_I2C_ADDR << 1 是因为 HAL 库要求传入 8 位地址（左移空出最低的读写位）
  return HAL_I2C_Master_Transmit(&hi2c1, (SGP30_I2C_ADDR << 1), cmd, 2, 100);
}

// 获取空气质量数据
HAL_StatusTypeDef BSP_SGP30_Measure(SGP30_AirQuality_Data *data)
{
  uint8_t cmd[2];
  uint8_t buf[6]; // 返回的数据有 6 个字节长，所以准备一个长度为 6 的数组去接
  HAL_StatusTypeDef ret;
  
  // 第一步：拆分并发送“开始测量”的命令 (0x2008)
  cmd[0] = (SGP30_CMD_MEASURE_AIR_QUALITY >> 8) & 0xFF;
  cmd[1] = SGP30_CMD_MEASURE_AIR_QUALITY & 0xFF;
  
  ret = HAL_I2C_Master_Transmit(&hi2c1, (SGP30_I2C_ADDR << 1), cmd, 2, 100);
  if(ret != HAL_OK) return ret; // 如果发送失败（比如线断了），直接报错退出
  
  // 第二步：等待 12 毫秒，让传感器在肚子里慢慢算
  HAL_Delay(12);
  
  // 第三步：去把算好的 6 个字节结果读回来。
  // (SGP30_I2C_ADDR << 1) | 0x01：左移后最低位变成 1，代表“读操作”
  ret = HAL_I2C_Master_Receive(&hi2c1, ((SGP30_I2C_ADDR << 1) | 0x01), buf, 6, 100);
  if(ret != HAL_OK) return ret;
  
  // 第四步：校验防伪码。
  // 传感器发回来的格式是：[二氧化碳高8位, 二氧化碳低8位, 二氧化碳的CRC校验码, TVOC高8位, TVOC低8位, TVOC的CRC校验码]
  // 算一下前两字节的 CRC，看看和它传过来的第三个字节（buf[2]）一不一样
  if(SGP30_CRC8(&buf[0], 2) != buf[2]) return HAL_ERROR; // 不一样说明数据坏了
  if(SGP30_CRC8(&buf[3], 2) != buf[5]) return HAL_ERROR;
  
  // 校验通过后，把高低八位拼成 16 位的完整数据，存进我们传进来的结构体里
  data->eCO2 = (buf[0] << 8) | buf[1];
  data->TVOC = (buf[3] << 8) | buf[4];
  
  return HAL_OK; // 告诉上层：成功拿到数据！
}

// 后面几个函数（GetBaseline, SetBaseline, GetSerial）的套路完全一样：
// 1. 把 16 位命令拆成两半发出去
// 2. 延时等一会儿
// 3. 把带 CRC 校验的数据读回来
// 4. 检查 CRC，拼数据。

// 获取算法基线值
HAL_StatusTypeDef BSP_SGP30_GetBaseline(uint16_t *eco2_baseline, uint16_t *tvoc_baseline)
{
  uint8_t cmd[2];
  uint8_t buf[6];
  HAL_StatusTypeDef ret;
  
  cmd[0] = (SGP30_CMD_GET_BASELINE >> 8) & 0xFF;
  cmd[1] = SGP30_CMD_GET_BASELINE & 0xFF;
  
  ret = HAL_I2C_Master_Transmit(&hi2c1, (SGP30_I2C_ADDR << 1), cmd, 2, 100);
  if(ret != HAL_OK) return ret;
  
  HAL_Delay(10);
  
  ret = HAL_I2C_Master_Receive(&hi2c1, ((SGP30_I2C_ADDR << 1) | 0x01), buf, 6, 100);
  if(ret != HAL_OK) return ret;
  
  if(SGP30_CRC8(&buf[0], 2) != buf[2]) return HAL_ERROR;
  if(SGP30_CRC8(&buf[3], 2) != buf[5]) return HAL_ERROR;
  
  *eco2_baseline = (buf[0] << 8) | buf[1];
  *tvoc_baseline = (buf[3] << 8) | buf[4];
  
  return HAL_OK;
}

// 设置（恢复）算法基线值
HAL_StatusTypeDef BSP_SGP30_SetBaseline(uint16_t eco2_baseline, uint16_t tvoc_baseline)
{
  uint8_t buf[8]; // 这次我们要往传感器发 8 个字节
  
  // 前两个字节放命令
  buf[0] = (SGP30_CMD_SET_BASELINE >> 8) & 0xFF;
  buf[1] = SGP30_CMD_SET_BASELINE & 0xFF;
  
  // 后面放我们自己的基线数据，同样要拆成高低八位
  buf[2] = (eco2_baseline >> 8) & 0xFF;
  buf[3] = eco2_baseline & 0xFF;
  // 重点：发给传感器的数据，我们也得自己算好 CRC 防伪码附在后面，不然传感器不认
  buf[4] = SGP30_CRC8(&buf[2], 2);
  
  buf[5] = (tvoc_baseline >> 8) & 0xFF;
  buf[6] = tvoc_baseline & 0xFF;
  buf[7] = SGP30_CRC8(&buf[5], 2);
  
  // 一口气把命令 + 数据 + CRC 全发过去
  return HAL_I2C_Master_Transmit(&hi2c1, (SGP30_I2C_ADDR << 1), buf, 8, 100);
}

// 获取芯片出厂序列号
HAL_StatusTypeDef BSP_SGP30_GetSerial(SGP30_Serial *serial)
{
  uint8_t cmd[2];
  uint8_t buf[9]; // 序列号比较长，会返回 9 个字节（3对“数据+CRC”）
  HAL_StatusTypeDef ret;
  
  cmd[0] = (SGP30_CMD_GET_SERIAL_ID >> 8) & 0xFF;
  cmd[1] = SGP30_CMD_GET_SERIAL_ID & 0xFF;
  
  ret = HAL_I2C_Master_Transmit(&hi2c1, (SGP30_I2C_ADDR << 1), cmd, 2, 100);
  if(ret != HAL_OK) return ret;
  
  HAL_Delay(10);
  
  ret = HAL_I2C_Master_Receive(&hi2c1, ((SGP30_I2C_ADDR << 1) | 0x01), buf, 9, 100);
  if(ret != HAL_OK) return ret;
  
  // 三对数据，每对都要校验
  if(SGP30_CRC8(&buf[0], 2) != buf[2]) return HAL_ERROR;
  if(SGP30_CRC8(&buf[3], 2) != buf[5]) return HAL_ERROR;
  if(SGP30_CRC8(&buf[6], 2) != buf[8]) return HAL_ERROR;
  
  // 校验通过，把真正的序列号数据挑出来存进数组（跳过 CRC 字节）
  serial->serial[0] = buf[0];
  serial->serial[1] = buf[1];
  serial->serial[2] = buf[3];
  serial->serial[3] = buf[4];
  serial->serial[4] = buf[6];
  serial->serial[5] = buf[7];
  
  return HAL_OK;
}
