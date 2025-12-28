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

#include "camera_layer.h"
#include "common_utils.h"
#include "common_init.h"
#include "menu_camview.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "mipi_display/dsi_layer.h"

#include "camera/camera_layer.h"
#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "gimp.h"

#include "font_ai_face_digit.h"
#include "bg_font_18_full.h"
#include "lcd.h"

/* RESOLUTION FROM CAMERA */
#define CAM_IMG_SIZE_X          320
#define CAM_IMG_SIZE_Y          240    /* Trim the Right Hand Edge hiding corruption */

/* normal screen */
#define CAM_LAYER_SIZE_X        480    /* 000 --> LCD_VPIX */
#define CAM_LAYER_SIZE_Y        256    /* 000 --> 480 */

/* AI model resolution */
#define DET_MODEL_IMG_SIZE_X    192
#define DET_MODEL_IMG_SIZE_Y    192

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   in_transition(void);
extern bool_t   is_camera_mode(void);

extern st_ai_detection_point_t g_ai_detection[5];

uint64_t face_detection_inference_time;

void do_face_reconition_screen(void);

static volatile d2_point xs_pos = 42;
static volatile d2_point xe_pos = 401;

static volatile d2_point ys_pos = 247;
static volatile d2_point ye_pos = 605;

static volatile d2_point x_pos = 42;
static volatile d2_point y_pos = 247;

static uint8_t gimp_spacer_text[(50 * 30 * 4) + 1] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(".sdram");
static display_runtime_cfg_t glcd_layer_change;

static uint32_t previous_face_count = 99;
static uint32_t frame_counter       = 0;

/**********************************************************************************************************************
 * Function Name: do_face_reconition_screen
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void do_face_reconition_screen (void)
{
    EventBits_t event;
    st_gimp_image_t img;

    char str_frame_cnt[8] = {0};

    frame_counter++;

    /* Initialise camera edge */
    xs_pos = 42;
    xe_pos = 401;

    ys_pos = 247;
    ye_pos = 605;

    x_pos = 42;
    y_pos = 247;

    /* suspend AI operation */
    do_classification = false;

    if (is_camera_mode() == true) {
        bsp_camera_capture_image();

        /* resume AI operation */
        do_classification = false;
        do_detection = true;
    }

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    /* Use background image */
    if (in_transition()) {
        img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_FACE_DETECT);
        d2_setblitsrc(d2_handle, img.pixel_data, LCD_PITCH, LCD_WIDTH, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, LCD_WIDTH, LCD_VPIX, 0, 0, (d2_width)((LCD_WIDTH) << 4), (d2_width)((LCD_HEIGHT) << 4), 0, 0, d2_tm_filter);

        /* show model information */
        print_bg_font_18(d2_handle, 50, 170, "--- ms ");

        if (is_camera_mode() == true) {
            /* Move graphics on-screen */
            glcd_layer_change.layer.coordinate.x = 186;
            glcd_layer_change.layer.coordinate.y = 652; // need to center horizontical;

            glcd_layer_change.input = g_display0.p_cfg->input[1];
            R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);
        }

        memset(gimp_spacer_text, 0, ((50 * 30 * 4) + 1));

        previous_face_count = 99;

    	d2_flushframe(d2_handle);
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
            print_bg_font_18(d2_handle, 210, 230, "5. Launch Face Detection");
        }
        else {
            d2_setblitsrc(d2_handle, bsp_camera_capture_image(), BSP_CAM_WIDTH, BSP_CAM_WIDTH, BSP_CAM_HEIGHT, d2_mode_rgb565);
            d2_blitcopy(d2_handle, CAM_IMG_SIZE_Y, CAM_IMG_SIZE_Y, 0, 0, CAM_LAYER_SIZE_Y << 4, CAM_LAYER_SIZE_Y << 4, 220 << 4, 0, d2_tm_filter);
            event = xEventGroupGetBits(g_event_group);
            if (event & FACE_DETECTION_DONE)
            {
                uint32_t face_count = 0;
                d2_point x_off      = 220;
                d2_point y_off      = 40;
                xEventGroupClearBits(g_event_group, FACE_DETECTION_DONE);

                for (int_t i = 0; i < 5; i++)
                {
                    d2_point x = g_ai_detection[i].m_x;
                    d2_point y = g_ai_detection[i].m_y;
                    d2_point w = g_ai_detection[i].m_w;
                    d2_point h = g_ai_detection[i].m_h;

                    if (h != 0) {
                        face_count++;

                        d2_point left_top_x = 480 - x;
                        d2_point left_top_y = y + y_off;
                        d2_point right_bottom_x = left_top_x + w;
                        d2_point right_bottom_y = left_top_y + h;

                        if (left_top_x <= x_off) {
                            left_top_x = x_off + 1;
                        }
                        if (left_top_y <=  y_off) {
                            left_top_y = y_off + 1;
                        }
                        if (right_bottom_x >= 480) {
                            right_bottom_x = 479;
                        }
                        if (right_bottom_y >= 256) {
                            right_bottom_y = 255;
                        }

                        d2_setcolor(d2_handle, 0, 0xFF0000);
                        d2_renderline(d2_handle,
                                     (d2_point)(left_top_x << 4),
                                     (d2_point)(left_top_y << 4),
                                     (d2_point)(left_top_x << 4),
                                     (d2_point)(right_bottom_y << 4),
                                     (d2_point)(2 << 4),
                                     0);
                        d2_renderline(d2_handle,
                                     (d2_point)(left_top_x << 4),
                                     (d2_point)(left_top_y << 4),
                                     (d2_point)(right_bottom_x << 4),
                                     (d2_point)(left_top_y << 4),
                                     (d2_point)(2 << 4),
                                     0);
                        d2_renderline(d2_handle,
                                     (d2_point)(right_bottom_x << 4),
                                     (d2_point)(right_bottom_y << 4),
                                     (d2_point)(left_top_x << 4),
                                     (d2_point)(right_bottom_y << 4),
                                     (d2_point)(2 << 4),
                                     0);
                        d2_renderline(d2_handle,
                                     (d2_point)(right_bottom_x << 4),
                                     (d2_point)(right_bottom_y << 4),
                                     (d2_point)(right_bottom_x << 4),
                                     (d2_point)(left_top_y << 4),
                                     (d2_point)(2 << 4),
                                     0);
                    }

                    memset(&g_ai_detection[i], 0, sizeof(g_ai_detection[i]));
                }

                print_bg_font_18(d2_handle, 50, 220, "   ");
                sprintf(str_frame_cnt, "%2d", face_count);
                print_bg_font_18(d2_handle, 50, 220, str_frame_cnt);

                uint32_t time = (uint32_t) (face_detection_inference_time / 1000); // ms

                char_t time_str[] = {'0', '0', '0', ' ', 'm', 's', ' ', ' ', '\0'};
                time_str[0] += (char_t) (time / 100);
                time_str[1] += (char_t) ((time / 10) % 10);
                time_str[2] += (char_t) (time % 10);

                print_bg_font_18(d2_handle, 50, 170, time_str);
            }
        }
    }

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}
