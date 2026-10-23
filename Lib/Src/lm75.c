/*
 * lm75.c
 *
 *  Created on: May 3, 2026
 *      Author: 54751
 */
#include "lm75.h"

#define LM75_ADDR_WRITE 0x90   // 8位写入地址 (0x48 << 1)
#define LM75_ADDR_READ  0x91   // 8位读取地址
// LM75固定地址的高四位为1001，而A0/A1/A2接地代表低三位的000，此时总共8位剩下的最后一位对应功能读还是写

float LM75_ReadTemp(void)
{
    uint8_t temp_data[2] = {0}; // 两字节接收缓冲区（I2C通信的基本单位为两位）
    uint16_t raw; // 合并上面得到的两字节数据成为一个16位原始数据

    // 从温度寄存器(指针0x00)连续读取2个字节（同时判断是否读取成功）
    if (HAL_I2C_Mem_Read(&hi2c1, LM75_ADDR_READ, 0x00, I2C_MEMADD_SIZE_8BIT, temp_data, 2, 100) != HAL_OK)
    	/*
    	 	HAL_StatusTypeDef HAL_I2C_Mem_Read(
    		I2C_HandleTypeDef *hi2c,
    		uint16_t DevAddress,
    		uint16_t MemAddress,
    		uint16_t MemAddSize,
    		uint8_t *pData,
    		uint16_t Size,
    		uint32_t Timeout
			);
			函数原型如上所示，其中Timeout代表超时时间，Size代表准备读取的字节数，pData代表接收的指针，MemAddSize一般是8位，MemAddress代表要从设备内部读取的寄存器/存储器起始地址，DevAddress代表I2C 从设备的 8 位设备地址
    	 */
    {
        return -999.0f;   // 读取失败时返回一个不可能的温度
    }

    // 合成16位原始数据（高字节在前，低字节在后）
    raw = ((uint16_t)temp_data[0] << 8) | temp_data[1]; // 采用移位 + 按位或来合并数据
    raw >>= 5;            // 右移5位，只保留11位温度数据

    // 判断正负，计算实际温度 (LSB = 0.125°C)
    if (raw & 0x0400)     // 第10位为1表示负温度（11位数据的符号位）
    {
        // 负温度处理：补码转原码，乘以 -0.125（注意，此处是在进行补码的扩展）
        return (float)((int16_t)(0xFC00 | raw)) * 0.125f; // 用 0xFC00（即二进制 1111 1100 0000 0000）与 raw 进行“或”操作，这会把高6位全变成 1（因为 raw 的 D10 已经是 1），而 D9~D0 保留不变
    }
    else
    {
        return (float)raw * 0.125f;
    }
}
