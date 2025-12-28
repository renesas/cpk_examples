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

#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "gimp.h"

#include "currency_table.h"
#include "lcd.h"

static st_gimp_currency_image_t gimp_spacer_text BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(".sdram");

#define CURRENCY_HB_WIDTH_SIZE     (150)
#define CURRENCY_HB_HEIGHT_SIZE    (120)

extern bool_t   https_currency_update(void);
extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   in_transition(void);
extern void   reset_transition(void);

extern char_t * gp_currency_full_table[10];

typedef enum e_currency_indictor
{
    LED_SCREEN_RED,
    LED_SCREEN_GREEN,
    LED_SCREEN_BLUE,
} e_currency_indictor_t;

typedef struct st_currency_screen_indicator_def
{
    e_currency_indictor_t id;
    char_t              * p_name_str;
    char_t              * p_sname_str;
    st_lcd_point_def_t    center;
    bool_t                in_use;      // blink indicator position in radians
} st_currency_screen_indicator_def_t;

st_currency_screen_indicator_def_t currency_screen_control[] =
{
    {0, "Austrailian Dollar",    "AUD", {31,  280  }, false},
    {1, "British Pound Sterlin", "GBP", {191, 280  }, false},
    {2, "Canadian Dollar",       "CAD", {352, 280  }, false},
    {3, "Chinese Yen",           "CNY", {514, 280  }, false},
    {4, "European Union Euro",   "EUR", {677, 280  }, false},
    {5, "Hong Kong Dollar",      "HKD", {31,  154  }, false},
    {6, "Indian Rupee",          "INR", {191, 154  }, false},
    {7, "Jananese Yen",          "JPY", {352, 154  }, false},
    {8, "Singaphorean Dollar",   "SGD", {514, 154  }, false},
    {9, "United States Dollar",  "USD", {677, 154  }, false},
};

st_lcd_point_def_t refpoint     = {89, 26};
bool_t             s_show_reset = false;
static void draw_with_alpha(st_gimp_countries_image_t * object, uint16_t hoffset, uint16_t voffset);

void do_currency_screen(void);
void reset_selection(void);

static uint32_t progress_state = 0;

static int32_t first_chosen  = -1;
static int32_t second_chosen = -1;
static display_runtime_cfg_t glcd_layer_change;

void reset_selection (void)
{
    first_chosen  = -1;
    second_chosen = -1;

    progress_state = 0;
    s_show_reset   = false;

    /* clear screen */
    taskENTER_CRITICAL();
    display_input_cfg_t const * p_input = &g_display0.p_cfg->input[1]; // Layer 2
    uint8_t * buf_ptr = (uint8_t *) fb_foreground;

    /* ver basic clear screen */
    gimp_spacer_text.height          = 290;
    gimp_spacer_text.width           = 70;
    gimp_spacer_text.bytes_per_pixel = 4;
    memset(gimp_spacer_text.pixel_data, 0, ((50 * 405 * 4) + 1));

    // Set to off screen;

    glcd_layer_change.layer.coordinate.x = 80;
    glcd_layer_change.layer.coordinate.y = 900;
    glcd_layer_change.input              = g_display0.p_cfg->input[1];

    memset(buf_ptr, 0, p_input->hstride * p_input->vsize * 4);

    taskEXIT_CRITICAL();

    (void) R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);
}

static void draw_with_alpha (st_gimp_countries_image_t * object, uint16_t hoffset, uint16_t voffset)
{
    /* GLCDC Graphics Layer 1 size must be 182 x 200 */
    uint8_t * buf_ptr = (uint8_t *) fb_foreground;
    display_input_cfg_t const * p_input = &g_display0.p_cfg->input[1]; // Layer 2

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
            buf_ptr[3] = object->pixel_data[v_offset + h + 3]; // A
            buf_ptr   += 4;
        }

        buf_ptr += ((p_input->hstride - p_input->hsize) * 4);

        /* align image size to be drawn with the surface size */
        buf_ptr += ((p_input->hsize - object->width) * bpp);
    }
}

void do_currency_screen(void)
{
    st_gimp_image_t img;
    st_gimp_countries_image_t simg;
    int i;
    uint8_t num;

    uint16_t xoff = 50;
    uint8_t num_pic_offset = PICTURE_INDEX_NUM_ZERO;
    char_t result_str[32] = "";

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    if (in_transition()) {
        img.pixel_data = (guint *)lcd_get_pic_from_flash(PICTURE_INDEX_CURRENCY);

        d2_setblitsrc(d2_handle, img.pixel_data, 480, 480, LCD_VPIX, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle, 480, LCD_VPIX, 0, 0, 480 << 4, LCD_VPIX << 4, 0, 0, d2_tm_filter);
    }
    else {
        https_currency_update();

        glcd_layer_change.layer.coordinate.x = 0;
        glcd_layer_change.layer.coordinate.y = 0;
        glcd_layer_change.input = g_display0.p_cfg->input[1];
        R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_2);

        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(PICTURE_INDEX_NUM_ONE);
        simg.height = 25;
        simg.width = 50;
        simg.bytes_per_pixel = 4;
        draw_with_alpha(&simg, xoff, 250);

        xoff += simg.height;
        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(PICTURE_INDEX_CURRENCY_USD);
        simg.height = 99;
        draw_with_alpha(&simg, xoff, 250);

        xoff += simg.height;
        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(PICTURE_INDEX_EQUALS);
        simg.height = 35;
        draw_with_alpha(&simg, xoff, 250);

        currency_get_rate(gp_currency_full_table[9], currency_screen_control[3].p_sname_str, result_str);
        for (i = 0; i < 6; i++) {
            if (result_str[i] == '.') {
                xoff += simg.height;
                simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(PICTURE_INDEX_POINT);
                simg.height = 8;
                draw_with_alpha(&simg, xoff, 250);
            }
            else {
                num = result_str[i] - '0';
                if ((num >= 0) && (num <= 9)) {
                    xoff += simg.height;
                    simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(num_pic_offset + num);
                    simg.height = 25;
                    draw_with_alpha(&simg, xoff, 250);
                }
            }
        }

        xoff += simg.height;
        simg.pixel_data = (guint8 *)lcd_get_pic_from_flash(PICTURE_INDEX_CURRENCY_CNY);
        simg.height = 99;
        draw_with_alpha(&simg, xoff, 250);
    }

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}
