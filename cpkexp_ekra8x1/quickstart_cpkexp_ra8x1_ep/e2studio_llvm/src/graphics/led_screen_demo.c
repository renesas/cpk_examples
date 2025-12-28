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

#include "hal_data.h"
#include "dsi_layer.h"

#include "camera_layer.h"
#include "graphics\graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "gimp.h"
#include "lcd.h"

#define MAX_SPEED_REFRESH (1)
#define REFRESH_RATE (8)
#define RADIANS_CONSTANT (0.0174533)               /* Convert degrees into radians */
#define BRIGHTNESS_SCALE_FACTOR_CONSTANT (0.38759) // Scale factor to convert brightness angel from 0 -100
#define BRIGHTNESS_SCALE_FACTOR_CONSTANT (0.38759) /* Scale factor to convert brightness angel from 0 -100 */

#define USE_SDRAM_IMAGE_SOURCES (1)

extern void reset_transition(void);
extern bool_t in_transition(void);
extern uint32_t get_image_data(st_image_data_t ref);

#define USE_BACKGROUND_IMAGE

/* The following defines configure the 6 detection boxes for this screen */
#define BLINKING_INDEX (0)
#define BRIGHTNESS_INDEX (1)

#define BL_WIDTH_SIZE (230)
#define BL_HEIGHT_SIZE (200)

#define HB_WIDTH_SIZE (240)
#define HB_HEIGHT_SIZE (75)

// LOSE LOG SCALE TO SET BLINKING RATE
#define BLINK_RATE_10 60000000 /* SLOWEST */
#define BLINK_RATE_20 57300000
#define BLINK_RATE_30 54600000
#define BLINK_RATE_40 51900000
#define BLINK_RATE_50 48120000
#define BLINK_RATE_60 43800000
#define BLINK_RATE_70 38400000
#define BLINK_RATE_80 31920000
#define BLINK_RATE_90 22200000
#define BLINK_RATE_100 6000000 /* FASTEST */

static uint32_t s_blinking_rate_map[] = {
    BLINK_RATE_10,
    BLINK_RATE_20,
    BLINK_RATE_30,
    BLINK_RATE_40,
    BLINK_RATE_50,
    BLINK_RATE_60,
    BLINK_RATE_70,
    BLINK_RATE_80,
    BLINK_RATE_90,
    BLINK_RATE_100,
    BLINK_RATE_100
};

static double xf = 420.0;
static double yf = 420.0;

static double xef = 0.0;
static double yef = 0.0;

static double xcos = 15.0;
static double ysin = 15.0;

static double angle_radians = 15.0;

static double inner_length = 40.0;
static double outer_length = 100.0;

typedef enum e_led_screen_indictor
{
    LED_SCREEN_BLUE,
} led_screen_indictor_t;

typedef struct
{
    led_screen_indictor_t id;
    st_gimp_led_image_t *simg; // sub_image_location

    double blink_center_x;            // blink indicator needle ceneter point x
    double blink_center_y;            // blink indicator needle ceneter point y
    d2_width blink_dest_pos_x;        // blink image destination position x
    d2_width blink_dest_pos_y;        // blink image destination position y
    d2_width blink_position;          // blink indicator position in radians
    d2_width blink_as_percent;        // Blink indicator as % for driving pins
    d2_width blink_as_log_index;      // Blink indicator as scale 0 -> 10 for driving pins
    d2_width brightness_dest_pos_x;   // brightness image destination position x
    d2_width brightness_dest_pos_y;   // brightness image destination position y
    d2_width brightness_position;     // brightness indicator position in screen pixels
    d2_width brightness_as_percent;   // brightness indicator as % for driving pins
    d2_width brightness_as_log_index; // Blink indicator as scale 0 -> 10 for driving pins
} st_led_screen_indicator_def_t;

static st_led_screen_indicator_def_t led_control[] = {
    {LED_SCREEN_BLUE, NULL, 185.0, 110.0, 65, 148, 315, 0, 0, 260, 132, 0, 1, 0},
};

typedef struct st_led_loc_def
{
    d2_point x_vert;
    d2_point y_hor;
} st_led_loc_def_t;

typedef struct st_led_hit_group_def
{
    st_led_loc_def_t red;
    st_led_loc_def_t green;
    st_led_loc_def_t blue;
} st_led_hit_group_def_t;

void do_led_screen(void);

static void draw_rate_indicaor(led_screen_indictor_t active);
static void draw_brightness_indicaor(led_screen_indictor_t active);

static d2_point brightness_offset = 0;

/**********************************************************************************************************************
 * Function Name: draw_rate_indicaor
 * Description  : .
 * Argument     : active
 * Return Value : .
 *********************************************************************************************************************/
static void draw_rate_indicaor(led_screen_indictor_t active)
{
    // Coloured arc led frequency position
    d2_setcolor(d2_handle, 0, 0xf7f7f7);

    xf = led_control[active].blink_center_x;
    yf = led_control[active].blink_center_y;

    angle_radians = (led_control[active].blink_position * RADIANS_CONSTANT);

    /* Calculate line angle */
    xcos = cos(angle_radians);
    ysin = sin(angle_radians);

    /* Calculate Line co-ordinates */
    xef = xf + (xcos * inner_length);
    yef = yf + (ysin * inner_length);

    /* Line form outer edge  to inner edge mode */
    xf = xf + (xcos * outer_length);
    yf = yf + (ysin * outer_length);

    d2_renderline(d2_handle, (d2_point)((d2_point)yf << 4), (d2_point)((d2_point)xf << 4), (d2_point)((d2_point)yef << 4), (d2_point)((d2_point)xef << 4), 7 << 4, 0);
}

/**********************************************************************************************************************
 * Function Name: draw_brightness_indicaor
 * Description  : .
 * Argument     : active
 * Return Value : .
 *********************************************************************************************************************/
static void draw_brightness_indicaor(led_screen_indictor_t active)
{
    /* Coloured arc led brightness position */
    d2_setcolor(d2_handle, 0, 0xf7f7f7);

    brightness_offset = led_control[active].brightness_position;

    d2_point start_x = led_control[active].brightness_dest_pos_x + brightness_offset;
    d2_point start_y = led_control[active].brightness_dest_pos_y + 0;

    d2_point pos_x = led_control[active].brightness_dest_pos_x + brightness_offset;
    d2_point pos_y = led_control[active].brightness_dest_pos_y + 40;

    d2_renderline(d2_handle, (d2_point)(start_x << 4), (d2_point)(start_y << 4), (d2_point)(pos_x << 4), (d2_point)(pos_y << 4), 7 << 4, 0);
}

#define GPT_EXAMPLE_MSEC_PER_SEC (1000)
#define GPT_EXAMPLE_DESIRED_PERIOD_MSEC (20)

double fw1 = 0.4166;
double fw2 = 0.7291;

led_screen_indictor_t active = LED_SCREEN_BLUE;
uint32_t active_buffer = 0;
bool_t led_screen_open = false;

void start_led_demo(void);

/**********************************************************************************************************************
 * Function Name: start_led_demo
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void start_led_demo(void)
{
    R_GPT_Stop(g_pwm_pins[0].p_timer->p_ctrl);
    R_GPT_Stop(g_blinker.p_ctrl);

    R_GPT_Open(g_led_scr_blue_blink.p_ctrl, g_led_scr_blue_blink.p_cfg);

    R_GPT_PeriodSet(g_led_scr_blue_blink.p_ctrl, s_blinking_rate_map[(led_control[LED_SCREEN_BLUE].blink_as_percent / 10)]);

    R_GPT_Start(g_pwm_pins[1].p_timer->p_ctrl);

    R_GPT_Start(g_led_scr_blue_blink.p_ctrl);

    led_screen_open = true;
}

void stop_led_demo(void);

/**********************************************************************************************************************
 * Function Name: stop_led_demo
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void stop_led_demo(void)
{
    led_screen_open = false;

    R_GPT_Stop(g_led_scr_blue_blink.p_ctrl);

    R_GPT_Stop(g_pwm_pins[1].p_timer->p_ctrl);

    R_GPT_Reset(g_led_scr_blue_blink.p_ctrl);

    R_GPT_Reset(g_pwm_pins[0].p_timer->p_ctrl);
    R_GPT_Reset(g_pwm_pins[1].p_timer->p_ctrl);

    R_GPT_Start(g_blinker.p_ctrl);

    R_BSP_PinAccessEnable();

    R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_00,
                    ((uint32_t)IOPORT_CFG_PERIPHERAL_PIN | (uint32_t)IOPORT_PERIPHERAL_GPT1));

    R_BSP_PinAccessDisable();
}

/**********************************************************************************************************************
 * Function Name: do_led_screen
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void do_led_screen(void)
{
    static uint8_t bri_direction = 0;
    static uint8_t blink_direction = 0;
    static uint16_t call_cnt = 0;
    static double pos_bri;

    st_gimp_image_t img;

    if (led_screen_open == false) {
        start_led_demo();
        pos_bri = led_control[LED_SCREEN_BLUE].brightness_position;
    }

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_LED_SCREEN);
    d2_setblitsrc(d2_handle, img.pixel_data, 480, 480, LCD_VPIX, EP_SCREEN_MODE);
    d2_blitcopy(d2_handle, 480, LCD_VPIX, 0, 0, 480 << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);

    if (!in_transition()) {
        draw_rate_indicaor(LED_SCREEN_BLUE);
        draw_brightness_indicaor(LED_SCREEN_BLUE);

        call_cnt++;
        if ((call_cnt % 100) == 0) {
            if (blink_direction) {
                led_control[LED_SCREEN_BLUE].blink_position += 27;
                led_control[LED_SCREEN_BLUE].blink_as_percent -= 10;
                if (led_control[LED_SCREEN_BLUE].blink_position >= 315) {
                    blink_direction = 0;
                }
            }
            else {
                led_control[LED_SCREEN_BLUE].blink_position -= 27;
                led_control[LED_SCREEN_BLUE].blink_as_percent += 10;
                if (led_control[LED_SCREEN_BLUE].blink_position <= 45) {
                    blink_direction = 1;
                }
            }
            stop_led_demo();
            start_led_demo();
        }

        if ((call_cnt % 2) == 0) {
            if (bri_direction) {
                pos_bri += 1.8;
                led_control[LED_SCREEN_BLUE].brightness_position = (d2_width)pos_bri;
                if (pos_bri >= 180) {
                    bri_direction = 0;
                }
                led_control[LED_SCREEN_BLUE].brightness_as_percent++;
            }
            else {
                pos_bri -= 1.8;
                led_control[LED_SCREEN_BLUE].brightness_position = (d2_width)pos_bri;
                if (pos_bri <= 0) {
                    bri_direction = 1;
                }
                led_control[LED_SCREEN_BLUE].brightness_as_percent--;
            }
        }

        if (call_cnt == 200) {
            call_cnt = 0;
        }
    }

    /* Reset alpha in case it was changed above */
    d2_setalpha(d2_handle, 0xFF);

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}

static volatile uint32_t s_blueled_flashing = OFF;
static volatile d2_width s_blueled_intense = 0;
static volatile d2_width s_blueled_duty = 99;

/**********************************************************************************************************************
 * Function Name: led_scr_blue_blink_cb
 * Description  : .
 * Argument     : p_args
 * Return Value : .
 *********************************************************************************************************************/
void led_scr_blue_blink_cb(timer_callback_args_t *p_args)
{
    /* Void the unused params */
    FSP_PARAMETER_NOT_USED(p_args);

    if (OFF == s_blueled_flashing)
    {
        s_blueled_flashing = ON;
    }
    else
    {
        s_blueled_flashing = OFF;
    }
}

/**********************************************************************************************************************
 * Function Name: led_scr_blue_bright_cb
 * Description  : .
 * Argument     : p_args
 * Return Value : .
 *********************************************************************************************************************/
void led_scr_blue_bright_cb(timer_callback_args_t *p_args)
{
    /* Void the unused params */
    FSP_PARAMETER_NOT_USED(p_args);

    switch (s_blueled_flashing)
    {
    case ON:
    {
        if ((s_blueled_intense++) < s_blueled_duty)
        {
            TURN_BLUE_ON
        }
        else
        {
            TURN_BLUE_OFF
        }

        if (s_blueled_intense >= 100)
        {
            s_blueled_intense = 0;
            s_blueled_duty = led_control[LED_SCREEN_BLUE].brightness_as_percent;
        }

        break;
    }

    default:
    {
        TURN_BLUE_OFF
        s_blueled_intense = 0;
        s_blueled_duty = led_control[LED_SCREEN_BLUE].brightness_as_percent;
    }
    }
}
