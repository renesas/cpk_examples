/***********************************************************************************************************************
 * File Name    : usb_thread_entry.c
 * Description  : Contains data structures and functions used in usb_thread_entry.c.
 **********************************************************************************************************************/

/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "FreeRTOS.h"
#include "task.h"
#include "usb_hcdc_app.h"

/* USB Thread entry function */
/* pvParameters contains TaskHandle_t */
void usb_thread_entry(void *pvParameters)
{
    (void)(pvParameters);
    usb_hcdc_task();
    while (1)
    {
        vTaskDelay (1);
    }
}
