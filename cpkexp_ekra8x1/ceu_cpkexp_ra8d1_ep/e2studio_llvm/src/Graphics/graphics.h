/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * graphics.h
 *
 *  Created on: Sep 5, 2023
 *      Author: a5123412
 */

#ifndef GRAPHICS_GRAPHICS_H_
#define GRAPHICS_GRAPHICS_H_

#define BYTES_PER_PIXEL                              (4)
#define COLOR_BAND_COUNT                             (8)
//#define BLUE                                         (0x000000FF)
//#define LIME                                         (0xFF00FF00)
//#define RED                                          (0x00FF0000)
//#define BLACK                                        (0x00000000)
//#define WHITE                                        (0xFFFFFFFF)
//#define YELLOW                                       (0xFFFFFF00)
//#define AQUA                                         (0xFF00FFFF)
//#define MAGENTA                                      (0x00FF00FF)


extern uint32_t * gp_single_buffer;
extern uint32_t * gp_double_buffer;
extern uint32_t * gp_frame_buffer;
extern uint16_t g_hz_size, g_vr_size;
extern uint32_t g_buffer_size, g_hstride;

void graphics_init(void);
//void graphics_draw_frame(const void * pSrc, int PitchSrc, int WidthSrc, int HeightSrc);
void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc);
//void display_draw (uint32_t * framebuffer);

#endif /* GRAPHICS_GRAPHICS_H_ */
