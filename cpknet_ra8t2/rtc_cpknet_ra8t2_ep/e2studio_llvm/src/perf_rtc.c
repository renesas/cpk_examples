/*
 * perf_rtc.c
 * Created on: Feb 12, 2026
 * Author: Ran Qingling
 */
#include <perf_rtc.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "common_utils.h"
/* 全局变量定义 */
static rtc_time_t g_current_time = {0};  // 存储当前RTC时间
volatile uint32_t g_1s_tick_flag = 0;    // 1秒中断标记（volatile确保多线程/中断可见）

/* 函数声明 */
fsp_err_t rtc_simple_init(void);
void rtc_set_initial_time(void);
void rtc_print_current_time(void);

/*******************************************************************************************************************//**
 * @brief RTC中断回调函数（1秒周期性中断触发）
 * @param[in] p_args 回调参数
 * @retval None
 **********************************************************************************************************************/
CODE_AREA
void rtc_callback(rtc_callback_args_t *p_args)
{
    if(RTC_EVENT_PERIODIC_IRQ == p_args->event)
    {
        g_1s_tick_flag = 1;  // 置位1秒中断标记
    }
}

/*******************************************************************************************************************//**
 * @brief 简化版RTC初始化：打开RTC + 设置1秒周期性中断
 * @retval FSP_SUCCESS 初始化成功
 * @retval 其他值      初始化失败
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t rtc_simple_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* 1. 打开RTC驱动 */
    err = R_RTC_Open(&g_rtc_ctrl, &g_rtc_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("RTC Open failed! Error code: 0x%x\r\n", err);
        return err;
    }

    /* 2. 设置RTC周期性中断为1秒（核心：每秒触发一次中断） */
    err = R_RTC_PeriodicIrqRateSet(&g_rtc_ctrl, RTC_PERIODIC_IRQ_SELECT_1_SECOND);
    if (FSP_SUCCESS != err)
    {
        printf("Set RTC 1s periodic IRQ failed! Error code: 0x%x\r\n", err);
        R_RTC_Close(&g_rtc_ctrl);  // 打开成功但设置失败，关闭驱动
        return err;
    }

    printf("RTC init success! Periodic IRQ set to 1 second\r\n");
    return err;
}

/*******************************************************************************************************************//**
 * @brief 初始化RTC为指定的初始时间（示例：2026-02-12 10:00:00）
 * @retval None
 **********************************************************************************************************************/
CODE_AREA
void rtc_set_initial_time(void)
{
    fsp_err_t err = FSP_SUCCESS;
    rtc_time_t init_time = {0};

    /* 配置初始时间：2026-02-12 10:00:00（注意RTC的月份是0基、年份是偏移值） */
    init_time.tm_year = 2026 - YEAR_ADJUST_VALUE;  // 适配硬件的年份偏移（通常是1970/2000）
    init_time.tm_mon  = 2 - MON_ADJUST_VALUE;      // 月份0=1月，所以2月对应1
    init_time.tm_mday = 12;                        // 日期12号
    init_time.tm_hour = 10;                        // 小时10
    init_time.tm_min  = 0;                         // 分钟0
    init_time.tm_sec  = 0;                         // 秒0

    /* 设置RTC初始时间 */
    err = R_RTC_CalendarTimeSet(&g_rtc_ctrl, &init_time);
    if (FSP_SUCCESS != err)
    {
        printf("Set RTC initial time failed! Error code: 0x%x\r\n", err);
    }

    printf("RTC initial time set: 2026-02-12 10:00:00\r\n");
}

/*******************************************************************************************************************//**
 * @brief 读取并打印当前RTC时间（格式化输出）
 * @retval None
 **********************************************************************************************************************/
CODE_AREA
void rtc_print_current_time(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* 读取当前RTC时间 */
    err = R_RTC_CalendarTimeGet(&g_rtc_ctrl, &g_current_time);
    if (FSP_SUCCESS != err)
    {
        printf("Get RTC time failed! Error code: 0x%x\r\n", err);
        return;
    }

    /* 转换为用户可读格式（补回偏移） */
    g_current_time.tm_mon  += MON_ADJUST_VALUE;
    g_current_time.tm_year += YEAR_ADJUST_VALUE;

    /* 格式化打印：年-月-日 时:分:秒（补零保证两位） */
    printf("Current Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
              g_current_time.tm_year, g_current_time.tm_mon, g_current_time.tm_mday,
              g_current_time.tm_hour, g_current_time.tm_min, g_current_time.tm_sec);

    /* 恢复结构体原始值（避免下次读取叠加偏移） */
    g_current_time.tm_mon  -= MON_ADJUST_VALUE;
    g_current_time.tm_year -= YEAR_ADJUST_VALUE;
}

/*******************************************************************************************************************//**
 * @brief 主测试函数：初始化RTC + 设置初始时间 + 每秒打印时间
 * @retval None
 **********************************************************************************************************************/
CODE_AREA
void rtc_simple_test(void)
{
     fsp_err_t err = FSP_SUCCESS;
     fsp_pack_version_t version;
     R_FSP_VersionGet(&version);
     printf(BANNER_1);
     printf(BANNER_2);
     printf(BANNER_3,EP_VERSION);
     printf(BANNER_4,version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
     printf(BANNER_5);
     printf(BANNER_6);
    /* 1. 初始化RTC（打开驱动+设置1秒中断） */
    err = rtc_simple_init();
    if (FSP_SUCCESS != err)
    {
        APP_ERR_TRAP(err);  // 初始化失败则终止
    }

    /* 2. 设置RTC初始时间 */
    rtc_set_initial_time();

    /* 3. 主循环：检测1秒中断标记，打印时间 */
    printf("Start printing time every 1 second...\r\n");
    while(1)
    {
        if (g_1s_tick_flag == 1)
        {
            rtc_print_current_time();  // 打印当前时间
            g_1s_tick_flag = 0;        // 清除中断标记
        }
    }
}

