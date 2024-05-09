/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include <stdio.h>

int user_read(char *buffer, int file, int count);
int user_write(char ptr, FILE* file);

volatile bool g_data_received_flag = false;
volatile bool g_data_transmit_flag = false;

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

int user_read(char *buffer, int file, int count)
{
    FSP_PARAMETER_NOT_USED(file);

    int bytesReceived = 0;


    for (int i = 0; i < count; i++)
    {
        // Start Transmission
        g_data_received_flag = false;
        fsp_err_t err = R_SCI_B_UART_Read(&g_uart3_ctrl, (uint8_t * const)(buffer + i), 1U);
        if (FSP_SUCCESS != err)
        {
             return -1;
        }
        /* Wait for event receive complete */
        while (!g_data_received_flag)
        {
        }

        bytesReceived++;

        if ((char)(buffer[i]) == '\r') //Break out of the loop if ENTER is pressed
        {
            break;
        }
    }
    __NOP();
    return bytesReceived;
}

int user_write(char ptr, FILE* file)
{
  FSP_PARAMETER_NOT_USED(file);

  // Start Transmission
  g_data_transmit_flag = false;
  if(ptr != 0)
  {
      fsp_err_t err = R_SCI_B_UART_Write(&g_uart3_ctrl, (uint8_t * const)(&ptr), 1U);
      if (FSP_SUCCESS != err)
      {
           return -1;
      }
  }
  else
      return 0;

  // Wait for event receive complete
  while (!g_data_transmit_flag)
  {
  }

  return FSP_SUCCESS;
}

/* Redirect sdtio as per https://github.com/picolibc/picolibc/blob/main/doc/os.md */
static FILE __stdio = FDEV_SETUP_STREAM(user_write, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);


