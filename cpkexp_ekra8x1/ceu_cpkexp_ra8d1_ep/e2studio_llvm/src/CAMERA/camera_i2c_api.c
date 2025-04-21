/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "common_utils.h"


static volatile i2c_master_event_t i2c_master_event = 0;

fsp_err_t wait_for_i2c_event (i2c_master_event_t set_event)
{
    uint32_t timeout = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_ICLK) / 10;
    uint32_t get_event;

    do
    {
        get_event = i2c_master_event;
        get_event = (set_event & get_event);
        if(get_event)
        {
            i2c_master_event &= (~set_event);
            break;
        }
    }while(timeout--);

    return timeout ? FSP_SUCCESS : FSP_ERR_TIMEOUT;
}

/*******************************************************************************************************************//**
 * @brief      Callback functions for i2c master interrupts
 *
 * @param[in]  p_args    Callback arguments
 * @retval     none
 **********************************************************************************************************************/
void g_i2c_master_callback(i2c_master_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        i2c_master_event |= p_args->event;
    }
}


fsp_err_t camera_i2c_comm_write(uint32_t sub_address, uint32_t sub_address_length, const uint8_t *data, size_t data_length)
{
    fsp_err_t err = FSP_SUCCESS;
    size_t i;
    size_t buffer_length = sub_address_length + data_length;
    uint8_t buffer[buffer_length];
    i = 0;

    // Calculate Check sub-address length and copy it into the buffer
    if(sub_address_length == 1)
    {
        buffer[i++] = sub_address & 0xFF;
    }
    else if (sub_address_length == 2)
    {
        buffer[i++] = (sub_address >> 8 ) & 0xFF;
        buffer[i++] = sub_address & 0xFF;
    }
    else if (sub_address_length == 4)
    {
        buffer[i++] = (uint8_t) ((sub_address >> 24) & 0xFF);
        buffer[i++] = (uint8_t) ((sub_address >> 16) & 0xFF);
        buffer[i++] = (uint8_t) ((sub_address >> 8) & 0xFF);
        buffer[i++] = (uint8_t) (sub_address & 0xFF);
    }
    else
    {
#if USE_DEBUG_BREAKPOINTS
        __BKPT(0);
#endif
    }

    // Add the data to the buffer
    memcpy(buffer + i, data, data_length);

    // Write I2C data
    err = R_IIC_MASTER_Write(&g_i2c_master1_ctrl, buffer, buffer_length, false);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");

    /* Wait until write transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_TX_COMPLETE);
    APP_ERR_RETURN(err, " ** I2C master event timeout ** \r\n");

    return err;
}

fsp_err_t camera_i2c_comm_read(uint32_t sub_address, uint32_t sub_address_length, uint8_t *data, size_t data_length)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t buffer[4];

    // Check sub-address length and format the sub_address data accordingly
    if(sub_address_length == 1)
    {
        buffer[0] = sub_address & 0xFF;
    }
    else if (sub_address_length == 2)
    {
        buffer[0] = (sub_address >> 8 ) & 0xFF;
        buffer[1] = sub_address & 0xFF;
    }
    else if (sub_address_length == 4)
    {
        buffer[0] = (uint8_t) ((sub_address >> 24) & 0xFF);
        buffer[1] = (uint8_t) ((sub_address >> 16) & 0xFF);
        buffer[2] = (uint8_t) ((sub_address >> 8) & 0xFF);
        buffer[3] = (uint8_t) (sub_address & 0xFF);
    }
    else
    {
#if USE_DEBUG_BREAKPOINTS
        __BKPT(0);
#endif
    }

    // Write register index
//    err = R_IIC_MASTER_Write(&g_i2c_master_ctrl, &buffer[0], sub_address_length, true);   //Restart for OV5640
//    APP_ERR_RETURN(err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");

    // Write register index
    err = R_IIC_MASTER_Write(&g_i2c_master1_ctrl, &buffer[0], sub_address_length, false);   //Stop for OV7725
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");

    /* Wait until write transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_TX_COMPLETE);
    APP_ERR_RETURN(err, " ** I2C master event timeout ** \r\n");

    // Read data
    err = R_IIC_MASTER_Read(&g_i2c_master1_ctrl, data, data_length, false);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_Read API FAILED ** \r\n");

    /* Wait until read transmission complete */
    err = wait_for_i2c_event (I2C_MASTER_EVENT_RX_COMPLETE);
    APP_ERR_RETURN(err, " ** I2C master event timeout ** \r\n");

    return err;
}
