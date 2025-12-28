/***********************************************************************************************************************
 * Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : menu_ns.c
 * Version      : .
 * Description  : The next steps screen display.
 *********************************************************************************************************************/

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

#include "bg_font_18_full.h"
#include "fg_font_22_full.h"

#include "gimp.h"
#include "lcd.h"
#include "key.h"
#include "currency_table.h"
#include "sensor.h"

// #define SHOW_DETECTION_BOXES (1)

#define CONNECTION_ABORT_CRTL (0x00)
#define MENU_EXIT_CRTL (0x20)

#define SLIDE_SHOW_RESET (60)
#define SLIDE_SHOW_RESET_WEATHER (150)

#define WEATHER_ICON_HOFFSET (308)
#define WEATHER_ICON_VOFFSET (48)

#define MODULE_NAME "\r\n%d. 交互式 AI, 连接性与人机接口演示\r\n\r\n"

#define SUB_OPTIONS "To explore various Interactive Connectivity & HMI Demos\r\n\r\n"                      \
                    "a) Please connect the MIPI Graphic and Camera Expansion Boards included in\r\n"       \
                    "   the CPKCOR-RA8D1 kit to the CPKEXP-RA8D1 board.\r\n"                               \
                    "b) Please connect the Ethernet cable included in the CPKEXP-RA8D1 kit to the EK-\r\n" \
                    "   RA8D1 board and your Internet Router or Switch.\r\n"                               \
                    "c) Reset the kit and follow the instructions.\r\n\r\n"

#define EP_INFO "(1)"

static char_t s_print_buffer[BUFFER_LINE_LENGTH] = {};
static display_runtime_cfg_t glcd_layer_change;

enum {
	DEMO_MODE_SPLASH_SCREEN = 0,
	DEMO_MODE_MAIN_SCREEN,
	DEMO_MODE_KIS_SCREEN,
	DEMO_MODE_LED_SCREEN,
	DEMO_MODE_CURRENCY_SCREEN,
	DEMO_MODE_WEATHER_SCREEN,
	DEMO_MODE_TIMEZONE_SCREEN,
	DEMO_MODE_AI_FACE_RECONITION,
	DEMO_MODE_AI_OBJECT_RECONITION,
	DEMO_MODE_MAX
};

#define SLIDESHOW_MODE (42)
#define SLIDESHOW_AUTO_MODE (48)
#define MAX_SCRRENS_FOR_DEMO (28)

extern void do_led_screen(void);

extern void do_kis_screen(void);
extern void do_face_reconition_screen(void);
extern void do_object_detection_screen(void);
extern void do_currency_screen(void);
extern void do_world_screen(void);
extern void do_weather_screen(void);

extern bool_t is_camera_mode(void);
extern bool_t is_ethernet_mode(void);
extern bool_t is_startup_mode_in_error(void);

extern uint8_t fb_foreground[1][DISPLAY_BUFFER_STRIDE_BYTES_INPUT1 * DISPLAY_VSIZE_INPUT1];

volatile uint8_t *live_bg_ptr = (uint8_t *)fb_background[1];
volatile uint8_t *upd_bg_ptr = (uint8_t *)fb_background[0];

extern void init_sdram(void);
extern void glcdc_init();
extern void init_ospi(void);
extern bool_t can_swap_demos(void);
extern void reset_transition(void);
extern bool_t in_transition(void);

extern void mipi_dsi_init(void);

#define NUM_WEATHER_IMAGE_ICONS (2)
#define NUM_VIEWER_IMAGES 16
#define REFRESH_RATE 8

/* RESOLUTION FROM CAMERA */
#define CAM_IMG_SIZE_X 640
#define CAM_IMG_SIZE_Y 480 /* Trim the Right Hand Edge hiding corruption */

#define HP_SIZE (70) /* Hit Box for ICONS */
#define HB_SIZE (45) /* Hit Box for Hamburger and CLose */

#if 0

// full screen
#define CAM_LAYER_SIZE_X 480 // 000 --> 480
#define CAM_LAYER_SIZE_Y 854 // 000 --> 854

#else

/* normal screen */
#define CAM_LAYER_SIZE_X 480 /* 000 --> LCD_VPIX */
#define CAM_LAYER_SIZE_Y 256 /* 000 --> 480 */

#define RGB_565_RED (0x1F << 11)
#define RGB_565_GREEN (0x3F << 5)
#define RGB_565_BLUE (0x1F << 0)
#define RGB_565_WHITE (0xFFFF)
#define RGB_565_GRAY (0xFFFF << 2)

#define act_hz 222
#define act_vz 480

uint16_t color[4 + 8] = {RGB_565_RED, RGB_565_GREEN, RGB_565_BLUE, RGB_565_WHITE, 0xe6d7, 0xcee7, 0xe5c0, 0xd1cf, 0x67FC, 0xCE7F, 0xFEA0, 0xC618};

#endif

bool_t show_bg = true;
uint32_t demo_screen = DEMO_MODE_SPLASH_SCREEN; // DAV/2D SPLASH SCREEN
uint32_t old_demo_screen = DEMO_MODE_SPLASH_SCREEN;
uint32_t slide_show = 7;              // LED

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t camera_init(bool_t use_test_mode);

static bool_t g_camera_initialised = false;
static uint32_t g_transition_frame = 2;

#define RESET_FLAG (false)
#define BYTES_PER_PIXEL (4)
#define DWT_DEM *(uint32_t *)0xE000EDFC

volatile bool g_vsync_flag, g_ulps_flag, g_irq_state, g_timer_overflow = RESET_FLAG;
uint16_t g_hz_size, g_vr_size;
uint32_t count;
uint32_t g_buffer_size, g_hstride;

void reset_transition(void)
{
    g_transition_frame = 3;
}

bool_t in_transition(void)
{
    while (can_swap_demos() == false)
    {
        vTaskDelay(1);
    }

    if (g_transition_frame > 0)
    {
        g_transition_frame--;

        return true;
    }
    else
    {
        return false;
    }
}

static bool_t menu_in_use = false;

extern bool_t ia_processing(void);

static void do_splash_screen(void)
{
    uint16_t *img = NULL;

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    if (in_transition())
    {
        d2_clear(d2_handle, 0x101010);

        img = lcd_get_pic_from_flash(PICTURE_INDEX_POWER_UP);
        d2_setblitsrc(d2_handle, img, LCD_HPIX, LCD_HPIX, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, LCD_HPIX, LCD_VPIX, 0, 0, LCD_HPIX << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);
    }

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}

static void do_main_screen(void)
{
    st_gimp_image_t img;

    graphics_wait_vsync();
    graphics_start_frame();

    if (in_transition()) {
        img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_MAIN_SCREEN);
        d2_setblitsrc(d2_handle, img.pixel_data, 480, 480, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, 480, LCD_VPIX, 0, 0, 480 << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);
    }

    d2_flushframe(d2_handle);
    graphics_end_frame();
}

extern void reenable_backlight_control(void);
extern void stop_led_demo(void);

#define KEY_TIMEOUT (60000000)

test_fn camview_display_menu(bool_t first_call)
{
    EventBits_t events;

    int16_t c = -1;
    uint32_t ss_timer = 50;

    sprintf(s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
    print_to_console(s_print_buffer);

    /* Select text foreground */
    sprintf(s_print_buffer, "%s", gp_white_fg);
    print_to_console(s_print_buffer);

    sprintf(s_print_buffer, MODULE_NAME, g_selected_menu);
    print_to_console(s_print_buffer);
    print_to_console("按空格键返回主界面\r\n");

    memset(fb_foreground, 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT1 * DISPLAY_VSIZE_INPUT1);

    /* Initialize D/AVE 2D driver */
    d2_handle = d2_opendevice(0);
    d2_inithw(d2_handle, 0);

    /* Clear both buffers */
    d2_framebuffer(d2_handle, fb_background, LCD_HPIX, LCD_STRIDE, LCD_VPIX * LCD_BUF_NUM, EP_SCREEN_MODE);
    d2_clear(d2_handle, 0x00000000);

    /* Set various D2 parameters */
    d2_setblendmode(d2_handle, d2_bm_alpha, d2_bm_one_minus_alpha);
    d2_setalphamode(d2_handle, d2_am_constant);
    d2_setalpha(d2_handle, 0xff);
    d2_setantialiasing(d2_handle, 1);
    d2_setlinecap(d2_handle, d2_lc_butt);
    d2_setlinejoin(d2_handle, d2_lj_none);

    if (is_camera_mode() == true)
    {
        /* true  = use live camera feed */
        /* false = use test_mode */
        if (false == g_camera_initialised) {
            g_camera_initialised = camera_init(true);
        }
    }

    if (is_startup_mode_in_error() == true)
    {
        uint32_t key_count = KEY_TIMEOUT;

        R_GPT_Stop(g_blinker.p_ctrl);

        print_to_console((void *)"\n\r检测不到相机，请检查相机排线连接");

        while (1)
        {
            if ((key_count--) == 0)
            {
                key_count = KEY_TIMEOUT * 2;
                print_to_console((void *)"\r检测不到相机，请检查相机排线连接");
            }
        }
    }

    if (first_call)
    {
        glcdc_init();
        mipi_dsi_init();
        vTaskDelay(100);
    }

    start_key_check();

    demo_screen = DEMO_MODE_SPLASH_SCREEN;
    reset_transition();
    menu_in_use = false;

    reenable_backlight_control();

    while (1) {
        switch (demo_screen) {
        case DEMO_MODE_SPLASH_SCREEN:
            do_splash_screen();
            if ((ss_timer--) == 0) {
                demo_screen = DEMO_MODE_MAIN_SCREEN;
                reset_transition();
            }
            break;
        case DEMO_MODE_MAIN_SCREEN:
            do_main_screen();
            break;
        case DEMO_MODE_KIS_SCREEN:
            do_kis_screen();
            break;
        case DEMO_MODE_LED_SCREEN:
            do_led_screen();
            break;
        case DEMO_MODE_WEATHER_SCREEN:
            do_weather_screen();
            break;
        case DEMO_MODE_AI_FACE_RECONITION:
            do_face_reconition_screen();
            break;
        case DEMO_MODE_AI_OBJECT_RECONITION:
            do_object_detection_screen();
            break;
        case DEMO_MODE_CURRENCY_SCREEN:
            do_currency_screen();
            break;
        case DEMO_MODE_TIMEZONE_SCREEN:
            do_world_screen();
            break;
        default:
            break;
        }

        /* This task delay- allows other threads (ie AI) to run in parallel with screen */
        vTaskDelay(CAMVIEW_MENU_DELAY);

        uint32_t next_demo_screen = demo_screen;
        if (demo_screen != next_demo_screen)
        {
            sprintf(s_print_buffer, "Showing Menu [%d]\r\n", demo_screen);
            print_to_console(s_print_buffer);

            reset_transition();
            demo_screen = next_demo_screen;
        }

        if (true == key_pressed())
        {
            c = get_detected_key();
            if ((MENU_EXIT_CRTL == c) || (CONNECTION_ABORT_CRTL == c))
            {
                dsi_layer_disable_backlight();
                memset(&fb_background[0][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
                memset(&fb_background[1][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);

                stop_led_demo();
                break;
            }
            start_key_check();
        }

        events = xEventGroupGetBits(g_event_group);
        if ((events & KEY_EVENT_01_PRESS) == KEY_EVENT_01_PRESS) {
            xEventGroupClearBits(g_event_group, KEY_EVENT_01_PRESS);
            if (demo_screen == DEMO_MODE_LED_SCREEN) {
                stop_led_demo();
            }
            demo_screen++;
            if (demo_screen == DEMO_MODE_MAX)
                demo_screen = DEMO_MODE_MAIN_SCREEN;

            reset_transition();

            glcd_layer_change.input = g_display0.p_cfg->input[1];
            uint8_t * buf_ptr = (uint8_t *) fb_foreground;
            display_input_cfg_t const * p_input = &g_display0.p_cfg->input[1];
            memset(buf_ptr, 0, p_input->hstride * p_input->vsize * 4);
            // printf("%s ==> Demo: %d\r\n", __FUNCTION__, demo_screen);
        } else if ((events & KEY_EVENT_02_PRESS) == KEY_EVENT_02_PRESS) {
            xEventGroupClearBits(g_event_group, KEY_EVENT_02_PRESS);
            if (demo_screen == DEMO_MODE_LED_SCREEN) {
                stop_led_demo();
            }
            demo_screen--;
            if (demo_screen == DEMO_MODE_SPLASH_SCREEN)
                demo_screen = DEMO_MODE_AI_OBJECT_RECONITION;

            reset_transition();

            glcd_layer_change.input = g_display0.p_cfg->input[1];
            uint8_t * buf_ptr = (uint8_t *) fb_foreground;
            display_input_cfg_t const * p_input = &g_display0.p_cfg->input[1];
            memset(buf_ptr, 0, p_input->hstride * p_input->vsize * 4);
            // printf("%s ==> Demo: %d\r\n", __FUNCTION__, demo_screen);
        }
    }

    return 0;
}

void menu_camview_test(void)
{
    uint16_t hz_size = g_display0_cfg.input[0].hsize;
    uint16_t vr_size = g_display0_cfg.input[0].vsize;

    uint16_t *p = (uint16_t *)&fb_background[0][0];

    for (uint32_t x = 0; x < vr_size / 2; x++)
    {
        for (uint32_t y = 0; y < (hz_size - 34) / 2; y++)
        {
            p[y + x * hz_size] = color[0];
        }
    }

    for (uint32_t x = 0; x < vr_size / 2; x++)
    {
        for (uint32_t y = (hz_size - 34) / 2; y < hz_size; y++)
        {
            p[y + x * hz_size] = color[1];
        }
    }
    for (uint32_t x = vr_size / 2; x < vr_size; x++)
    {
        for (uint32_t y = 0; y < act_hz / 2; y++)
        {
            p[y + x * hz_size] = color[2];
        }
    }
    for (uint32_t x = vr_size / 2; x < vr_size; x++)
    {
        for (uint32_t y = act_hz / 2; y < hz_size; y++)
        {
            p[y + x * hz_size] = color[3];
        }
    }

    R_GLCDC_BufferChange(&g_display0_ctrl, (uint8_t *)p, DISPLAY_FRAME_LAYER_1);
}
