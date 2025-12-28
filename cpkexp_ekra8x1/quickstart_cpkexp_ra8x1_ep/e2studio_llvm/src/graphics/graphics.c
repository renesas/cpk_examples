/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : graphics.c
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/

#include <math.h>
#include "graphics.h"
#include "hal_data.h"
#include "dave_driver.h"

// #include "cachel1_armv7.h"

#include "common_utils.h"
#include "common_init.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#define LCD_BITS_PER_PIXEL        (32)
#define LCD_XSTRIDE_PHYS          (((DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0 * LCD_BITS_PER_PIXEL + 0x1FF) & 0xFFFFFE00) / LCD_BITS_PER_PIXEL)
#define LCD_XSIZE_PHYS            (DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0)
#define LCD_YSIZE_PHYS            (DISPLAY_VSIZE_INPUT0)
#define LCD_NUM_FRAMEBUFFERS      (2)

/******************************************************************************
 * Static global variables
 *****************************************************************************/
uint8_t drw_buf = 0;
static volatile uint32_t g_vsync_flag = 0;
static d2_renderbuffer * renderbuffer = NULL;

/******************************************************************************
 * Extern global variables
 *****************************************************************************/
extern d2_device * d2_handle;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

display_runtime_cfg_t glcd_layer_change;

bool         update_frame = false;
static float precision_x  = 0;
static float precision_y  = 0;

void glcdc_vsync_isr(display_callback_args_t * p_args);
void glcdc_init();

static unsigned char fb_rot[LCD_WIDTH * LCD_HEIGHT * 2 + 64] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(".sdram");

static void rotate_rgb565_90(const uint16_t *src, uint16_t *dst, int src_w, int src_h)
{
	// 顺时针旋转 90 度
	for (int y = 0; y < src_h; ++y) {
        	for (int x = 0; x < src_w; ++x) {
            		int new_x = src_h - 1 - y;
            		int new_y = x;
            		dst[new_y * src_h + new_x] = src[y * src_w + x];
        	}
    	}
}

/**********************************************************************************************************************
 * Function Name: glcdc_init
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void glcdc_init ()
{
    /* Seed rand - rand is used by this demo for image location calculations */
    uint32_t data;

    display_input_cfg_t const  * p_input  = &g_display0.p_cfg->input[0];
    display_output_cfg_t const * p_output = &g_display0.p_cfg->output;

    memset(&fb_background[0][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
    memset(&fb_background[1][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);

    /* copy the data to runtime - for GLCDC layer change */
    glcd_layer_change.input = g_display0.p_cfg->input[0];
    glcd_layer_change.layer = g_display0.p_cfg->layer[0];

    /* Centre image */
    precision_x = (int16_t) (p_output->htiming.display_cyc - p_input->hsize) / 2;
    precision_y = (int16_t) (p_output->vtiming.display_cyc - p_input->vsize) / 2;
    glcd_layer_change.layer.coordinate.x = (int16_t) precision_x;
    glcd_layer_change.layer.coordinate.y = (int16_t) precision_y;

    (void)R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_1);
}

/**********************************************************************************************************************
 * End of function glcdc_init
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: glcdc_update_layer_position
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void glcdc_update_layer_position ()
{
}

/**********************************************************************************************************************
 * End of function glcdc_update_layer_position
 *********************************************************************************************************************/

/* Draw 15 frames before enabling backlight to prevent flash of white light on screen */
static uint8_t g_draw_count = 0;
#define DIM_FRAME_COUNT_LIMIT    (15)

/**********************************************************************************************************************
 * Function Name: reenable_backlight_control
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void reenable_backlight_control (void)
{
    g_draw_count = 0;
}

/**********************************************************************************************************************
 * End of function reenable_backlight_control
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: glcdc_vsync_isr
 * Description  : .
 * Argument     : p_args
 * Return Value : .
 *********************************************************************************************************************/
void glcdc_vsync_isr (display_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    switch (p_args->event)
    {
        case DISPLAY_EVENT_GR1_UNDERFLOW:
        {
            break;
        }

        case DISPLAY_EVENT_GR2_UNDERFLOW:
        {
            break;
        }

        default:
    }

    if (g_draw_count < DIM_FRAME_COUNT_LIMIT)
    {
        g_draw_count++;
        if (g_draw_count == 15)
        {
            dsi_layer_enable_backlight();
        }
    }

    update_frame = true;
    g_vsync_flag = 1;
}

/**********************************************************************************************************************
 * End of function glcdc_vsync_isr
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Simple function to wait for vertical sync semaphore
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_wait_vsync
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void graphics_wait_vsync ()
{
    g_vsync_flag = 0;
    while (!g_vsync_flag)
    {
        ;
    }

    g_vsync_flag = 0;
}

/**********************************************************************************************************************
 * End of function graphics_wait_vsync
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Initialize D/AVE 2D driver and graphics LCD controller
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_init
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void graphics_init (void)
{
}

/**********************************************************************************************************************
 * End of function graphics_init
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Get address of the buffer to use when setting the D2 framebuffer
 *
 * NOTE: The returned address is technically a pointer to the currently displaying buffer.  By the time the DRW engine
 * starts drawing to it it will no longer be the active frame.
 *
 * @retval     void *     Draw buffer pointer to use with d2_framebuffer
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_get_draw_buffer
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void * graphics_get_draw_buffer ()
{
    return &(fb_background[drw_buf][0]);
}

/**********************************************************************************************************************
 * End of function graphics_get_draw_buffer
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Swap the active buffer in the graphics LCD controller
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_swap_buffer
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void graphics_swap_buffer ()
{
#if LCD_BUF_NUM > 1
    drw_buf = drw_buf ? 0 : 1;
#endif

    /* Update the layer to display in the GLCDC (will be set on next Vsync) */
    rotate_rgb565_90(fb_background[drw_buf], fb_rot, LCD_WIDTH, LCD_HEIGHT);

    // R_GLCDC_BufferChange(g_display0.p_ctrl, fb_background[0], 0); // fixed
    // R_GLCDC_BufferChange(g_display0.p_ctrl, fb_background[drw_buf], 0); // double buffered
    R_GLCDC_BufferChange(g_display0.p_ctrl, fb_rot, 0); // double buffered
    R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_1);
}

/**********************************************************************************************************************
 * End of function graphics_swap_buffer
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Start a new display list, set the framebuffer and add a clear operation
 *
 * This function will automatically prepare an empty framebuffer.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_start_frame
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void graphics_start_frame ()
{
    /* Start a new display list */
    d2_startframe(d2_handle);

    /* Set the new buffer to the current draw buffer */
    d2_framebuffer(d2_handle, graphics_get_draw_buffer(), LCD_HPIX, LCD_STRIDE, LCD_VPIX, EP_SCREEN_MODE);
}

/**********************************************************************************************************************
 * End of function graphics_start_frame
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * End the current display list and flip the active framebuffer
 *
 * WARNING: As part of d2_endframe the D2 driver will wait for the current frame to finish displaying.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_end_frame
 * Description  : .
 * Argument     :
 * Return Value : .
 *********************************************************************************************************************/
void graphics_end_frame ()
{
    /* End the current display list */
    d2_endframe(d2_handle);

    /* Flip the framebuffer */
    graphics_swap_buffer();

    /* Clean data cache */
    // SCB_CleanDCache();
}

/**********************************************************************************************************************
 * End of function graphics_end_frame
 *********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Converts float HSV values (0.0F-1.0F) to RGB888
 *
 * @param[in]  h   Hue
 * @param[in]  s   Saturation
 * @param[in]  v   Value
 *
 * @retval     uint32_t    RGB888 color
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: graphics_hsv2rgb888
 * Description  : .
 * Arguments    : h
 *              : s
 *              : v
 * Return Value : .
 *********************************************************************************************************************/
uint32_t graphics_hsv2rgb888 (float h, float s, float v)
{
    uint32_t r, g, b;

    if (s < 0.003F)                    // monochrome
    {
        r = b = g = (uint16_t) (v * 0xFF);
    }
    else
    {
        if (h >= 1.0F)
        {
            h = h - floorf(h);
        }

        if (s > 1.0F)
        {
            s = s - floorf(s);
        }

        if (v > 1.0F)
        {
            v = v - floorf(v);
        }

        h *= 6.0F;
        switch ((int) h)
        {
            case 0:
            {
                r = (uint32_t) (0xFF * v);
                g = (uint32_t) (0xFF * v * (1.0F - (s * (1.0F - h))));
                b = (uint32_t) (0xFF * v * (1.0F - s));
                break;
            }

            case 1:
            {
                r = (uint32_t) (0xFF * v * (1.0F - (s * (h - 1.0F))));
                g = (uint32_t) (0xFF * v);
                b = (uint32_t) (0xFF * v * (1.0F - s));
                break;
            }

            case 2:
            {
                r = (uint32_t) (0xFF * v * (1.0F - s));
                g = (uint32_t) (0xFF * (v));
                b = (uint32_t) (0xFF * v * (1.0F - (s * (1.0F - (h - 2.0F)))));
                break;
            }

            case 3:
            {
                r = (uint32_t) (0xFF * v * (1.0F - s));
                g = (uint32_t) (0xFF * v * (1.0F - (s * (h - 3.0F))));
                b = (uint32_t) (0xFF * (v));
                break;
            }

            case 4:
            {
                r = (uint32_t) (0xFF * v * (1.0F - (s * (1.0F - (h - 4.0F)))));
                g = (uint32_t) (0xFF * v * (1.0F - s));
                b = (uint32_t) (0xFF * (v));
                break;
            }

            case 5:
            {
                r = (uint32_t) (0xFF * (v));
                g = (uint32_t) (0xFF * v * (1.0F - s));
                b = (uint32_t) (0xFF * v * (1.0F - (s * (h - 5.0F))));
                break;
            }

            default:
                r = 0xFF;
                b = 0xFF;
                g = 0xFF;
        }
    }

    return (r << 16) + (g << 8) + b;
}

void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc)
{

    if (renderbuffer == NULL) {
        renderbuffer = d2_newrenderbuffer(d2_handle, 10, 10);
    }
    /* Set the new buffer to the current draw buffer */
    d2_framebuffer(d2_handle, pDst, LCD_XSTRIDE_PHYS, LCD_XSIZE_PHYS, LCD_YSIZE_PHYS, d2_mode_rgb565);

    d2_selectrenderbuffer(d2_handle, renderbuffer);
    //
    // Generate render operations
    //
    d2_setblitsrc(d2_handle, (void *) pSrc, (d2_s32) PitchSrc, WidthSrc, HeightSrc, d2_mode_rgb565);
    d2_blitcopy(d2_handle,
                WidthSrc,
                HeightSrc,
                0,
                0,
                (d2_width) ((256) << 4),
                (d2_width) ((480) << 4),

                0,
                0,
                0);

    d2_setcolor(d2_handle, 0, 0x00FF00);
    d2_setalpha(d2_handle, 0xee);

    /* End the current display list */
    d2_executerenderbuffer(d2_handle, renderbuffer, 0);
    d2_flushframe(d2_handle);
}

/**
 * @brief 旋转RGB565图像数组（支持90/180/270度）
 * @param src 输入图像数组指针（RGB565格式）
 * @param width 输入图像的宽度（像素数）
 * @param height 输入图像的高度（像素数）
 * @param angle 旋转角度（仅支持90/180/270）
 * @param rotated 输出图像数组指针（需预先分配足够空间）
 */
void graphics_rotate_image(uint16_t* input, int width, int height, int angle, uint16_t* output) {
    int w, h, new_width, new_height;
    // 参数有效性检查
    if (!input || !output || width <= 0 || height <= 0) return;

    // 根据角度执行不同旋转逻辑
    switch (angle) {
        case 90:  // 顺时针90度
            new_width = 3;
            new_height = 5;
            for (w = 0; w < new_width; w++) {
                for (h = 0; h < new_height; h++) {
                    output[h * new_width + w] = input[width * (height - 1 - w) + h];
                }
            }
            break;
        case 180:  // 顺时针180度（直接逆序）
            for (w = width * height - 1; w > 0; w--) {
                output[w] = input[width * height - w - 1];
            }
            output[0] = input[width * height - 1];
            break;
        case 270:  // 顺时针270度（逆时针90度）
            new_width = height;
            new_height = width;
            for (w = 0; w < new_width; w++) {
                for (h = 0; h < new_height; h++) {
                    output[h * new_width + w] = input[width * (w + 1) - h - 1];
                }
            }
            break;
        default:  // 无效角度直接返回
            return;
    }
}
