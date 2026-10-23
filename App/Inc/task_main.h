#ifndef INC_TASK_MAIN_H_
#define INC_TASK_MAIN_H_

/* 必要的头文件 */
// 引用头文件
#include "oled.h"
#include "usart.h"
#include "user_rtc.h"
#include "adc.h"
#include "lm75.h"

// C语言头文件
#include <stdio.h>
#include <string.h>

// 函数声明
void MainTaskInit();
void MainTask();

/* 宏定义 */
#define CURSOR_FLASH_INTERVAL 500 // 定义闪烁间隔时间为500ms

// led指示灯的引脚定义
#define LED0_PORT GPIOB
#define LED0_PIN GPIO_PIN_3

#define LED1_PORT GPIOC
#define LED1_PIN GPIO_PIN_0

#define LED2_PORT GPIOC
#define LED2_PIN GPIO_PIN_1

#define LED3_PORT GPIOB
#define LED3_PIN GPIO_PIN_0

#define LD2_PORT GPIOA
#define LD2_PIN GPIO_PIN_5

#endif /* INC_TASK_MAIN_H_ */
