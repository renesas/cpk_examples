/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include <LVGL_thread.h>
#include <string.h>
#include <stdio.h>
#include <touch_GT911.h>
#include "arducam.h"

#define GT_911_I2C_ADDRESS_0x5D  0x5D
#define GT_911_I2C_ADDRESS_0x14  0x14

#define GT_911_I2C_ADDRESS  GT_911_I2C_ADDRESS_0x14// GT_911_I2C_ADDRESS_0x5D

//#define WRITE_GT11_FW
//#define DUMP_GT911_REGS



static fsp_err_t productId(i2c_master_ctrl_t * p_api_ctr, char *target);

#ifdef DUMP_GT911_REGS
uint8_t g_read_config[184];
#endif

#ifdef WRITE_GT11_FW

/* Read from a virgin ER45RA-MW276-C LCD */
static const uint8_t g911xFW[] = {
///* 0x41, */ 0x81, 0xe0, 0x01, 0x56, 0x03, 0x05, 0x05, 0x00, 0x01, 0x08, 0x28, 0x05, 0x50,
//0x32, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x86, 0x27, 0x08, 0x17, 0x15, 0x31, 0x0d, 0x00, 0x00, 0x02, 0xbb, 0x03,
//0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x64, 0x32, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x10, 0x0e, 0x0c, 0x0a,
//0x08, 0x06, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
//0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xff,
//0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00


///* 0x41, */ 0x81, 0x20, 0x03, 0x00, 0x05, 0x05, 0x35, 0x00, 0x01, 0x08, 0x28, 0x05, 0x50,
//0x32, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x86, 0x27, 0x08, 0x17, 0x15, 0x31, 0x0d, 0x00, 0x00, 0x02, 0xbb, 0x03,
//0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x64, 0x32, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x10, 0x0e, 0x0c, 0x0a,
//0x08, 0x06, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
//0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xff,
//0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00

/* 0x41, */
0, 32, 3, 0, 5, 5, 53, 0, 1, 15, 40, 15, 80,
50, 3, 5, 0, 0, 0, 0, 0, 0, 6, 24, 26, 30,
20, 141, 45, 119, 49, 51, 178, 4, 0, 0, 0, 65, 3,
36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30,
80, 148, 208, 2, 7, 0, 0, 4, 174, 33, 0, 148, 40,
0, 127, 49, 0, 112,59, 0, 100, 72, 0, 100, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 17, 16, 15, 14, 13,
12, 9, 8, 7, 6, 5, 4, 1, 0, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 39,
38, 37, 36, 35, 34, 33, 32, 31, 30, 28, 27, 25, 0,
2, 4, 6, 7, 8, 10, 12, 15, 16, 17, 18, 19, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x10, 0x0e, 0x0c, 0x0a,
//0x08, 0x06, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
//0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0xff,
//0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//0x00, 0x00
};

#endif


void reset_gt911(void);

/**********************************************************************************************************************
 * Function definitions
 **********************************************************************************************************************/

void touch_irq_callback(external_irq_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    BaseType_t xHigherPriorityTaskWoken;
    BaseType_t xResult;

    xResult = xSemaphoreGiveFromISR( g_irq_binary_semaphore, &xHigherPriorityTaskWoken );

    if( pdFAIL != xResult)
    {
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}
#define PIN_DISPLAY_INT                              (BSP_IO_PORT_00_PIN_02)
#define PIN_DISPLAY_RST                              (BSP_IO_PORT_00_PIN_00)

void reset_gt911(void)
{
        R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RST, BSP_IO_LEVEL_LOW);

        R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_INT,  (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

#if   (GT_911_I2C_ADDRESS_0x14 == GT_911_I2C_ADDRESS)
        R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_INT, BSP_IO_LEVEL_HIGH);
#elif (GT_911_I2C_ADDRESS_0x5D == GT_911_I2C_ADDRESS)
        R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_INT, BSP_IO_LEVEL_LOW);
#endif

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

        R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RST, BSP_IO_LEVEL_HIGH);

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

        R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_INT, BSP_IO_LEVEL_LOW);

        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);

        R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_INT,  ((uint32_t) IOPORT_CFG_IRQ_ENABLE | (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT));
}


fsp_err_t productId(i2c_master_ctrl_t * p_api_ctrl, char *target)
{
    fsp_err_t err;

    err = rdSensorReg16_Multi(p_api_ctrl, GT911_REG_PRODUCT_ID, (uint8_t *)target, 4);

    return (err);
}

#ifdef WRITE_GT11_FW
static uint8_t calcCheckSum(uint8_t *buf, uint8_t len) {
  uint8_t ccsum = 0;

  for (uint8_t i = 0; i < len; i++)
  {
    ccsum += buf[i];
  }

  ccsum = (~ccsum) + 1;
  return ccsum;
}
#endif

fsp_err_t init_ts(i2c_master_ctrl_t * p_api_ctrl)
{
    fsp_err_t err;
    char product_id[5];

    reset_gt911();

    err = R_IIC_MASTER_SlaveAddressSet( p_api_ctrl, GT_911_I2C_ADDRESS, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS == err)
    {
        err = productId(p_api_ctrl, &product_id[0]);
        if (FSP_SUCCESS == err)
        {
            if (product_id[0] != '9')
            {
                /* Product ID should be 9xx */
                err = FSP_ERR_ASSERTION;
            }
        }
    }

    return(err);    // product id should be 911
}


fsp_err_t enable_ts(i2c_master_ctrl_t * p_api_i2c_ctrl, external_irq_ctrl_t * p_api_irq_ctrl)
{

    fsp_err_t err;
    char product_id[5];
#if (defined WRITE_GT11_FW) || (defined DUMP_GT911_REGS)
    uint8_t x = 0U;
#endif

    reset_gt911();

    err = R_IIC_MASTER_SlaveAddressSet( p_api_i2c_ctrl, GT_911_I2C_ADDRESS, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS == err)
    {
        err = productId(p_api_i2c_ctrl, &product_id[0]);
        if (FSP_SUCCESS == err)
        {
            if (product_id[0] != '9')
            {
                /* Product ID should be 9xx */
                err = FSP_ERR_ABORTED;
            }
        }
    }

#ifdef WRITE_GT11_FW
    if (FSP_SUCCESS == err)
    {
        uint8_t buf[2];

        buf[0] = calcCheckSum((uint8_t *)g911xFW, sizeof(g911xFW));  // Config_Chksum (0x80FF)
        buf[1] = 0x01;                                    // Config_Fresh (0x8100)

       /* Write the configuration registers with g911xFW[]. */
        for (x = 0U; x < sizeof (g911xFW); x++)
        {
            err = wrSensorReg16_8(p_api_i2c_ctrl, GT911_REG_CONFIG_VERSION + x, (int)g911xFW[x]);
            if (FSP_SUCCESS != err)
            {
               break;
            }
        }

        if (FSP_SUCCESS == err)
        {
            /* Write the checksum and "fresh config" registers with buf[]. */
            for (x = 0U; x < sizeof (buf); x++)
            {
                err = wrSensorReg16_8(p_api_i2c_ctrl, GT911_REG_CONFIG_CHECKSUM + x, (int)buf[0+x]);
                if (FSP_SUCCESS != err)
                {
                    break;
                }
            }
        }
    }
#endif

#ifdef DUMP_GT911_REGS
    if (FSP_SUCCESS == err)
    {
        /* Dump the configuration registers. */
         for (x = 0U; x < sizeof (g_read_config); x++)
         {
             err = rdSensorReg16_8(p_api_i2c_ctrl, GT911_REG_CONFIG_VERSION + x, (uint8_t*)&g_read_config[x]);
             if (FSP_SUCCESS != err)
             {
                 break;
             }
             printf("0x%x, ", g_read_config[x]);
         }
    }
#endif
    if (FSP_SUCCESS == err)
    {
        err = wrSensorReg16_8(p_api_i2c_ctrl, GT911_REG_COMMAND, 0x00U);
        if (FSP_SUCCESS == err)
        {

            {
                err = R_ICU_ExternalIrqEnable(p_api_irq_ctrl);
            }
        }
    }


    return err;
}
