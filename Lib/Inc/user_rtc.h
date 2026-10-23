#ifndef __KK_RTC_H__
#define __KK_RTC_H__ // 防止头文件被重定义

//头文件部分
#include "stm32f4xx_hal.h"
#include <time.h>

extern RTC_HandleTypeDef hrtc;   // 在 rtc.c 中定义，这里只是引用说明

//函数声明部分
HAL_StatusTypeDef KK_RTC_SetTime(struct tm *time);
struct tm *USER_RTC_GetTime(void);
void KK_RTC_Init(void);

#endif /* __KK_RTC_H__ */
