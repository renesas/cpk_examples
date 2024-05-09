/***********************************************************************************************************************
 * File Name    : usb_thread_entry.c
 * Description  : Contains data structures and functions used in usb_thread_entry.c.
 **********************************************************************************************************************/
 
/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "usb_hcdc_thread.h"
#include "usb_hcdc_app.h"

/* USB Thread entry function */
/* pvParameters contains TaskHandle_t */
void usb_hcdc_thread_entry(void *pvParameters)
{
    R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_09, ((uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW));
    R_IOPORT_Close(&g_ioport_ctrl);

    FSP_PARAMETER_NOT_USED (pvParameters);
    usb_hcdc_task();
    while (true)
    {
        vTaskDelay (1000);
    }
}
