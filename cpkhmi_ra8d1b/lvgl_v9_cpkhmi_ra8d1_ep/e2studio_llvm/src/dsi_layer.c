/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

void mipi_dsi0_callback(mipi_dsi_callback_args_t * p_args);
static fsp_err_t dsi_layer_set_peripheral_max_return_msg_size(void);

static volatile bool g_message_sent = false;

fsp_err_t dsi_layer_configure_peripheral()
{
    fsp_err_t err = FSP_SUCCESS;
    LCD_setting_table * init_table = lcd_init_focuslcd;

    err = dsi_layer_set_peripheral_max_return_msg_size(); // This must be performed prior to reading from display
    if (FSP_SUCCESS == err)
    {

        LCD_setting_table * p_entry = init_table;
        uint32_t counter = 0;
        while(p_entry->msg_id != REGFLAG_END_OF_TABLE)
        {
            mipi_dsi_cmd_t msg = { .channel = 0,
                                   .cmd_id = p_entry->msg_id,
                                   .flags = p_entry->flags,
                                   .tx_len = p_entry->size,
                                   .p_tx_buffer = p_entry->buffer };


            if (p_entry->msg_id == REGFLAG_DELAY)
            {
                R_BSP_SoftwareDelay(p_entry->size, BSP_DELAY_UNITS_MILLISECONDS);
            }
            else
            {

                g_message_sent = false;
                err = R_MIPI_DSI_Command (&g_mipi_dsi0_ctrl, &msg);
                if (FSP_SUCCESS == err)
                {

                    while(!g_message_sent);

                    mipi_dsi_status_t status;
                    R_MIPI_DSI_StatusGet(&g_mipi_dsi0_ctrl, &status);
                    while (MIPI_DSI_LINK_STATUS_CH0_RUNNING & status.link_status)
                    {
                        R_MIPI_DSI_StatusGet(&g_mipi_dsi0_ctrl, &status);
                    }
                }
                else
                {
                    break;
                }

            }
            p_entry++;
            counter++;
        }
    }

    return err;
}


void mipi_dsi0_callback (mipi_dsi_callback_args_t * p_args)
{
    fsp_err_t err;
    switch (p_args->event)
    {
        case MIPI_DSI_EVENT_SEQUENCE_0:
        {
            g_message_sent |= (p_args->tx_status == MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED);
            break;
        }
        case MIPI_DSI_EVENT_SEQUENCE_1:
        {
            __NOP();
            break;
        }
        case MIPI_DSI_EVENT_VIDEO:
        {
            __NOP();
            break;
        }
        case MIPI_DSI_EVENT_RECEIVE:
        {
            __NOP();
            break;
        }
        case MIPI_DSI_EVENT_FATAL:
        {
            __NOP();
            break;
        }
        case MIPI_DSI_EVENT_PHY:
        {
            __NOP();
            break;
        }
        case MIPI_DSI_EVENT_POST_OPEN:
        {
            /* This case is called from R_DSI_Open(), so not from an interrupt */

            err = dsi_layer_configure_peripheral();
              if (FSP_SUCCESS != err)
              {
                  __BKPT(0);
              }
            break;
        }
        default:
        {
            break;
        }
    }
}

/* See ILI9806E Datasheet, chapter  3.5.39
 *  1. Set max return packet size
 *  2. Read data be sending appropriate request */
static fsp_err_t  dsi_layer_set_peripheral_max_return_msg_size()
{
    fsp_err_t err;
    uint8_t msg_buffer[] = {0x02, 0x00};
    mipi_dsi_cmd_t return_size_msg = { .channel = 0,
                                       .cmd_id = MIPI_DSI_CMD_ID_SET_MAXIMUM_RETURN_PACKET_SIZE,
                                       .flags = MIPI_DSI_CMD_FLAG_LOW_POWER,
                                       .tx_len = 2,
                                       .p_tx_buffer = msg_buffer, };
    /* Set Return packet size */
    g_message_sent = false;
    err = R_MIPI_DSI_Command (&g_mipi_dsi0_ctrl, &return_size_msg);
    if (FSP_SUCCESS == err)
    {
    while(!g_message_sent);
    }

    return err;
}

