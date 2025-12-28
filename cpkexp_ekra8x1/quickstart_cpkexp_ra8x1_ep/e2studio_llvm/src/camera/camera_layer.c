/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "camera_layer.h"
#include "hal_data.h"
#include "board_cfg.h"
#include "r_ceu.h"

#include "r_capture_api.h"

#include "ov7725.h"

#define CAM_DATA_READY               (1 << 0)

// #define USE_DEBUG_BREAKPOINTS 1

#define RANGE_LIMIT(x)    (x > 255 ? 255 : (x < 0 ? 0 : x))

static capture_event_t g_last_cam_event = CEU_EVENT_NONE; ///< Event causing the callback
bool g_capture_ready = false;
bool image_processed = true;

uint8_t bsp_camera_out_buffer[2][BSP_CAM_WIDTH * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_camera_out_buffer565[BSP_CAM_WIDTH * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_camera_out_buffer888[BSP_CAM_WIDTH * BSP_CAM_HEIGHT * 3] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_det_model_ip_buffer888[192 * 192 * 3] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_det_crop_model_ip_buffer888[240 * 240 * 3] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_camera_out_rot_buffer565[2][BSP_CAM_WIDTH * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t throw_away_buffer[BSP_CAM_WIDTH * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t bsp_rec_model_ip_buffer888[224 * 224 * 3] BSP_ALIGN_VARIABLE(8);
uint8_t bsp_cls_model_ip_buffer888[224 * 224 * 3] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);

uint8_t g_rgb_buffer = 0;              // double buffering current display buffer
uint8_t s_ceu_buffer = 0;              // double buffering current capture buffer

void rot90_clock(uint8_t * input_image, uint8_t * output_image, int n_ch, int ip_w, int ip_h);
void bsp_camera_rgb565_to_rgb888(const void * inbuf, void * outbuf, uint16_t width, uint16_t height);

/**********************************************************************************************************************
 * Function Name: bsp_camera_rgb565_to_rgb888
 * Description  : .
 * Argument     : .
 * Return Value : .
 *********************************************************************************************************************/
void bsp_camera_rgb565_to_rgb888 (const void * inbuf, void * outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;

    register uint16_t * in_data  = (uint16_t *) inbuf;
    register uint8_t  * out_data = (uint8_t *) outbuf;

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            *out_data++ = (uint8_t) ((*in_data >> 8) & 0xF8);
            *out_data++ = (uint8_t) (((0x07E0 & *in_data) >> 3) & 0xFC);
            *out_data++ = (uint8_t) (((0x001F & *in_data)) << 3);

            in_data++;
        }
    }
}

static int camera_sensor_init(void)
{
	return ov7725_init();
}

/**********************************************************************************************************************
 * Function Name: camera_init
 * Description  : .
 * Argument     : use_test_mode
 * Return Value : .
 *********************************************************************************************************************/
bool_t camera_init(bool_t use_test_mode)
{
    byte   reg_val1          = 0;
    byte   reg_val2          = 0;

    ov7725_init();

    /* Open camera module */
    R_CEU_Open(&g_ceu_ctrl, &g_ceu_cfg);

    R_CEU_CaptureStart(&g_ceu_ctrl, bsp_camera_out_buffer[s_ceu_buffer]);

    return true;
}

/**********************************************************************************************************************
 * End of function camera_init
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * End of function rot90_clock
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: bsp_camera_capture_image
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
uint8_t *bsp_camera_capture_image(void)
{
	return &bsp_camera_out_buffer[g_rgb_buffer][0];
}

uint8_t *bsp_get_camera_rot_buf(void)
{
	return &bsp_camera_out_rot_buffer565[0];
}

/**********************************************************************************************************************
 * End of function bsp_camera_capture_image
 *********************************************************************************************************************/
void g_ceu_user_callback (capture_callback_args_t * p_args)
{
   BaseType_t xHigherPriorityTaskWoken, xResult;

    /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
    xHigherPriorityTaskWoken = pdFALSE;
    if (CEU_EVENT_FRAME_END & p_args->event)
    {
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, CAM_DATA_READY, &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if (xResult != pdFAIL)
        {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
             * switch should be requested.  The macro used is port specific and will
             * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
             * the documentation page for the port being used. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

//        g_rgb_buffer = s_ceu_buffer;
//        s_ceu_buffer = !s_ceu_buffer;
        R_CEU_CaptureStart(&g_ceu_ctrl, bsp_camera_out_buffer[s_ceu_buffer]);
        g_rgb_buffer = s_ceu_buffer;
        s_ceu_buffer = s_ceu_buffer == 0 ? 1 : 0;
    }

    g_last_cam_event = p_args->event;
}
