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
#include "portable.h"

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

#include "gimp.h"

#include "font_ai_face_digit.h"
#include "bg_font_18_full.h"
#include "fg_font_22_full.h"
#include "lcd.h"

/* RESOLUTION FROM CAMERA */
#define CAM_IMG_SIZE_X      320
#define CAM_IMG_SIZE_Y      240        /* Trim the Right Hand Edge hiding corruption */

/* normal screen */
#define CAM_LAYER_SIZE_X    476        /* 000 --> LCD_VPIX */
#define CAM_LAYER_SIZE_Y    360        /* 000 --> 480 */

#define MAX_TEXT_UPDATE     3
#define MAX_STR_LEN         15
#define MAX_LOCAL_STR_LEN   (MAX_STR_LEN + 3)

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   in_transition(void);

extern st_ai_classification_point_t g_ai_classification[5];
extern const char ** getLabelPtr();
extern uint32_t      get_image_data(st_image_data_t ref);
extern bool_t        is_camera_mode(void);

static char local_str[5][MAX_LOCAL_STR_LEN] = {0};
static char local_prob[5][8] = {0};
static display_runtime_cfg_t glcd_layer_change;

uint64_t image_classification_inference_time;
void do_object_detection_screen(void);
void process_str(const char * input, char * output, int max_len);

/**********************************************************************************************************************
 * Function Name: process_str
 * Description  : .
 * Arguments    : input
 *              : output
 *              : max_len
 * Return Value : .
 *********************************************************************************************************************/
void process_str (const char * input, char * output, int max_len)
{
    int i;
    for (i = 0; (input[i] != '\0') && (i < (max_len - 1)); i++)
    {
        if (input[i] == ',')
        {
            break;
        }

        output[i] = input[i];
    }

    for ( ; i < (max_len - 1); i++)
    {
        output[i] = ' ';
    }

    output[max_len - 1] = '\0';
}

/**********************************************************************************************************************
 * End of function process_str
 *********************************************************************************************************************/
uint32_t refresh_fg = 0;

uint32_t tm = 0;

static volatile d2_point xs_pos = 42;
static volatile d2_point xe_pos = 401;

static volatile d2_point ys_pos = 247;
static volatile d2_point ye_pos = 605;

static volatile d2_point x_pos = 42;
static volatile d2_point y_pos = 247;

/**********************************************************************************************************************
 * Function Name: do_object_detection_screen
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void do_object_detection_screen (void)
{
    int i;
    st_gimp_image_t img;

    /* Initialise camera edge */
    xs_pos = 42;
    xe_pos = 401;

    ys_pos = 247;
    ye_pos = 605;

    x_pos = 42;
    y_pos = 247;

    /* suspend AI operation */
    do_detection = false;
    vTaskDelay(0);

    if (is_camera_mode() == true)
    {
        bsp_camera_capture_image();

        /* resume AI operation */
        do_detection = false;
        vTaskDelay(0);
        do_classification = true;
        vTaskDelay(0);
    }

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    if (in_transition())
    {
        img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_IMAGE_CLASS);
        d2_setblitsrc(d2_handle, img.pixel_data, 480, 480, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, 480, LCD_VPIX, 0, 0, 480 << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);

        if (is_camera_mode() == true)
        {
            // Move graphics on-screen
            glcd_layer_change.layer.coordinate.x = 76;
            glcd_layer_change.layer.coordinate.y = 520; // need to center horizontical;

            glcd_layer_change.input = g_display0.p_cfg->input[1];
            R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);
        }
    }
    else {
        if (is_camera_mode() == false) {
            print_bg_font_18(d2_handle, 210, 50, "PROBLEM CONNECTING TO");
            print_bg_font_18(d2_handle, 210, 70, "THE CAMERA");
            print_bg_font_18(d2_handle, 210, 90, "1.  Power down the kit");
            print_bg_font_18(d2_handle, 210, 110, "2. Ensure that the Camera");
            print_bg_font_18(d2_handle, 210, 130, "   Expansion Board is securely");
            print_bg_font_18(d2_handle, 210, 150, "   and correctly mounted on");
            print_bg_font_18(d2_handle, 210, 170, "   the Camera Expansion Port");
            print_bg_font_18(d2_handle, 210, 190, "3. Check the jumper cap");
            print_bg_font_18(d2_handle, 210, 210, "4. Power up the kit");
            print_bg_font_18(d2_handle, 210, 230, "5. Launch Image Detection");
        }
        else {
            d2_setblitsrc(d2_handle, bsp_camera_capture_image(), BSP_CAM_WIDTH, BSP_CAM_WIDTH, BSP_CAM_HEIGHT, d2_mode_rgb565);
            d2_blitcopy(d2_handle, CAM_IMG_SIZE_Y, CAM_IMG_SIZE_Y, 0, 0, 256 << 4, 256 << 4, 220 << 4, 0, d2_tm_filter);
            d2_setcolor(d2_handle, 0, 0x000000);

            const char ** labels = getLabelPtr(); // stored in ai_apps/img_class/Labels.c
            d2_point xpos = 10;
            d2_point ypos = 140;

            if (refresh_fg == 0) {
                refresh_fg = MAX_TEXT_UPDATE;
                for (i = 0; i < 5; i++) {
                    char processed_str[MAX_STR_LEN] = {0};

                    process_str(labels[g_ai_classification[i].category], processed_str, MAX_STR_LEN);
                    sprintf(local_str[i], "%s", processed_str);
                    memset(&local_str[i][MAX_STR_LEN - 1], ' ', MAX_LOCAL_STR_LEN - MAX_STR_LEN);
                    local_str[i][MAX_LOCAL_STR_LEN - 1] = '\0';

                    sprintf(local_prob[i], "%02d%%  ", (size_t) (g_ai_classification[i].prob * 100.0));
                    local_prob[i][5] = '\0';
                }
            }

            for (i = 0; i < 5; i++) {
                print_bg_font_18(d2_handle, xpos, ypos, (char *) local_prob[i]);
                print_bg_font_18(d2_handle, xpos + 60, ypos, (char *) local_str[i]);
                ypos += 20;
            }

            refresh_fg--;
        }
    }

    /* Wait for q frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}

/**********************************************************************************************************************
 * End of function do_object_detection_screen
 *********************************************************************************************************************/
