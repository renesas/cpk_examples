/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include <stdarg.h>
#include "arducam.h"
#include "hal_data.h"



fsp_err_t i2c_cam_cb_wait(void);

#define I2C_TRANSFER_COMPLETE  (1<<0)
#define I2C_TRANSFER_ABORT     (1<<1)

#define I2C_TIMEOUT_MS         1000

volatile bool g_i2c_completed = false;
uint8_t g_i2c_flag=0;

/* Called from touch i2c isr routine */
void g_i2c_master1_cb(i2c_master_callback_args_t * p_args)
{
#if BSP_CFG_RTOS == 2               // FreeRTOS
    BaseType_t xHigherPriorityTaskWoken;
    BaseType_t xResult = pdFAIL;

      /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
      xHigherPriorityTaskWoken = pdFALSE;

    if ((I2C_MASTER_EVENT_TX_COMPLETE == p_args->event) || (I2C_MASTER_EVENT_RX_COMPLETE == p_args->event))
    {
        xResult = xEventGroupSetBitsFromISR(g_i2c_event_group, I2C_TRANSFER_COMPLETE, &xHigherPriorityTaskWoken );
    }
    else if (I2C_MASTER_EVENT_ABORTED == p_args->event)
    {
        xResult = xEventGroupSetBitsFromISR(g_i2c_event_group, I2C_TRANSFER_ABORT, &xHigherPriorityTaskWoken );
    }
    else
    {
       //should never get here.
    }

    /* Was the message posted successfully? */
    if( pdFAIL != xResult)
    {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
        switch should be requested.  The macro used is port specific and will
        be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
        the documentation page for the port being used. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
#else
    FSP_PARAMETER_NOT_USED(p_args);
//    g_i2c_completed = true;

    if ((I2C_MASTER_EVENT_TX_COMPLETE == p_args->event) || (I2C_MASTER_EVENT_RX_COMPLETE == p_args->event))
    {
        g_i2c_completed = true;
        g_i2c_flag = I2C_TRANSFER_COMPLETE;
    }
    else if (I2C_MASTER_EVENT_ABORTED == p_args->event)
    {
        g_i2c_completed = true;
        g_i2c_flag = I2C_TRANSFER_ABORT;
    }
    else
    {
       //should never get here.
    }
#endif
}

fsp_err_t i2c_cam_cb_wait(void)
{
    fsp_err_t ret = FSP_SUCCESS;
    while(g_i2c_completed == false)
        {
        }
        g_i2c_completed = false;

    if ((I2C_TRANSFER_COMPLETE & g_i2c_flag) == I2C_TRANSFER_COMPLETE)
    {
        ret = FSP_SUCCESS;
    }
    else if ((I2C_TRANSFER_ABORT & g_i2c_flag) == I2C_TRANSFER_ABORT)
    {
        ret = FSP_ERR_ABORTED;
    }
    else
    {
        /* xEventGroupWaitBits() returned because of timeout */
        ret = FSP_ERR_TIMEOUT;
    }

    return ret;
}

fsp_err_t wrSensorReg16_8(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t regDat)
{
    fsp_err_t err;

    uint8_t data[3] = {(uint8_t) (regID >> 8), (uint8_t) regID, regDat};

    err = R_IIC_MASTER_Write(p_api_ctrl, data, 3, false);
    if (FSP_SUCCESS == err)
    {
        err = i2c_cam_cb_wait();
    }

    return err;
}

fsp_err_t rdSensorReg16_8(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t* regDat)
{
    fsp_err_t err;

    uint8_t data[2] = {(uint8_t) (regID >> 8), (uint8_t) regID};

    err = R_IIC_MASTER_Write(p_api_ctrl, data, 2, true);
    if (FSP_SUCCESS == err)
    {
        err = i2c_cam_cb_wait();
        if (FSP_SUCCESS == err)
        {
            err = R_IIC_MASTER_Read(p_api_ctrl, regDat, 1, false);
            if (FSP_SUCCESS == err)
            {
                err = i2c_cam_cb_wait();
            }
        }
    }

    return err;
}



fsp_err_t rdSensorReg16_Multi(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t* regDat, uint32_t len)
{
    fsp_err_t err;

    uint8_t data[2] = {(uint8_t) (regID >> 8), (uint8_t) regID};

    err = R_IIC_MASTER_Write(p_api_ctrl, data, 2, true);
    if (FSP_SUCCESS == err)
    {
        err = i2c_cam_cb_wait();
        if (FSP_SUCCESS == err)
        {
            err = R_IIC_MASTER_Read(p_api_ctrl, regDat, len, false);
            if (FSP_SUCCESS == err)
            {
                err = i2c_cam_cb_wait();
            }
        }
    }

    return err;
}
