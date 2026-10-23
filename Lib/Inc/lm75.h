/*
 * lm75.h
 *
 *  Created on: May 3, 2026
 *      Author: 54751
 */
#ifndef __LM75_H
#define __LM75_H

#include "main.h"          // 包含 HAL 句柄
#include "i2c.h"           // 包含 hi2c1 的声明

float LM75_ReadTemp(void);

#endif
