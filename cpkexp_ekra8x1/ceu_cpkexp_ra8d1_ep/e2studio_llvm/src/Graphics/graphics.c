/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * graphics.c
 *
 *  Created on: Sep 5, 2023
 *      Author: a5123412
 */

#include "hal_data.h"
#include "common_utils.h"
#include "Graphics\graphics.h"

#define LCD_BITS_PER_PIXEL        (32)
#define LCD_XSTRIDE_PHYS          (((DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0 * LCD_BITS_PER_PIXEL + 0x1FF) & 0xFFFFFE00) / LCD_BITS_PER_PIXEL)
#define LCD_XSIZE_PHYS            (DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0)
#define LCD_YSIZE_PHYS            (DISPLAY_VSIZE_INPUT0)
#define LCD_NUM_FRAMEBUFFERS      (2)

extern d2_device *d2_handle;

d2_device ** _d2_handle_user = &d2_handle;
static d2_renderbuffer * renderbuffer;

/* Variables to store resolution information */
uint16_t g_hz_size, g_vr_size;

/* Variables used for buffer usage */
uint32_t g_buffer_size, g_hstride;
uint32_t * gp_single_buffer = NULL;
uint32_t * gp_double_buffer = NULL;
uint32_t * gp_frame_buffer = NULL;

void graphics_init(void)
{
    /* Get LCDC configuration */
    g_hz_size = (g_display_cfg.input[0].hsize);
    g_vr_size = (g_display_cfg.input[0].vsize);
    g_hstride = (g_display_cfg.input[0].hstride);

    /* Initialize buffer pointers */
    g_buffer_size = (uint32_t) (g_hz_size * g_vr_size * BYTES_PER_PIXEL);
    gp_single_buffer = (uint32_t*) g_display_cfg.input[0].p_base;

    /* Double buffer for drawing color bands with good quality */
    gp_double_buffer = gp_single_buffer + g_buffer_size;

    //
    // Initialize D/AVE 2D driver
    //
    *_d2_handle_user = d2_opendevice(0);
    d2_inithw(*_d2_handle_user, 0);

    d2_framebuffer(*_d2_handle_user,
                    g_display_cfg.input[0].p_base,
                   LCD_XSTRIDE_PHYS,
                   LCD_XSIZE_PHYS,
                   LCD_YSIZE_PHYS * LCD_NUM_FRAMEBUFFERS,
                   d2_mode_rgb565);
    d2_clear(*_d2_handle_user, 0x000000);

    // Set various D2 parameters
    d2_setblendmode(*_d2_handle_user, d2_bm_alpha, d2_bm_one_minus_alpha);
    d2_setalphamode(*_d2_handle_user, d2_am_constant);
    d2_setalpha(*_d2_handle_user, UINT8_MAX);
    d2_setantialiasing(*_d2_handle_user, 1);
    d2_setlinecap(*_d2_handle_user, d2_lc_butt);
    d2_setlinejoin(*_d2_handle_user, d2_lj_miter);

#if 1
    renderbuffer = d2_newrenderbuffer(*_d2_handle_user, 10, 10);
#endif
}

/*******************************************************************************************************************//**
 * Start a new display list, set the framebuffer and add a clear operation
 *
 * This function will automatically prepare an empty framebuffer.
 **********************************************************************************************************************/
void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc)
{

    /* Set the new buffer to the current draw buffer */
    d2_framebuffer(*_d2_handle_user, pDst, LCD_XSTRIDE_PHYS, LCD_XSIZE_PHYS, LCD_YSIZE_PHYS, d2_mode_rgb565);

    d2_selectrenderbuffer(*_d2_handle_user, renderbuffer);
    //
    // Generate render operations
    //
    d2_setblitsrc(*_d2_handle_user, (void *) pSrc, (d2_s32) PitchSrc, WidthSrc, HeightSrc, d2_mode_rgb565);
    d2_blitcopy(*_d2_handle_user,
                WidthSrc,
                HeightSrc,
                0,
                0,
                (d2_width) ((256) << 4),
                (d2_width) ((480) << 4),

                0,
                0,
                0);

//    d2_blitcopy(*_d2_handle_user,
//                WidthSrc,
//                HeightSrc,
//                0,
//                0,
//
//                (d2_width) (222 << 4),
//                (d2_width) ((480/2) << 4),
//                0,
//                (d2_width) ((480/2) << 4),
//                0);


//    d2_blitcopy(*_d2_handle_user,
//                WidthSrc,
//                HeightSrc,
//                0,
//                0,
//                (d2_width) (1280/2 << 4),
//                (d2_width) (720 << 4),
//                (d2_width) (1280/2 << 4),
//                0,
//                0);

//    d2_blitcopy(*_d2_handle_user,
//                    WidthSrc,
//                    HeightSrc,
//                    0,
//                    0,
//                    (d2_width) (480 << 4),
//                    (d2_width) (854/2 << 4),
//                    0,
//                    (854/2)<<4,
//                    0);


    d2_setcolor(d2_handle, 0, 0x00FF00);
    d2_setalpha(d2_handle, 0xee);

#if 0
    d2_outlinewidth(d2_handle,8<<4);
    d2_renderquad_outline(d2_handle ,
            (d2_point) ((150)<<4), (150) << 4,
            (d2_point) ((300)<< 4), (150) << 4,
            (d2_point) ((300)<< 4), (300) << 4,
            (d2_point) ((150)<< 4), (300) << 4,
            0);

    d2_setcolor(d2_handle, 0, 0xFF0000);
    d2_setalpha(d2_handle, 0xff);
    d2_outlinewidth(d2_handle,3<<4);
    for(int i=0;i<50;i++){
//        d2_renderquad_outline(d2_handle ,
//                (d2_point) ((100)<<4), (100) << 4,
//                (d2_point) ((i+250)<< 4), (i+100) << 4,
//                (d2_point) ((i+250)<< 4), (i+250) << 4,
//                (d2_point) ((i+100)<< 4), (i+250) << 4,
//                0);
        d2_renderbox_outline(d2_handle ,
                        (d2_point) ((10)<<4), (i*16+10) << 4,
                        (d2_point) ((200)<< 4), (10) << 4,
                        0);
    }
    d2_setcolor(d2_handle, 0, 0x13901F);
    d2_setalpha(d2_handle, 0xff);
    for(int i=0;i<50;i++){
    //        d2_renderquad_outline(d2_handle ,
    //                (d2_point) ((100)<<4), (100) << 4,
    //                (d2_point) ((i+250)<< 4), (i+100) << 4,
    //                (d2_point) ((i+250)<< 4), (i+250) << 4,
    //                (d2_point) ((i+100)<< 4), (i+250) << 4,
    //                0);
            d2_renderbox_outline(d2_handle ,
                            (d2_point) ((240)<<4), (i*16+10) << 4,
                            (d2_point) ((200)<< 4), (10) << 4,
                            0);
        }
#endif

    /* End the current display list */
    d2_executerenderbuffer(*_d2_handle_user, renderbuffer, 0);
    d2_flushframe(*_d2_handle_user);
}

/*******************************************************************************************************************//**
 * @brief      User-defined function to draw the current display to a framebuffer.
 *
 * @param[in]  framebuffer    Pointer to frame buffer.
 * @retval     None.
 **********************************************************************************************************************/
//void display_draw (uint32_t * framebuffer)
//{
//    /* Draw buffer */
////    uint32_t color[COLOR_BAND_COUNT]= {BLUE, LIME, RED, BLACK, WHITE, YELLOW, AQUA, MAGENTA};
////    uint16_t bit_width = g_hz_size / COLOR_BAND_COUNT;
////    for (uint32_t y = 0; y < g_vr_size; y++)
////    {
////        for (uint32_t x = 0; x < g_hz_size; x ++)
////        {
////            uint32_t bit       = x / bit_width;
////            framebuffer[x] = color [bit];
////        }
////        framebuffer += g_hstride;
////    }
//    for(uint32_t i=0;i<480/2;i++)
//    {
//        for(uint32_t j=0;j<854;j++){
//            framebuffer[i*854+j] = RED;
//        }
//    }
//
////    for(uint32_t i=0;i<480*854/2;i++)
////    {
////        framebuffer[i] = BLUE;
////    }
//}
