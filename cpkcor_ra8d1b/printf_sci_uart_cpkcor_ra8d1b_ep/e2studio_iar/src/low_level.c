/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include <stdio.h>
#include <stdbool.h>

#define BUFF_SIZE 64

char buffer[BUFF_SIZE];

volatile bool g_data_received_flag = false;
volatile bool g_data_transmit_flag = false;

int __write(int fd, char *pBuffer, int size);
int __read(int fd, char *pBuffer, int size);

void user_uart3_callback(uart_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
            g_data_transmit_flag = true;
        break;

        case UART_EVENT_RX_COMPLETE:
            g_data_received_flag = true;
        break;

        default:
        break;
    }
}

int __write(int fd, char *pBuffer, int size)
{
    FSP_PARAMETER_NOT_USED(fd);

    // Start Transmission
    g_data_transmit_flag = false;

    fsp_err_t err = R_SCI_B_UART_Write(&g_uart3_ctrl, (uint8_t * const)(pBuffer), size);
    if (FSP_SUCCESS != err)
    {
         return -1;
    }

    // Wait for event receive complete
    while (!g_data_transmit_flag)
    {
    }

    return FSP_SUCCESS;
}
//
//int fgetc(FILE *f)
//{
//    volatile int bytesReceived = 0;
//    FSP_PARAMETER_NOT_USED(f);
//
//    for (int i = 0; i < BUFF_SIZE; i++)
//    {
//        // Start Transmission
//        g_data_received_flag = false;
//        fsp_err_t err = R_SCI_B_UART_Read(&g_uart3_ctrl, (uint8_t * const)(buffer + i), 1U);
//        if (FSP_SUCCESS != err)
//        {
//             return -1;
//        }
//        /* Wait for event receive complete */
//        while (!g_data_received_flag)
//        {
//        }
//
//        bytesReceived++;
//
//        if ((char)(buffer[i]) == '\r') //Break out of the loop if ENTER is pressed
//        {
//            break;
//        }
//    }
//    __NOP();
//    return bytesReceived;
//}
