/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : world_screen
 * Version      : .
 * Description  : The led demo screen display.
 *********************************************************************************************************************/

#include <math.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "bsp_api.h"

#include "common_utils.h"
#include "common_init.h"
#include "board_cfg.h"
#include "menu_camview.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"
#include "r_rtc_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "bg_font_18_full.h"
#include "gimp.h"
#include "lcd.h"

#define WC_OFFSET                  6

#define CURRENCY_HB_WIDTH_SIZE     (48)
#define CURRENCY_HB_HEIGHT_SIZE    (48)

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   in_transition(void);

typedef struct st_world_screen_indicator_def
{
    uint32_t           id;
    char_t           * p_name_str;
    st_lcd_point_def_t center;
    int32_t            utcOffset;
    bool_t             in_use;         // blink indicator position in radians
} st_world_screen_indicator_def_t;

static st_world_screen_indicator_def_t world_screen_control[] =
{
    {GI_SAN_FRANSICSCO, "San Francisco, United States", {120,  50}, -8, false},
    {GI_MEXICO_CITY,    "Mexico City, Mexico",          {181, 181}, -6, false},
    {GI_TORONTO,        "Toronto, Canada",              {226, 227}, -5, false},
    {GI_SANTIAGO,       "Santiago, Chile",              {226,  46}, -3, false},
    {GI_SAO_PAULO,      "Sao Paulo, Brazil",            {297, 108}, -3, false},
    {GI_LONDON,         "London, England",              {100, 130}, +0, false},
    {GI_BERLIN,         "Berlin, Germany",              {402, 236}, +1, false},
    {GI_CAPE_TOWN,      "Cape Town, South Africa",      {418,  51}, +2, false},
    {GI_MOSCOW,         "Moscow, Russia",               {485, 265}, +3, false},
    {GI_NEW_DELHI,      "New Delhi, India",             {530, 183}, +5, false},
    {GI_SINGAPHORE,     "Singapore, Singapore",         {592, 130}, +8, false},
    {GI_BEIJING,        "Beijing, China",               {110, 300}, +8, false},
    {GI_TOKYO,          "Tokyo, Japan",                 {130, 340}, +9, false},
    {GI_SYDNEY,         "Sydney, Australia",            {686,  54}, +11,false},
    {GI_AUCKLAND,       "Auckland, New Zealand",        {717,  46}, +13,false}
};

static void reset_rtc(void);

bool_t convert_gmt_to_region(uint32_t region, rtc_time_t * time);

extern bool_t https_local_time_update(void);

extern char_t       g_current_time_raw_str[];

void do_world_screen(void);

bool_t convert_gmt_to_region (uint32_t region, rtc_time_t * time)
{
    bool_t  is_am   = true;
    int32_t local_h = 0;

    if (https_local_time_update())
    {
        reset_rtc();
    }

    R_RTC_CalendarTimeGet(&g_rtc_ctrl, time);
    local_h = (int16_t) time->tm_hour;

    /* Beijing time and Tokyo time are one hour ahead. Subtract here */
    if ((region == GI_BEIJING) || (region == GI_TOKYO)) {
        if (local_h > 0) {
            local_h--;
        }
        else {
            local_h = 23;
        }
    }

    time->tm_hour += (int) (time->tm_hour + world_screen_control[region].utcOffset);
    local_h       += world_screen_control[region].utcOffset;

    /* special case for 9, "New Delhi, India", */
    if (region == 9)
    {
        time->tm_min += 30;
        if (time->tm_min > 59)
        {
            local_h      += 1;
            time->tm_min %= 60;
        }
    }

    if (local_h < 0)
    {
        local_h += 12;
    }

    if (local_h > 23)
    {
        local_h %= 24;
    }

    if (local_h > 12)
    {
        local_h %= 12;
        is_am    = false;
    }

    if (local_h == 12)
    {
        is_am = false;
    }

    time->tm_hour = local_h;

    return is_am;
}

bool_t show_it = false;

static void reset_rtc (void)
{
    rtc_time_t set_time         = {};
    char_t     index_minuite[8] = {};
    char_t     index_hours[8]   = {};

    char_t * p_index_localtime = strstr(g_current_time_raw_str, "localtime\"");
    p_index_localtime = strstr(p_index_localtime, " ");

    memcpy(index_hours, strstr(p_index_localtime, " ") + 1, 2);

    p_index_localtime++;

    memcpy(index_minuite, strstr(p_index_localtime, ":") + 1, 2);

    set_time.tm_hour = atoi(index_hours);
    set_time.tm_min  = atoi(index_minuite);

    /* Set the calendar time */
    R_RTC_CalendarTimeSet(&g_rtc_ctrl, &set_time);
}

void do_world_screen(void)
{
    static bool_t  update_time    = false;
    static int32_t refresh_screen = 0;
    static uint16_t call_cnt = 0;
    static int pic_index = PICTURE_INDEX_SAN_FRANCISCO;
    static int world_screen_index = GI_SAN_FRANSICSCO;

    st_gimp_image_t img;

    graphics_wait_vsync();
    graphics_start_frame();

    if (in_transition())
    {
        update_time   = false;

        img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_WORLD_TIME);
        d2_setblitsrc(d2_handle, img.pixel_data, LCD_HPIX, LCD_HPIX, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, LCD_HPIX, LCD_VPIX, 0, 0, LCD_HPIX << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);
    }
    else {
        update_time = true;
    }

    call_cnt++;
    if (call_cnt == 200) {
        call_cnt = 0;
        refresh_screen = 2;

        /* Since QSPI Flash can't work properly, so specify these values directly */
        switch (pic_index) {
            case PICTURE_INDEX_SAN_FRANCISCO:
                pic_index = PICTURE_INDEX_LONDON;
                world_screen_index = GI_LONDON;
                break;
            case PICTURE_INDEX_LONDON:
                pic_index = PICTURE_INDEX_BEIJING;
                world_screen_index = GI_BEIJING;
                break;
            case PICTURE_INDEX_BEIJING:
                pic_index = PICTURE_INDEX_TOKYO;
                world_screen_index = GI_TOKYO;
                break;
            case PICTURE_INDEX_TOKYO:
            default:
                pic_index  = PICTURE_INDEX_SAN_FRANCISCO;
                world_screen_index = GI_SAN_FRANSICSCO;
                break;
        }
    }

    if (update_time == true)
    {
        st_gimp_city_image_t simg;

        rtc_time_t local_time = {};
        bool_t is_am = false;

        update_time = false;

        if (refresh_screen > 0)
        {
            refresh_screen--;
            img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_WORLD_TIME);
            d2_setblitsrc(d2_handle, img.pixel_data, LCD_HPIX, LCD_HPIX, LCD_VPIX, EP_SCREEN_MODE);
            d2_blitcopy(d2_handle, LCD_HPIX, LCD_VPIX, 0, 0, LCD_HPIX << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);
        }

        is_am = convert_gmt_to_region(world_screen_control[world_screen_index].id, &local_time);

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wimplicit-int-conversion"
        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(pic_index);
    #pragma clang diagnostic pop
        simg.bytes_per_pixel = 2;
        simg.height = 66;
        simg.width = 133;

        d2_setblitsrc(d2_handle, simg.pixel_data, simg.width, simg.width, simg.height, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle,
                   (d2_width)simg.width,
                   (d2_width)simg.height,
                   0,
                   0,
                   (d2_width)(simg.width << 4),
                   (d2_width)(simg.height << 4),
                   (d2_width)((world_screen_control[world_screen_index].center.verticle) << 4),
                   (d2_width)((world_screen_control[world_screen_index].center.horizontal) << 4),
                   d2_tm_filter);

        char msg3[32] = {};

        sprintf(msg3, "%2d:%02d.%02d %s ", local_time.tm_hour, local_time.tm_min, local_time.tm_sec, is_am ? "AM" : "PM");

        d2_point vpos = world_screen_control[world_screen_index].center.verticle + 15;
        d2_point hpos = world_screen_control[world_screen_index].center.horizontal + 40;

        print_bg_font_18(d2_handle, vpos, hpos, msg3);
    }

    d2_flushframe(d2_handle);
    graphics_end_frame();
}
