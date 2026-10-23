#include "user_rtc.h"

// 系统生成rtc部分的时候只提供了init的部分，其他部分，例如时间戳的获取等函数全部都要由自己实现

#define RTC_INIT_FLAG  0x2333
//定义一个常量标志 0x2333，用于写入备份寄存器，标记“RTC 已经被首次初始化过”。选择该值是为了避免与上电随机值（通常是 0xFFFF 或 0x0000）冲突，提高判断的可靠性。

// 基姆拉尔森公式计算星期几
/*
 * @param year, month, day
 * @retval w即周几
 */
static uint8_t RTC_CalculateWeekDay(uint16_t year, uint8_t month, uint8_t day) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    uint8_t w = (day + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400) % 7;
    // w: 0=周日,1=周一,2=周二 … 6=周六
    // 转换为 HAL 格式：HAL 周一=1, 周二=2 … 周日=7
    if (w == 0) return 7;           // 周日 -> 7
    else return w;                  // 周一~周六 -> 1~6
}

/**
 * @brief 设置RTC时间（年月日时分秒）
 * @param time 指向包含本地时间信息的 struct tm 指针
 * @retval HAL status
 */
HAL_StatusTypeDef USER_RTC_SetTime(struct tm *time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0}; // 初始化结构体为全0，避免干扰后续设置

    sTime.Hours   = time->tm_hour;
    sTime.Minutes = time->tm_min;
    sTime.Seconds = time->tm_sec;
    sTime.TimeFormat = RTC_HOURFORMAT_24; // 定义为24小时制
    sTime.SubSeconds = 0; // 亚秒清零
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE; // 不使用夏时制
    sTime.StoreOperation = RTC_STOREOPERATION_RESET; // 存储操作复位

    sDate.Year    = time->tm_year - 100;   // tm_year 从1900起算，RTC年份0~99代表2000~2099（RTC仅储存后两位）
    sDate.Month   = time->tm_mon + 1;      // tm_mon 0~11，RTC需1~12
    sDate.Date    = time->tm_mday;
    sDate.WeekDay = RTC_CalculateWeekDay(time->tm_year + 1900, sDate.Month, sDate.Date) + 1;    // 使用基姆拉尔森公式计算星期几

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return HAL_ERROR;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

/**
 * @brief 获取RTC时间
 * @param none
 * @retval 指向静态 struct tm 的指针（包含当前时间），失败返回 NULL
 * @note 在task_main.c中调用
 */
struct tm *USER_RTC_GetTime(void)
{
    static struct tm timeinfo;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0}; //初始化时间戳

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return NULL;
    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        return NULL;

    timeinfo.tm_sec  = sTime.Seconds;
    timeinfo.tm_min  = sTime.Minutes;
    timeinfo.tm_hour = sTime.Hours;
    timeinfo.tm_mday = sDate.Date;
    timeinfo.tm_mon  = sDate.Month - 1;     // RTC月份1~12转为tm_mon 0~11
    timeinfo.tm_year = sDate.Year + 100;     // RTC年份0~99转为tm_year从1900起算

    /* 星期转换：HAL_WeekDay 1=周一 ~ 7=周日，转换为 tm_wday 0=周日 ~ 6=周六 */
    timeinfo.tm_wday = (sDate.WeekDay == 7) ? 0 : sDate.WeekDay;

    timeinfo.tm_yday = 0;
    timeinfo.tm_isdst = 0; //此两项不由rtc直接提供，简单令做0

    return &timeinfo;
}

/**
 * @brief RTC初始化（首次上电设置默认时间，之后跳过）
 */
void USER_RTC_Init(void)
{
    uint32_t initFlag = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    if (initFlag == RTC_INIT_FLAG)
        return;

    // 注意：hrtc 必须先由 MX_RTC_Init() 完成基础初始化
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    struct tm time = {
        .tm_year = 2025 - 1900,
        .tm_mon  = 1 - 1,
        .tm_mday = 1,
        .tm_hour = 23,
        .tm_min  = 59,
        .tm_sec  = 55,
    };

    USER_RTC_SetTime(&time);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_INIT_FLAG);
}
