/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : led_screen_demo.c
 * Version      : .
 * Description  : The led demo screen display.
 *********************************************************************************************************************/

#include <math.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "common_utils.h"
#include "common_init.h"
#include "menu_camview.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "camera_layer.h"
#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "fg_font_22_full.h"
#include "gimp.h"
#include "lcd.h"

#define NUM_WEATHER_IMAGE_ICONS    (4)
#define RIGHT_HAND_DETECT          (554)
#define LEFT_HAND_DETECT           (300)

extern void   reset_transition(void);
extern bool_t in_transition(void);

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   convert_gmt_to_region(uint32_t region, rtc_time_t * time);

extern bool_t https_weather_update(void);

// Simulated / live data if available
extern char_t * gp_weather_full_table[16];

static char_t s_print_buffer[BUFFER_LINE_LENGTH] = {};
static char_t s_print_buffer2[64] = {};

static int32_t selected_icon;

#define RESET_DETECT                  10
#define GI_WEATHER_ICON_SCREEN_MAX    15
#define NUM_ICONS_AVAILABLE           4

#define FG_PANNEL_VERTICAL            (0)

#define TEMP_C_DISP_STR               "'C"
#define TEMP_F_DISP_STR               "'F"

#define TEMP_C_KEY_STR                "temp_c"
#define TEMP_F_KEY_STR                "temp_f"

#define WIND_DISP_MPH_STR             "miles/hr"
#define WIND_DISP_KMH_STR             "km/hr"

#define WIND_MPH_KEY_STR              "wind_mph"
#define WIND_KMH_KEY_STR              "wind_kph"

typedef struct st_weather_screen_indicator_def
{
    uint32_t id;                       // Weather Screen
    uint32_t timezone_id;              // Nearest Timezone (from WTZ page)
    char_t * p_name_str;               // User Readable Name
    char_t * p_temp_key_str;           // Lookup key for temperature
    char_t * p_wind_key_str;           // lookup key for wind speed
    char_t * p_temp_str;               // Display for temperature
    char_t * p_wind_str;               // Display for wind speed
} st_weather_screen_indicator_def_t;

static st_weather_screen_indicator_def_t weather_screen_control[] =
{
    {GI_WEATHER_HONG_KONG,      GI_TOKYO,          "Hong Kong",      TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_KYOTO,          GI_TOKYO,          "Kyoto",          TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_LONDON,         GI_LONDON,         "London",         TEMP_C_KEY_STR, WIND_MPH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_MPH_STR},
    {GI_WEATHER_MIAMI,          GI_MEXICO_CITY,    "Miami",          TEMP_F_KEY_STR, WIND_MPH_KEY_STR, TEMP_F_DISP_STR, WIND_DISP_MPH_STR},
    {GI_WEATHER_MUNICH,         GI_BERLIN,         "Munich",         TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_NEW_YORK,       GI_SAN_FRANSICSCO, "New York",       TEMP_F_KEY_STR, WIND_MPH_KEY_STR, TEMP_F_DISP_STR, WIND_DISP_MPH_STR},
    {GI_WEATHER_PARIS,          GI_BERLIN,         "Paris",          TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_PRAGUE,         GI_BERLIN,         "Prague",         TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_QUEENSTOWN,     GI_AUCKLAND,       "Queenstown",     TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_RIO_DE_JANERIO, GI_SAO_PAULO,      "Rio de Janeiro", TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_ROME,           GI_BERLIN,         "Rome",           TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_SAN_FRANCISCO,  GI_SAN_FRANSICSCO, "San Francisco",  TEMP_F_KEY_STR, WIND_MPH_KEY_STR, TEMP_F_DISP_STR, WIND_DISP_MPH_STR},
    {GI_WEATHER_SHANGHAI,       GI_BEIJING,        "Shanghai",       TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_SINGAPORE,      GI_SINGAPHORE,     "Singapore",      TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_SYDNEY,         GI_SYDNEY,         "Sydney",         TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR},
    {GI_WEATHER_TORONTO,        GI_TORONTO,        "Toronto",        TEMP_C_KEY_STR, WIND_KMH_KEY_STR, TEMP_C_DISP_STR, WIND_DISP_KMH_STR}
};

#define STD_SIZE    8
#define MED_SIZE    12
#define LDG_SIZE    48

static char_t str_condition[LDG_SIZE] = {};

uint16_t static_x = 190;
uint16_t static_y = 68;

#define LINE_1_FORMAT    "%s %s             "
#define LINE_2_FORMAT    "%s %s from %s"
#define LINE_3_FORMAT    "%s%%"

static char_t clear_line[56] = "                                  ";
static char_t line1[96] = {};
static char_t line2[56] = {};
static char_t line3[56] = {};

static bool_t first_call = true;
static int32_t current_screen = GI_WEATHER_SHANGHAI - GI_WEATHER_HONG_KONG;
static int32_t current_weather_screen = PICTURE_INDEX_WEATHER_SHANGHAI;
static st_gimp_fg_icon_image_t icon_erase;
static guint8 s_icon_erase_pixel[113 * 126 * 4 + 1];

static display_runtime_cfg_t glcd_layer_change;

void        do_weather_screen(void);
static void draw_with_alpha(st_gimp_fg_icon_image_t * object, uint16_t hoffset, uint16_t voffset);

static void draw_with_alpha (st_gimp_fg_icon_image_t * object, uint16_t hoffset, uint16_t voffset)
{
    /* GLCDC Graphics Layer 1 size must be 182 x 200 */
    uint8_t * buf_ptr = (uint8_t *) fb_foreground;
    display_input_cfg_t const * p_input = &g_display0.p_cfg->input[1]; // Layer 2

    /* Supports up to 4 bytes per pixel  */
    if ((uint32_t) object->bytes_per_pixel > 4)
    {

        /* The input images specified in object has been corrupted */
        return;
    }

    /* clear screen */

    // memset(buf_ptr, 0, p_input->hstride * p_input->vsize * 4);

    /* offset 200 horizontal */
    buf_ptr += (hoffset * (p_input->hstride * 4)); // Horizontal offset
    buf_ptr += (voffset * 4);                      // Vertical offset

    uint32_t bpp = object->bytes_per_pixel;

    for (uint32_t v = 0; v < object->height; v++)
    {
        uint32_t v_offset = v * object->width * bpp;
        for (uint32_t h = 0; h < (object->width * bpp); h += bpp)
        {
            buf_ptr[0] = object->pixel_data[v_offset + h + 2]; // B
            buf_ptr[1] = object->pixel_data[v_offset + h + 1]; // G
            buf_ptr[2] = object->pixel_data[v_offset + h];     // R
            buf_ptr[3] = object->pixel_data[v_offset + h + 3]; // A                                     // A (Unused)
            buf_ptr   += 4;
        }

        buf_ptr += ((p_input->hstride - p_input->hsize) * 4);

        /* align image size to be drawn with the surface size */
        buf_ptr += ((p_input->hsize - object->width) * bpp);
    }
}

#define ANIMATION_FRAME_RESET_VALUE    6
#define NUM_VIEWER_IMAGES              16
uint16_t SELECTED = 0;

static uint32_t decide_icon(uint32_t region, char_t * str_cur_time, char * condition, bool_t snow, bool_t windy);

static uint32_t s_animation_frame = ANIMATION_FRAME_RESET_VALUE;

static void reset_animation (void)
{
    bool_t     is_am      = false;
    rtc_time_t local_time = {};

    s_animation_frame = ANIMATION_FRAME_RESET_VALUE;

    glcd_layer_change.layer.coordinate.x = FG_PANNEL_VERTICAL;
    glcd_layer_change.layer.coordinate.y = 0;

    glcd_layer_change.input = g_display0.p_cfg->input[1];
    R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);

    if (https_weather_update() == true)
    {
        /* update live data; */
    }

    static char_t str_condition2[LDG_SIZE] = {};
    static char_t str_cur_time[LDG_SIZE]   = "14:00";
    static char_t str_temp[STD_SIZE]       = {};
    static char_t str_wind_speed[STD_SIZE] = {};
    static char_t str_wind_dir[STD_SIZE]   = {};
    static char_t str_humidity[STD_SIZE]   = {};
    static char_t str_snow[STD_SIZE]       = {};
    static char_t str_windy[STD_SIZE]      = {};

    memset(str_condition2, 0, LDG_SIZE);
    memset(str_cur_time, 0, LDG_SIZE);
    memset(str_temp, 0, STD_SIZE);
    memset(str_wind_speed, 0, STD_SIZE);
    memset(str_wind_dir, 0, STD_SIZE);
    memset(str_humidity, 0, STD_SIZE);
    memset(str_snow, 0, STD_SIZE);
    memset(str_windy, 0, STD_SIZE);

    bool_t snow  = false;
    bool_t windy = false;

    is_am = convert_gmt_to_region(weather_screen_control[current_screen].timezone_id, &local_time);

    if ((is_am == false) && (local_time.tm_hour != 12))
    {
        sprintf(str_cur_time, "%02d:00", local_time.tm_hour + 12);
    }
    else
    {
        sprintf(str_cur_time, "%02d:00", local_time.tm_hour);
    }

    char_t * p1 = strstr(gp_weather_full_table[current_screen], str_cur_time);

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, weather_screen_control[current_screen].p_temp_key_str, &str_temp[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, (size_t) len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    /* find weather_screen_control[current_screen].p_temp_key_str */
                    char_t * p3 = strstr(s_print_buffer, weather_screen_control[current_screen].p_temp_key_str);
                    if (p3 != NULL)    // "temp_f":77.0,
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, (size_t) len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);
                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';
                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_temp, "%s", (p5 + 1));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, "condition", &str_condition[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, (size_t) len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    /* find weather_screen_control[current_screen].p_temp_key_str */
                    char_t * p3 = strstr(s_print_buffer, "condition");
                    if (p3 != NULL)    // "condition"
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, (size_t) len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

// print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

// print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, "\":\"");
                            if (p5 != NULL)
                            {
                                sprintf(str_condition2, "%s", (p5 + 3));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, weather_screen_control[current_screen].p_wind_key_str, &str_wind_speed[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, (size_t) len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    // find weather_screen_control[current_screen].p_temp_key_str
                    char_t * p3 = strstr(s_print_buffer, weather_screen_control[current_screen].p_wind_key_str);
                    if (p3 != NULL)    // p_wind_key_str
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, (size_t) len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_wind_speed, "%s", (p5 + 1));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, weather_screen_control[current_screen].p_wind_key_str, &str_wind_speed[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, (size_t) len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    /* find weather_screen_control[current_screen].p_temp_key_str */
                    char_t * p3 = strstr(s_print_buffer, weather_screen_control[current_screen].p_wind_key_str);
                    if (p3 != NULL)    // p_wind_key_str
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, (size_t) len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_wind_speed, "%s", (p5 + 1));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, "wind_dir", &str_wind_dir[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, (size_t) len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    // find weather_screen_control[current_screen].p_temp_key_str
                    char_t * p3 = strstr(s_print_buffer, "wind_dir");
                    if (p3 != NULL)    // wind_dir
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, (size_t) len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_wind_dir, "%s", (p5 + 1));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, "humidity", &str_humidity[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    // find weather_screen_control[current_screen].p_temp_key_str
                    char_t * p3 = strstr(s_print_buffer, "humidity");
                    if (p3 != NULL)    // humidity
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_humidity, "%s", (p5 + 1));
                            }
                        }
                    }
                }
            }
        }
    }

    // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, "will_it_snow", &str_snow[0]);
    {
        if (p1 != NULL)
        {
            char_t * p2 = strstr(p1, "uv");

            if (p2 != NULL)
            {
                int len = p2 - p1;

                if (len > 0)
                {
                    sprintf(s_print_buffer, "len [%d]\r\n", len);

                    // print_to_console(s_print_buffer);

                    memcpy(s_print_buffer, p1, len);
                    s_print_buffer[len] = '\0';

                    // print_to_console(s_print_buffer);

                    // find weather_screen_control[current_screen].p_temp_key_str
                    char_t * p3 = strstr(s_print_buffer, "will_it_snow");
                    if (p3 != NULL)    // humidity
                    {
                        char_t * p4   = strstr(p3, ",");
                        int      len2 = p4 - p3;

                        if (len2 > 0)
                        {
                            memcpy(s_print_buffer2, p3, len2);

                            sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                            // print_to_console(s_print_buffer);

                            s_print_buffer2[len2] = '\0';

                            // print_to_console(s_print_buffer2);

                            char_t * p5 = strstr(s_print_buffer2, ":");
                            if (p5 != NULL)
                            {
                                sprintf(str_snow, "%s", (p5 + 1));
                                if (atoi(str_snow) == 1)
                                {
                                    snow = true;
                                }
                            }
                        }
                    }
                }
            }

            // json_get_data_from_key(gp_weather_full_table[current_screen % NUM_ICONS_AVAILABLE],str_cur_time, "will_it_rain", &str_rain[0]);
            {
                if (p1 != NULL)
                {
                    char_t * p2 = strstr(p1, "uv");

                    if (p2 != NULL)
                    {
                        int len = p2 - p1;

                        if (len > 0)
                        {
                            sprintf(s_print_buffer, "len [%d]\r\n", len);

                            // print_to_console(s_print_buffer);

                            memcpy(s_print_buffer, p1, len);
                            s_print_buffer[len] = '\0';

                            // print_to_console(s_print_buffer);

                            // find weather_screen_control[current_screen].p_temp_key_str
                            char_t * p3 = strstr(s_print_buffer, "wind_mph");
                            if (p3 != NULL) // humidity
                            {
                                char_t * p4   = strstr(p3, ",");
                                int      len2 = p4 - p3;

                                if (len2 > 0)
                                {
                                    memcpy(s_print_buffer2, p3, len2);

                                    sprintf(s_print_buffer, "len2 [%d]\r\n", len2);

                                    // print_to_console(s_print_buffer);

                                    s_print_buffer2[len2] = '\0';

                                    // print_to_console(s_print_buffer2);

                                    char_t * p5 = strstr(s_print_buffer2, ":");
                                    if (p5 != NULL)
                                    {
                                        sprintf(str_windy, "%s", (p5 + 1));
                                        if (atof(str_windy) > 8.0)
                                        {
                                            windy = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    selected_icon = decide_icon(current_screen % NUM_ICONS_AVAILABLE, str_cur_time, str_condition, snow, windy);

    sprintf(line1, LINE_1_FORMAT, str_temp, weather_screen_control[current_screen].p_temp_str);
    sprintf(line2, LINE_2_FORMAT, str_wind_speed, weather_screen_control[current_screen].p_wind_str, str_wind_dir);
    sprintf(line3, LINE_3_FORMAT, str_humidity);

    char_t bigg[128] = {};

    sprintf(bigg,
            "%s at %s: [%s] - %s%s, [%s] - %s%s %s'\r\n",
            weather_screen_control[current_screen].p_name_str,
            str_cur_time,
            weather_screen_control[current_screen].p_temp_key_str,
            str_temp,
            weather_screen_control[current_screen].p_temp_str,
            weather_screen_control[current_screen].p_wind_key_str,
            str_wind_speed,
            weather_screen_control[current_screen].p_wind_str,
            str_humidity);
    // print_to_console(bigg);
}


static void inc_new_screen (int32_t * index)
{
    switch (current_weather_screen) {
        case PICTURE_INDEX_WEATHER_SHANGHAI:
            current_weather_screen = PICTURE_INDEX_WEATHER_KYOTO;
            *index = GI_WEATHER_KYOTO - GI_WEATHER_HONG_KONG;
            break;
        case PICTURE_INDEX_WEATHER_KYOTO:
            current_weather_screen = PICTURE_INDEX_WEATHER_NEW_YORK;
            *index = GI_WEATHER_NEW_YORK - GI_WEATHER_HONG_KONG;
            break;
        case PICTURE_INDEX_WEATHER_NEW_YORK:
            current_weather_screen = PICTURE_INDEX_WEATHER_SAN_FRAN;
            *index = GI_WEATHER_SAN_FRANCISCO - GI_WEATHER_HONG_KONG;
            break;
        case PICTURE_INDEX_WEATHER_SAN_FRAN:
            current_weather_screen = PICTURE_INDEX_WEATHER_SHANGHAI;
            *index = GI_WEATHER_SHANGHAI - GI_WEATHER_HONG_KONG;
            break;
        default:
            current_weather_screen = PICTURE_INDEX_WEATHER_SHANGHAI;
            *index = GI_WEATHER_SHANGHAI - GI_WEATHER_HONG_KONG;
            break;
    }

    reset_transition();
    reset_animation();
}

static uint32_t decide_icon(uint32_t region, char_t * str_cur_time, char * condition, bool_t snow, bool_t windy)
{
    (void)str_cur_time;

    uint32_t decision = GI_ICON_SUN;
    bool_t selected = false;

    /* Ensure region is valid */
    region = region % NUM_ICONS_AVAILABLE;

    if ((selected == false) && (strstr(condition, "drizzle") != NULL))
    {
        decision = GI_ICON_RAIN;
        selected = true;
    }

    if ((selected == false) && (strstr(condition, "rain possible") != NULL))
    {
        decision = GI_ICON_RAIN;
        selected = true;
    }

    if ((selected == false) && (strstr(str_condition, "rain") != NULL))
    {
        decision = GI_ICON_RAIN;
        selected = true;
    }

    if ((selected == false) && (snow == true))
    {
        decision = GI_ICON_SNOW;
        selected = true;
    }

    if ((selected == false) && (windy == true))
    {
        decision = GI_ICON_WIND;
        selected = true;
    }

    return decision;
}

volatile uint16_t v_icon_offset = 118;

void do_weather_screen(void)
{
    static uint16_t call_cnt = 0;

    st_gimp_image_t img;

    graphics_wait_vsync();
    graphics_start_frame();

    if (first_call) {
        reset_animation();
        first_call = false;
    }

    if (in_transition())
    {
        img.pixel_data = (guint *)lcd_get_pic_from_flash((uint8_t)current_weather_screen);

        d2_setblitsrc(d2_handle, img.pixel_data, LCD_HPIX, LCD_HPIX, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, LCD_HPIX, LCD_VPIX, 0, 0, LCD_HPIX << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);

        glcd_layer_change.layer.coordinate.x = FG_PANNEL_VERTICAL;
        glcd_layer_change.layer.coordinate.y = 0;

        glcd_layer_change.input = g_display0.p_cfg->input[1];
        R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);

        st_gimp_fg_icon_image_t simg;

        print_fg_font_22(static_x, static_y, clear_line);
        print_fg_font_22(static_x, static_y, line1);

        print_fg_font_22(static_x, static_y - 30, clear_line);
        print_fg_font_22(static_x, static_y - 30, line2);

        print_fg_font_22(static_x, static_y - 60, clear_line);
        print_fg_font_22(static_x, static_y - 60, line3);

        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash((uint8_t)(selected_icon + PICTURE_INDEX_WEATHER_RAIN));
        simg.bytes_per_pixel = 4;
        simg.height = 113;
        simg.width = 126;

        switch (selected_icon)
        {
            case GI_ICON_RAIN:
            {
                v_icon_offset = 90;
                break;
            }

            case GI_ICON_SNOW:
            {
                v_icon_offset = 90;
                break;
            }

            case GI_ICON_SUN:
            {
                v_icon_offset = 90;
                break;
            }

            case GI_ICON_WIND:
            {
                v_icon_offset = 96;
                break;
            }

            default:
                break;
        }

        icon_erase.bytes_per_pixel = 4;
        icon_erase.height = 113;
        icon_erase.width = 126;
        icon_erase.pixel_data = s_icon_erase_pixel;

        memset(icon_erase.pixel_data, 0, icon_erase.width * icon_erase.height * icon_erase.bytes_per_pixel);

        draw_with_alpha(&icon_erase, 350, 90);
        draw_with_alpha(&simg, 350, v_icon_offset); // From bottom left of layer to bottom left of image
    }

    call_cnt++;
    if (call_cnt == 200) {
        call_cnt = 0;
        inc_new_screen(&current_screen);
    }

    d2_flushframe(d2_handle);
    graphics_end_frame();
}
