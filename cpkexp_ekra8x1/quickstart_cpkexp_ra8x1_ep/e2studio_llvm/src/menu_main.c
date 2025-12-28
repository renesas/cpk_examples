/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : menu_main.c
 * Description  : The main menu controller.
 *********************************************************************************************************************/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "menu_main.h"
#include "common_utils.h"
#include "common_init.h"
#include "menu_main.h"
#include "jlink_console.h"
#include "menu_camview.h"

#define MODULE_NAME              "\r\n\x1b[2m\x1b[37m欢迎来到 %s QSEP 工程!\r\n"

#define SUB_OPTIONS              "\r\n> Select from the options in the menu below:\r\n" \
                                 "\r\nMENU"

#define CONNECTION_ABORT_CRTL    (0x00)
#define MENU_EXIT_CRTL           (0x20)
#define ETH_TIMEOUT    (16000)  /* It takes longer to refresh the weather information */

typedef struct menu_fn_tbl
{
    char_t * p_name;                   /*<! Name of Test */
    test_fn (* p_func)(void);          /*<! Pointer to Test Function */
} st_menu_fn_tbl_t;

extern bool_t  is_ethernet_mode(void);
extern bool_t  erase_stored_keys(void);
extern test_fn camview_display_menu(bool_t first_call);

/* Table of menu functions */
static st_menu_fn_tbl_t s_menu_items[] =
{
    {"开发板信息", kis_display_menu},
    {"交互式 AI, 连接性与人机接口演示", camview_display_menu},
    {"更多信息", ns_display_menu},
    {"", NULL}
};

int8_t g_selected_menu = 0;

static char_t s_print_buffer[BUFFER_LINE_LENGTH] = {};
static bool_t s_bool_screen_test = true;
static uint32_t timeout = ETH_TIMEOUT;

int8_t main_display_menu(void)
{
    int8_t c          = -1;
    int8_t menu_limit = 0;

    if (s_bool_screen_test == true) {
        sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
        g_selected_menu = 2;
        camview_display_menu(s_bool_screen_test);
        s_bool_screen_test = false;
    }

    sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);

    /* ignoring -Wpointer-sign is OK when treating signed char_t array as as unsigned */
    print_to_console((void *) s_print_buffer);
    sprintf(s_print_buffer, MODULE_NAME, FULL_NAME);
    print_to_console((void *) s_print_buffer);

    int8_t test_active = 0;
    int8_t limit       = 2;            // append next steps dynamically

    for ( ; test_active < limit; test_active++) {
        sprintf(s_print_buffer, "\r\n %d. %s", (test_active + 1), s_menu_items[menu_limit++].p_name);
        print_to_console((void *) s_print_buffer);
    }

    if ((true == is_ethernet_mode()) && (false == is_camera_mode())) {
        test_active++;
        menu_limit++;
        sprintf(s_print_buffer, "\r\n %d. 刷新货币转换汇率", (test_active++));
        print_to_console((void *) s_print_buffer);

        menu_limit++;
        sprintf(s_print_buffer, "\r\n %d. 刷新世界时钟", (test_active++));
        print_to_console((void *) s_print_buffer);

        menu_limit++;
        sprintf(s_print_buffer, "\r\n %d. 刷新世界天气", (test_active++));
        print_to_console((void *) s_print_buffer);

        menu_limit++;
        sprintf(s_print_buffer, "\r\n %d. API keys 帮助与管理", (test_active));
        print_to_console((void *) s_print_buffer);
    }

    menu_limit++;
    test_active++;
    sprintf(s_print_buffer, "\r\n %d. %s", (test_active), s_menu_items[2].p_name);
    print_to_console((void *) s_print_buffer);

    print_to_console("\r\n");

    while ((0 != c))
    {
        c = input_from_console();
        if (0 != c) {
            c = (int8_t) (c - '0');
            g_selected_menu = c;

            bool_t esc_sequence = false;
            if ((c > 0) && (c <= menu_limit))
            {
                if (c == 2)
                {
                    camview_display_menu(s_bool_screen_test);
                }
                else {
                    if (true == is_ethernet_mode()) {
                        if (3 == c) {
                            sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
                            print_to_console((void *) s_print_buffer);

                            print_to_console(" 3. 刷新货币转换汇率\r\n\r\n");

                            print_to_console("当首次刷新汇率信息时，QSEP 会要求用户输入 API key。");
                            print_to_console("\r\n当用户输入 API key 后，它会存储在 RA8D1 MCU 的 Data Flash 中。");
                            print_to_console("\r\n在之后的刷新中，QSEP 会使用已存储的 API key 从 app.currencyapi.com 获取数据。");
                            print_to_console("\r\n请确认以太网电缆已稳定连接且网络可用（例如不被防火墙阻止等）");
                            print_to_console("\r\n按任意键开始刷新，按空格键返回菜单\r\n");

                            esc_sequence = false;

                            while (1) {
                                c = input_from_console();
                                if (c == MENU_EXIT_CRTL) // MENU_EXIT_CRTL
                                {
                                    esc_sequence = true;
                                }

                                break;
                            }

                            if (esc_sequence == false) {
                                /* User has pressed signal Ethernet to start Can be set many times but should only be needed once */
                                xEventGroupSetBits(g_update_console_event, STATUS_ENABLE_ETHERNET);

                                print_to_console("\r\n数据正在刷新，这可能花费数分钟，请耐心等待\r\n");
                                vTaskDelay(500);
                                xEventGroupSetBits(g_update_console_event, STATUS_IOT_REQUEST_CURRENCY);

                                timeout = ETH_TIMEOUT / 2;

                                EventBits_t uxBits;
                                while (timeout) {
                                    uxBits = xEventGroupWaitBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE, pdFALSE, pdTRUE, 1);
                                    vTaskDelay(10);
                                    timeout--;

                                    if ((uxBits & (STATUS_IOT_RESPONSE_COMPLETE)) == (STATUS_IOT_RESPONSE_COMPLETE))
                                    {
                                        xEventGroupClearBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);

                                        break;
                                    }
                                }

                                if (timeout == 0)
                                {
                                    esc_sequence = false;

                                    /* Let interested parties know timout has occurred */
                                    eth_escape();

                                    print_to_console("\r\n不能连接以太网，请按以下步骤进行检查并重试：\r\n");
                                    print_to_console("\r\n1. 以太网电缆已稳定连接\r\n");
                                    print_to_console("\r\n2. 路由设备可以访问以太网\r\n");
                                    print_to_console("\r\n3. 防火墙没有阻止网络连接\r\n");

                                    print_to_console("\r\n按空格键返回菜单\r\n");
                                }
                            }

                            if (did_data_abort_from_eth() == false)
                            {
                                if (esc_sequence == false)
                                {
                                    while (CONNECTION_ABORT_CRTL != c)
                                    {
                                        c = input_from_console();
                                        if ((MENU_EXIT_CRTL == c) || (CONNECTION_ABORT_CRTL == c))
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if (4 == c)
                        {
                            sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
                            print_to_console((void *) s_print_buffer);

                            print_to_console(" 4. 刷新世界时钟信息\r\n\r\n");

                            print_to_console("当首次刷新世界时钟信息时，QSEP 会要求用户输入 API key。");
                            print_to_console("\r\n当用户输入 API key 后，它会存储在 RA8D1 MCU 的 Data Flash 中。");
                            print_to_console("\r\n在之后的刷新中，QSEP 会使用已存储的 API key 从 www.weatherapi.com 获取数据。");
                            print_to_console("\r\n请确认以太网电缆已稳定连接且网络可用（例如不被防火墙阻止等）");
                            print_to_console("\r\n按任意键开始刷新，按空格键返回菜单\r\n");

                            esc_sequence = false;

                            while (1)
                            {
                                c = input_from_console();
                                if (c == MENU_EXIT_CRTL) // MENU_EXIT_CRTL
                                {
                                    esc_sequence = true;
                                }

                                break;
                            }

                            if (esc_sequence == false)
                            {
                                /* User has pressed signal Ethernet to start Can be set many times but should only be needed once */
                                xEventGroupSetBits(g_update_console_event, STATUS_ENABLE_ETHERNET);

                                print_to_console("\r\n数据正在刷新，这可能花费数分钟，请耐心等待\r\n");

                                vTaskDelay(500);
                                xEventGroupSetBits(g_update_console_event, STATUS_IOT_REQUEST_TIME);

                                timeout = ETH_TIMEOUT;

                                EventBits_t uxBits;
                                while (timeout)
                                {
                                    uxBits = xEventGroupWaitBits(g_update_console_event,
                                                                 STATUS_IOT_RESPONSE_COMPLETE,
                                                                 pdFALSE,
                                                                 pdTRUE,
                                                                 1);
                                    vTaskDelay(10);
                                    timeout--;

                                    if ((uxBits & (STATUS_IOT_RESPONSE_COMPLETE)) == (STATUS_IOT_RESPONSE_COMPLETE))
                                    {
                                        xEventGroupClearBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);

                                        break;
                                    }
                                }

                                if (timeout == 0)
                                {
                                    esc_sequence = false;

                                    /* Let interested parties know timout has occurred */
                                    eth_escape();

                                    print_to_console("\r\n不能连接以太网，请按以下步骤进行检查并重试：\r\n");
                                    print_to_console("\r\n1. 以太网电缆已稳定连接\r\n");
                                    print_to_console("\r\n2. 路由设备可以访问以太网\r\n");
                                    print_to_console("\r\n3. 防火墙没有阻止网络连接\r\n");

                                    print_to_console("\r\n按空格键返回菜单\r\n");
                                }
                            }

                            if (did_data_abort_from_eth() == false)
                            {
                                if (esc_sequence == false)
                                {
                                    while (CONNECTION_ABORT_CRTL != c)
                                    {
                                        c = input_from_console();
                                        if ((MENU_EXIT_CRTL == c) || (CONNECTION_ABORT_CRTL == c))
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if (5 == c)
                        {
                            sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
                            print_to_console((void *) s_print_buffer);

                            print_to_console(" 5. 刷新世界天气\r\n\r\n");

                            print_to_console("当首次刷新世界天气信息时，QSEP 会要求用户输入 API key。");
                            print_to_console("\r\n当用户输入 API key 后，它会存储在 RA8D1 MCU 的 Data Flash 中。");
                            print_to_console("\r\n在之后的刷新中，QSEP 会使用已存储的 API key 从 www.weatherapi.com 获取数据。");
                            print_to_console("\r\n请确认以太网电缆已稳定连接且网络可用（例如不被防火墙阻止等）");
                            print_to_console("\r\n按任意键开始刷新，按空格键返回菜单\r\n");

                            esc_sequence = false;

                            while (1)
                            {
                                c = input_from_console();
                                if (c == MENU_EXIT_CRTL) // MENU_EXIT_CRTL
                                {
                                    esc_sequence = true;
                                }

                                break;
                            }

                            if (esc_sequence == false)
                            {
                                /* User has pressed signal Ethernet to start Can be set many times but should only be needed once */
                                xEventGroupSetBits(g_update_console_event, STATUS_ENABLE_ETHERNET);

                                print_to_console("\r\n数据正在刷新，这可能花费数分钟，请耐心等待\r\n");

                                vTaskDelay(500);
                                xEventGroupSetBits(g_update_console_event, STATUS_IOT_REQUEST_WEATHER);
                                EventBits_t uxBits;

                                timeout = ETH_TIMEOUT;

                                while (timeout)
                                {
                                    uxBits = xEventGroupWaitBits(g_update_console_event,
                                                                 STATUS_IOT_RESPONSE_COMPLETE,
                                                                 pdFALSE,
                                                                 pdTRUE,
                                                                 1);
                                    vTaskDelay(10);
                                    timeout--;

                                    if ((uxBits & (STATUS_IOT_RESPONSE_COMPLETE)) == (STATUS_IOT_RESPONSE_COMPLETE))
                                    {
                                        xEventGroupClearBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);

                                        break;
                                    }
                                }

                                if (timeout == 0)
                                {
                                    esc_sequence = false;

                                    /* Let interested parties know timout has occurred */
                                    eth_escape();

                                    print_to_console("\r\n不能连接以太网，请按以下步骤进行检查并重试：\r\n");
                                    print_to_console("\r\n1. 以太网电缆已稳定连接\r\n");
                                    print_to_console("\r\n2. 路由设备可以访问以太网\r\n");
                                    print_to_console("\r\n3. 防火墙没有阻止网络连接\r\n");

                                    print_to_console("\r\n按空格键返回菜单\r\n");
                                }
                            }

                            if (did_data_abort_from_eth() == false)
                            {
                                if (esc_sequence == false)
                                {
                                    while (CONNECTION_ABORT_CRTL != c)
                                    {
                                        c = input_from_console();
                                        if ((MENU_EXIT_CRTL == c) || (CONNECTION_ABORT_CRTL == c))
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if (6 == c)
                        {
                            bool_t  res = true;
                            uint8_t c   = (uint8_t) -1;

                            sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
                            print_to_console((void *) s_print_buffer);

                            print_to_console(" 6. API KEYS 帮助和管理\r\n\r\n");

                            print_to_console("连接性演示应用需要 API key 从第三方服务获取实时的数据。\r\n");
                            print_to_console("用户需要在第三方服务提供商处登录或创建一个账号以获得 API key\r\n");

                            print_to_console(" 1. 世界天气和世界时钟演示需要从以下网址获取一个相同的 API key\r\n    weatherapi.com\r\n");
                            print_to_console(" 2. 货币转换演示需要从以下网址获取一个 API key\r\n    app.currencyapi.com\r\n");

                            print_to_console("\r\n当首次刷新连接性应用时，QSEP 会要求用户输入 API key。\r\n");
                            print_to_console("当用户输入 API key 后，它会存储在 RA8D1 MCU 的 Data Flash 中。\r\n");
                            print_to_console("之后 QSEP 会在 MCU 中寻找 API keys。\r\n");
                            print_to_console("如果找到，将会使用现有 API key 从第三方服务获取当前数据\r\n");

                            print_to_console("\r\n> 按 'E' 擦除已存在的 API key\r\n  或按空格返回菜单\r\n\r\n");

                            start_key_check();

                            while (1)
                            {
                                if (true == key_pressed())
                                {
                                    c = get_detected_key();

                                    if (('E' == c) || ('e' == c))
                                    {
                                        res = true;
                                        break;
                                    }

                                    if (MENU_EXIT_CRTL == c) // SPACE Key
                                    {
                                        res = false;
                                        break;
                                    }

                                    start_key_check();
                                }
                            }

                            if (false == res)
                            {
                            }
                            else
                            {
                                erase_stored_keys();
                                print_to_console("完成 - 数据已擦除。\r\n");

                                sprintf(s_print_buffer, MENU_RETURN_INFO);
                                print_to_console((void *) s_print_buffer);

                                while (CONNECTION_ABORT_CRTL != c)
                                {
                                    c = input_from_console();
                                    if ((MENU_EXIT_CRTL == c) || (CONNECTION_ABORT_CRTL == c))
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (c == menu_limit)
                            {
                                s_menu_items[2].p_func();
                            }
                            else
                            {
                                s_menu_items[c - 1].p_func();
                            }
                        }
                    }
                    else
                    {
                        if (c == menu_limit)
                        {
                            s_menu_items[2].p_func();
                        }
                        else
                        {
                            s_menu_items[c - 1].p_func();
                        }
                    }
                }

                break;
            }
        }
    }

    /* Cast, as compiler will assume calc is int */
    return (int8_t) (c - '0');
}
