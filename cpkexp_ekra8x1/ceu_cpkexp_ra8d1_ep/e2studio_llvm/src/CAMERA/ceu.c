/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/***********************************************************************************************************************
 * File Name    : ceu.c
 * Description  : Contains data structures and functions used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#include "ceu.h"
#include "ov7725.h"
#include "ov5640.h"
#include "math.h"

/* External variable */

/* Global variable */

uint32_t g_image_width = RESET_VALUE;
uint32_t g_image_height = RESET_VALUE;
uint8_t * gp_image_buffer = NULL;
volatile bool g_capture_ready = false;

/*******************************************************************************************************************//**
 *  @brief      ceu vga callback function
 *  @param[in]  p_args
 *  @retval     None
 **********************************************************************************************************************/
void g_ceu_vga_callback (capture_callback_args_t * p_args)
{
    if (CEU_EVENT_FRAME_END == p_args->event )
    {
        g_capture_ready = true;
    }
}

/*******************************************************************************************************************//**
 *  @brief      ceu init function
 *  @param[in]  p_instance : ceu instance pointer
 *  @param[in]  p_buffer : image buffer pointer
 *  @param[in]  width : width of image
 *  @param[in]  height : height of image
 *  @retval     FSP_SUCCESS   Upon successful operation
 *  @retval     Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
fsp_err_t ceu_init(uint8_t * const p_buffer, uint32_t width, uint32_t height)
{
    fsp_err_t   err;

    /* Initialize CEU module with the configuration specified by CEU instance pointer */
    err = R_CEU_Open(&g_ceu_vga_ctrl, &g_ceu_vga_cfg);
    APP_ERR_RETURN(err, " ** R_CEU_Open API FAILED ** \r\n");

//    err = R_CEU_Open(&g_ceu_rgb_ctrl, &g_ceu_rgb_cfg);
//    APP_ERR_RETURN(err, " ** R_CEU_Open API FAILED ** \r\n");

    /* Clean image buffer */
    memset(p_buffer, RESET_VALUE, width * height * YUV422_BYTE_PER_PIXEL);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 *  @brief      ceu operation function
 *  @param[in]  p_instance : ceu instance pointer
 *  @param[in]  p_buffer : image buffer pointer
 *  @param[in]  width : width of image
 *  @param[in]  height : height of image
 *  @retval     FSP_SUCCESS   Upon successful operation
 *  @retval     Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
//fsp_err_t ceu_operation (uint8_t * const p_buffer, uint32_t width, uint32_t height)
fsp_err_t ceu_operation (uint8_t * const p_buffer)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t timeout = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_ICLK) / 10;

    /* Print capture operation start */
//    APP_PRINT("\r\nImage Capturing Operation started\r\n");

    /* Start capture image and store it in the buffer specified by image buffer pointer */
    err = R_CEU_CaptureStart(&g_ceu_vga_ctrl, p_buffer);
//    APP_ERR_RETURN(err, " ** R_CEU_CaptureStart API FAILED ** \r\n");

    /*  Wait until CEU callback triggers */
    while (true != g_capture_ready)
    {
        /* Start checking for time out to avoid infinite loop */
        timeout --;
        /* Check for time elapse*/
        if (RESET_VALUE == timeout)
        {
            APP_ERR_RETURN(FSP_ERR_TIMEOUT, " ** CEU Callback event not received ** \r\n");
        }
    }

    /* Print success notice */
//    APP_PRINT("\r\nCEU Capture Successful !\r\n");

    /* Reset capture flag */
    g_capture_ready = false;
    return FSP_SUCCESS;
}

// YUV422 non-swapped data format : Y0 U0 Y1 V2 Y2 U2 Y3 V4 Y4 U4 Y5 V6 Y6 U6 Y7…
// YUV422 swapped data format     : U0 Y0 V0 Y1 U2 Y2 V2 Y3 U4 Y4 V4 Y5 U6 Y6 V6…

//*****************************//
// Pixel Number | Pixel Values //
//      0       | 0Y0V0        //
//      1       | U0Y1V0       //
//      2       | U2Y2V2       //
//      3       | U2Y3V2       //
//      4       | U4Y4V4       //
//     ...      |  ...         //
//*****************************//
#define RANGE_LIMIT(x)        (x > 255 ? 255 : (x < 0 ? 0 : x))

void yuv422_to_rgb888(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v;

    int32_t  r8, g8, b8;
    uint8_t  *yuv_buf;
    uint32_t *rgb_buf = (uint32_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;

    yuv_buf = (uint8_t *)inbuf;
    uint32_t x_start, y_start;
    int32_t temp;

    uint32_t rgb888_pixel_data = 0;

    SCB_EnableDCache();

    x_start = 0;
    y_start = 0;

    // YUV422 swapped data format : U0 Y0 V0 Y1 U2 Y2 V2 Y3 U4 Y4 V4 Y5 U6 Y6 V6…
    y_pos = 1;
    u_pos = 0;
    v_pos = 2;

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r8 = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g8 = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b8 = (int32_t) RANGE_LIMIT( temp );

            // RGB rearrange & merge back into RGB888 pixel
            rgb888_pixel_data = (uint32_t) ( ( r8 << 16 ) | ( g8 << 8 ) | ( b8 ) );

            // Display pixel directly into the screen working buffer
            rgb_buf[ ( ( rows + y_start ) * width ) + ( columns + x_start) ] = rgb888_pixel_data;

            rgb888_pixel_data = 0;
            // Move to next pixel
            y_pos += 2;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

    SCB_DisableDCache();
}

void yuv422_to_rgb565(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v, r, g, b;
    uint8_t  *yuv_buf;
    uint16_t *rgb_buf = (uint16_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;

    yuv_buf = (uint8_t *)inbuf;
    uint32_t x_start, y_start;
    int32_t temp;
    uint16_t pixel_data;

    SCB_EnableDCache();

    x_start = 0;
    y_start = 0;

    y_pos = 1;//1; // 0 1
    u_pos = 0;//0; // 1 0
    v_pos = 2;//2; // 3 2

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b = (int32_t) RANGE_LIMIT( temp );

            // RGB rearrange & merge back into RGB565 pixel
            pixel_data = (uint16_t) ( ( (r & 0xF8) << 8 ) | ( (g & 0xFC) << 3 ) | ( (b & 0xF8) >> 3 ) );

            // Display pixel directly into the screen working buffer
            rgb_buf[ ( ( rows + y_start ) * width ) + ( columns + x_start) ] = pixel_data;

            // Move to next pixel
            y_pos += 2;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

    SCB_DisableDCache();
}
void YUVtoRGB565(uint8_t *yuv, uint16_t *rgb565, int width, int height) {
    int i, j;
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 2) {
                // 从YUV数据中提取每个像素的Y、U和V值
                uint8_t y0 = yuv[i * width * 2 + j * 2];
                uint8_t u = yuv[i * width * 2 + j * 2 + 1];
                uint8_t y1 = yuv[i * width * 2 + j * 2 + 2];
                uint8_t v = yuv[i * width * 2 + j * 2 + 3];

                // YUV转换为RGB
                int c0 = y0 - 16;
                int c1 = y1 - 16;
                int d = u - 128;
                int e = v - 128;

                uint16_t r0 = (298 * c0 + 409 * e + 128) >> 8;
                uint16_t g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
                uint16_t b0 = (298 * c0 + 516 * d + 128) >> 8;

                uint16_t r1 = (298 * c1 + 409 * e + 128) >> 8;
                uint16_t g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
                uint16_t b1 = (298 * c1 + 516 * d + 128) >> 8;

                // Clamp values
                if (r0 > 0x1F) r0 = 0x1F;
                if (g0 > 0x3F) g0 = 0x3F;
                if (b0 > 0x1F) b0 = 0x1F;

                if (r1 > 0x1F) r1 = 0x1F;
                if (g1 > 0x3F) g1 = 0x3F;
                if (b1 > 0x1F) b1 = 0x1F;

                // 将RGB值组合成RGB565格式并保存
                rgb565[i * width + j] = (r0 << 11) | (g0 << 5) | b0;
                rgb565[i * width + j + 1] = (r1 << 11) | (g1 << 5) | b1;
            }
        }
}


