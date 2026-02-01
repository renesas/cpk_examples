/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "blinky_thread.h"
#include "common_utils.h"
#include "lwprintf.h"

#define LED_PIN (BSP_IO_PORT_04_PIN_12)   //CPU1 for LED401  P412
volatile bool           led_status = false;
uint32_t led_blinky_wait_ms = 100;

extern void Debug_UART0_Init(void);
extern int lwprintf_output_func(int ch, lwprintf_t *lw);

extern void rp_ping_pong(void);

/* Blinky Thread entry function */
void blinky_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
    Debug_UART0_Init();
    lwprintf_init(lwprintf_output_func);
    lwprintf_printf("initial SCI0 port!\r\n");
    lwprintf_printf("start rpmsg task for m33!\r\n");
    lwprintf_printf("<< rpmsg-lite component testcase:PING_PONG >>\r\n");

    while (1)
    {

        rp_ping_pong();
        led_status = !led_status;
        R_IOPORT_PinWrite(&g_ioport_ctrl, LED_PIN, (bsp_io_level_t)led_status);

        R_BSP_SoftwareDelay(led_blinky_wait_ms, BSP_DELAY_UNITS_MILLISECONDS);
        vTaskDelay (1);
    }
}
