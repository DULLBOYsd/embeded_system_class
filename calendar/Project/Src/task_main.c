/* 头文件 */
#include "task_main.h"

/* 用户结构体自定义 */
struct userTime {
	uint32_t hour;
	uint32_t min;
	uint32_t sec;
};

/* 全局变量 */
char weeks[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
struct userTime set = {0}; // 定义变量set用于获取当前设定的闹钟时间

// 光标闪烁部分
static uint8_t cursor_blink_active = 0;          // 0: 不闪烁, 1: 闪烁中
static uint32_t last_blink_time = 0;             // 上次切换的时间
static uint8_t cursor_visible = 0;                // 0: 隐, 1: 显
uint32_t funcmode = 0; // 定义界面功能选项

static uint8_t cursor_pos = 0;  // 当前光标所在字段编号(即修改哪一位时间的值)

/* 函数定义部分 */
// 初始化任务函数
void MainTaskInit() {
	HAL_Delay(20);
	OLED_Init(); //初始化OLED
	USER_RTC_Init(); //初始化自定义的rtc模块
}

/* ADC部分 */
// 动态获取ADC电压数值
uint32_t getADCValue() {
	uint32_t number = 0; // 定义一个变量来接收电压值
	const uint32_t samples = 10; // 采样10次，取平均值以去除抖动

	for(uint32_t i = 0; i < samples; i++) {
		HAL_ADC_Start(&hadc1); // 开始采集ADC信号
		if(HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) { // 每5ms采集一次数据
			number += HAL_ADC_GetValue(&hadc1);
		}
	}
	return number / samples; // 返回采集信号平均值
}

// 线性映射函数（通过线性函数将0~4095映射到对应需要的区间[min, max]上，例如年就应该要在2000~2099即0~99之间进行修改）
uint8_t MapADC(uint32_t adc, uint8_t min, uint8_t max) {
    uint32_t range = max - min + 1;
    return min + (uint8_t)((adc * range) / 4096);  // 4096 让 max 有机会被取到
}

/* 获取温度传感器数值 */
void getTemperature() {
	float temp = LM75_ReadTemp(); // 获取当前温度的浮点数值
	char str[30]; // 用来储存得到的温度字符串
	if(temp > -100.0f) { // 保证获取到的温度为有效值
		sprintf(str, "%.1f", temp); // 注意在setting中启用浮点数
	} else {
		sprintf(str, "Error");
	}
	OLED_DrawImage(15, 0, &temperatureImg, OLED_COLOR_NORMAL);
	OLED_PrintASCIIString(70, 15, str, &afont24x12, OLED_COLOR_NORMAL);
}

/* 闹钟控制模块 */
// 闹钟显示
void setAlarmDisplay() {
	struct tm* now = USER_RTC_GetTime();

	char str[30];
	sprintf(str, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
	OLED_PrintASCIIString(32, 0, str, &afont16x8, OLED_COLOR_NORMAL);

	sprintf(str, "%02d:%02d:%02d", set.hour, set.min, set.sec);
	OLED_PrintASCIIString(16, 20, str, &afont24x12, OLED_COLOR_NORMAL);
}

// 设置闹钟小时
uint32_t setAlarmHour(void) {
    return MapADC(getADCValue(), 0, 23);
}

// 设置闹钟分钟
uint32_t setAlarmMinute(void) {
	return MapADC(getADCValue(), 0, 59);
}

//设置闹钟秒
uint32_t setAlarmSecond(void) {
	return MapADC(getADCValue(), 0, 59);
}

// 闹钟设置
void setAlarmClock(void) {
	if(cursor_blink_active && funcmode == 2) {
		switch(cursor_pos) {
		case 0:
			set.hour = setAlarmHour();
			break;
		case 1:
			set.min = setAlarmMinute();
			break;
		case 2:
			set.sec = setAlarmSecond();
			break;
		}
	}
}

// 闹钟触发函数
void trigAlarmClock() {
	struct tm* now = USER_RTC_GetTime();

	// 静态变量：保留触发状态
	static uint32_t alarm_start = 0;
	static uint8_t alarm_active = 0; // 0：未触发，1：触发中

	if(now->tm_hour == set.hour && now->tm_min == set.min && now->tm_sec == set.sec) {
		if(!alarm_active) {
			alarm_active = 1; // 记录处于active状态
			alarm_start = HAL_GetTick(); // 记录蜂鸣器鸣叫开始的时间
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
		}
	}

	//如果闹钟处于active，检查是否需要停止
	if(alarm_active) {
		if(HAL_GetTick() - alarm_start >= 1000) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
			alarm_active = 0; // 重置闹钟状态
		}
	}
}

/* 日期修改函数 */
// 获取当前时间
void getDate() {
	struct tm* now = USER_RTC_GetTime();

	char str[30];
	sprintf(str, "%d-%02d-%02d", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
	OLED_PrintASCIIString(24, 0, str, &afont16x8, OLED_COLOR_NORMAL);

	sprintf(str, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
	OLED_PrintASCIIString(16, 20, str, &afont24x12, OLED_COLOR_NORMAL);

	char* week = weeks[now->tm_wday];
	uint8_t x_weeks = (128 - (strlen(week) * 8)) / 2; // 应该留白的像素大小->（两侧等量留白）显示
	OLED_PrintASCIIString(x_weeks, 48, week, &afont16x8, OLED_COLOR_NORMAL);
}

// 以下为设置年月日时分秒的函数
void setYear(void) {
    struct tm *now = USER_RTC_GetTime();
    if (!now) return;

    uint32_t adc = getADCValue(); // 获取当前的ADC值
    // 年份范围: 0~99 (代表 2000~2099)
    uint8_t y = MapADC(adc, 0, 99);
    now->tm_year = y + 100;   // tm_year 从 1900 起算
    USER_RTC_SetTime(now);
}

void setMonth(void) {
    struct tm *now = USER_RTC_GetTime();
    if (!now) return;

    uint32_t adc = getADCValue();
    uint8_t m = MapADC(adc, 1, 12);
    now->tm_mon = m - 1;      // tm_mon 0~11
    USER_RTC_SetTime(now);
}

void setDay(void) {
    struct tm *now = USER_RTC_GetTime(); // 获取时间戳
    if (!now) return; // 检查指针的有效性

    // 计算当月最大天数
    uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31}; // 定义月份对应的天数

    // 调整天数时必须要用到年份和月份来计算该年该月的最大天数
    uint8_t year = now->tm_year + 1900; // tm_year的起始年份为1900
    uint8_t month = now->tm_mon + 1;

    uint8_t maxDay = daysInMonth[month - 1];
    // 闰年二月处理
    if (month == 2 && ((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0))) {
        maxDay = 29;
    }

    uint32_t adc = getADCValue();
    uint8_t d = MapADC(adc, 1, maxDay); // 将值映射到对应区间上
    now->tm_mday = d;
    USER_RTC_SetTime(now);
}

void setHour(void) {
    struct tm *now = USER_RTC_GetTime();
    if (!now) return;

    uint32_t adc = getADCValue();
    uint8_t h = MapADC(adc, 0, 23);
    now->tm_hour = h;
    USER_RTC_SetTime(now);
}

void setMinute(void) {
    struct tm *now = USER_RTC_GetTime();
    if (!now) return;

    uint32_t adc = getADCValue();
    uint8_t m = MapADC(adc, 0, 59);
    now->tm_min = m;
    USER_RTC_SetTime(now);
}

void setSecond(void) {
    struct tm *now = USER_RTC_GetTime();
    if (!now) return;

    uint32_t adc = getADCValue();
    uint8_t s = MapADC(adc, 0, 59);
    now->tm_sec = s;
    USER_RTC_SetTime(now);
}

// 该函数将设定跳转到光标指向的位置
void setDate() {
	// 如果正在闪烁（修改模式），根据 cursor_pos 调用对应的修改函数
	if (cursor_blink_active && funcmode == 0) {
	    switch (cursor_pos) {
	        case 0:
	        	setYear();
	        	break;
	        case 1:
	        	setMonth();
	        	break;
	        case 2:
	        	setDay();
	        	break;
	        case 3:
	        	setHour();
	        	break;
	        case 4:
	        	setMinute();
	        	break;
	        case 5:
	        	setSecond();
	        	break;
	    }
	}
}

/* 闪烁函数 */
// 时钟闪烁显示函数
void showCursor() {
    if (cursor_blink_active && funcmode == 0) {
        // 闪烁计时
        if (HAL_GetTick() - last_blink_time > CURSOR_FLASH_INTERVAL) {
            cursor_visible = !cursor_visible;
            last_blink_time = HAL_GetTick();
        }

        if (cursor_visible) {
            uint8_t x_start, x_end, y_pos;

            switch (cursor_pos) {
                case 0: // 年 (24~55)
                    x_start = 24;
                    x_end   = 55;
                    y_pos   = 17;   // 在16像素高的字体下方1像素
                    break;
                case 1: // 月 (64~79)
                    x_start = 64;
                    x_end   = 79;
                    y_pos   = 17;
                    break;
                case 2: // 日 (88~103)
                    x_start = 88;
                    x_end   = 103;
                    y_pos   = 17;
                    break;
                case 3: // 时 (16~39)
                    x_start = 16;
                    x_end   = 39;
                    y_pos   = 45;   // 字体高24，显示在y=20，则下方为 20+24+1=45
                    break;
                case 4: // 分 (52~75)
                    x_start = 52;
                    x_end   = 75;
                    y_pos   = 45;
                    break;
                case 5: // 秒 (88~111)
                	x_start = 88;
                	x_end   = 111;
                	y_pos   = 45;
                	break;
                default:
                    return;
            }
            // 画一条水平线作为光标
            OLED_DrawLine(x_start, y_pos, x_end, y_pos, OLED_COLOR_NORMAL);
        }
    }
}

// 闹钟闪烁显示函数
void showAlarmCursor() {
    if (cursor_blink_active && funcmode == 2) {
        // 闪烁计时
        if (HAL_GetTick() - last_blink_time > CURSOR_FLASH_INTERVAL) {
            cursor_visible = !cursor_visible;
            last_blink_time = HAL_GetTick();
        }

        if (cursor_visible) {
            uint8_t x_start, x_end, y_pos;

            switch (cursor_pos) {
                case 0: // 时 (16~39)
                    x_start = 16;
                    x_end   = 39;
                    y_pos   = 45;   // 字体高24，显示在y=20，则下方为 20+24+1=45
                    break;
                case 1: // 分 (52~75)
                    x_start = 52;
                    x_end   = 75;
                    y_pos   = 45;
                    break;
                case 2: // 秒 (88~111)
                	x_start = 88;
                	x_end   = 111;
                	y_pos   = 45;
                	break;
                default:
                    return;
            }
            // 画一条水平线作为光标
            OLED_DrawLine(x_start, y_pos, x_end, y_pos, OLED_COLOR_NORMAL);
        }
    }
}


// 开始闪烁&进入修改模式
void startCursorBlink(void) {
    cursor_blink_active = 1;
    cursor_visible = 1;
    last_blink_time = HAL_GetTick();
    cursor_pos = 0;    // 每次进入修改模式，光标从第一个字段（年）开始
}

// 停止闪烁&退出修改模式
void stopCursorBlink(void) {
    cursor_blink_active = 0;
}



/* 按键功能部分 */
// 按键长短按控制选择调节和显示
void calendarChange(uint32_t CURSOR_FIELD_COUNT) {
    static uint8_t key_state = 0;
    static uint32_t key_press_time = 0;

    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET) {
        if(key_state == 0) {
            key_state = 1; // 调整到状态1
            key_press_time = HAL_GetTick(); // 获取当前时间作为按键按下的时间
        } else if(key_state == 1) {
            if(HAL_GetTick() - key_press_time > 50) { // 去除按键消抖时间
                key_state = 2; // 调整到状态2
            }
        } else if(key_state == 2) {
            if(HAL_GetTick() - key_press_time > 1000) {
                key_state = 3; // 调整到状态3
            }
        }
    } else {
        if(key_state == 2) {
            // 短按：如果正在闪烁，则切换光标位置
            if (cursor_blink_active) {
                cursor_pos = (cursor_pos + 1) % CURSOR_FIELD_COUNT; // 短按转换按键调整的对象，注意使用模来限制mode的大小
            }
        } else if(key_state == 3) {
            // 长按释放：切换闪烁模式
            if (cursor_blink_active) {
                stopCursorBlink();
            } else {
                startCursorBlink();
            }
        }
        key_state = 0;
    }
}

// 状态切换函数
void changeFunction() {
	uint32_t old_mode = funcmode; // 记录旧模式

	if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET && funcmode != 0) { // 模式1：万年历系统
		funcmode = 0;
	} else if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET && funcmode != 1) { // 模式2：温度显示
		funcmode = 1;
	} else if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_RESET && funcmode != 2) { // 模式3：闹钟界面
		funcmode = 2;
	}

	// 由于界面0和界面2共享一个光标闪烁函数，当模式发生切换时，强制退出修改状态
	if(funcmode != old_mode) {
		stopCursorBlink(); // 强制退出光标闪烁状态
		cursor_pos = 0; // 光标位置复位
	}

	switch(funcmode) {
	case 0:
		// 获取并显示时间
		OLED_NewFrame(); // 更新OLED
		getDate();

		//更改时间
		calendarChange(6);
		setDate();
		showCursor();
		OLED_ShowFrame();
		break;
	case 1: // 温度传感器显示数值模块
		OLED_NewFrame();
		getTemperature();
		OLED_ShowFrame();
		break;
	case 2:
		OLED_NewFrame();
		//显示当前时间
		setAlarmDisplay();

		//更改时间
		calendarChange(3);
		setAlarmClock();
		showAlarmCursor();

		//触发闹钟
		trigAlarmClock();
		OLED_ShowFrame();
	}
}

// 主任务函数执行部分
void MainTask() {
	changeFunction(); // 选择功能
}
