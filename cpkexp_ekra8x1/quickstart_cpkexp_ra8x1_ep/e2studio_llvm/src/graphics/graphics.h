/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : graphics.h
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>
#include "hal_data.h"
#include "dave_driver.h"

#define LCD_HPIX         (480)
#define LCD_STRIDE       (480)
#define LCD_VPIX         (256)

#define LCD_WIDTH	(LCD_HPIX)
#define LCD_HEIGHT	(LCD_VPIX)
#define LCD_PITCH	(LCD_WIDTH)

#define LCD_COLOR_SIZE    (2)
#define LCD_BUF_NUM       (2)

#define D2_FIX(x)   ((x) * 16)

extern d2_device * d2_handle;

void     graphics_init(void);
void   * graphics_get_draw_buffer();
void     graphics_start_frame();
void     graphics_end_frame();
void     graphics_swap_buffer();
void     graphics_wait_vsync();
uint32_t graphics_hsv2rgb888(float h, float s, float v);
void     graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc);
void     graphics_rotate_image(uint16_t* input, int width, int height, int angle, uint16_t* output);

#endif
