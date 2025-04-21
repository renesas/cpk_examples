/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/***********************************************************************************************************************
 * File Name    : ov7725.c
 * Description  : Contains data structures and functions setup ov7725 camera used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#include "ov7725.h"
#include "common_utils.h"
#include "camera_i2c_api.h"

//#include "ov7725.h"
//#include "ov7725_regs.h"

/*******************************************************************************************************************//**
 * @addtogroup ceu_ep
 * @{
 **********************************************************************************************************************/
#define USE_DEBUG_BREAKPOINTS       1

static const uint8_t OV7725_default_regs[][2] = {

        /*输出窗口设置*/
        {CLKRC,     0x00}, //clock config
        {COM7,      0x00}, //VGA RGB565
        {HSTART,    0x23},   //0x3f}, //水平起始位置
        {HSIZE,     0xA0},   //0x50}, //水平尺寸
        {HSTART,    0x07},   //0x03}, //垂直起始位置
        {VSIZE,     0xf0},   //0x78}, //垂直尺寸
        {HREF,      0x00},
        {HOUTSIZE,  0xA0},   //0x50}, //输出尺寸
        {VOUTSIZE,  0xF0},   //0x78}, //输出尺寸

        /*DSP control*/
        {TGT_B,     0x7f},
        {FIXGAIN,   0x09},
        {AWB_CTRL0, 0xe0},
        {DSP_CTRL1, 0xff},
        {DSP_CTRL2, 0x00},
        {DSP_CTRL3, 0x00},
        {DSP_CTRL4, 0x00},

        /*AGC AEC AWB*/
        {COM8,      0xf0},
        {COM4,      0x41}, /*Pll AEC CONFIG*/  //PLL modify to 0x41
        {COM6,      0xc5},
        {COM9,      0x11},
        {BDBASE,    0x7F},
        {BDSTEP,   0x03},
        {AEW,       0x40},
        {AEB,       0x30},
        {VPT,       0xa1},
        {EXHCL,     0x9e},
        {AWB_CTRL3,  0xaa},
        {COM8,      0xff},

        /*matrix shapness brightness contrast*/
        {EDGE1,     0x08},
        {DNSOFF,    0x01},
        {EDGE2,     0x03},
        {EDGE3,     0x00},
        {MTX1,      0xb0},
        {MTX2,      0x9d},
        {MTX3,      0x13},
        {MTX4,      0x16},
        {MTX5,      0x7b},
        {MTX6,      0x91},
        {MTX_CTRL,  0x1e},
        {BRIGHTNESS,    0x08},
        {CONTRAST,      0x20},
        {UVADJ0,    0x81},
        {SDE,       0X06},
        {USAT,      0x65},
        {VSAT,      0x65},
        {HUECOS,    0X80},
        {HUESIN,    0X80},

        /*GAMMA config*/
        {GAM1,      0x0c},
        {GAM2,      0x16},
        {GAM3,      0x2a},
        {GAM4,      0x4e},
        {GAM5,      0x61},
        {GAM6,      0x6f},
        {GAM7,      0x7b},
        {GAM8,      0x86},
        {GAM9,      0x8e},
        {GAM10,     0x97},
        {GAM11,     0xa4},
        {GAM12,     0xaf},
        {GAM13,     0xc5},
        {GAM14,     0xd7},
        {GAM15,     0xe8},
        {SLOP,      0x20},


        {COM3,      0x50},/*Horizontal mirror image*/
                          //注：datasheet默认0x10,即改变YUV为UVY格式。但是摄像头不是芯片而是模组时，
                          //要将0X10中的1变成0，即设置YUV格式。
        /*night mode auto frame rate control*/
        {COM5,      0xf5},   /*在夜视环境下，自动降低帧率，保证低照度画面质量*/
        {0x00,      0x00},   /*END*/


};




/**********************************************************************************************************************
 * Function definitions
 **********************************************************************************************************************/

//以下是OV7725的配置
static fsp_err_t ov7725_camera_write_reg(uint8_t address, uint8_t data)
{
    fsp_err_t err = FSP_SUCCESS;

    // Set I2C slave device_address
    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV7725_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_SlaveAddressSet API FAILED ** \r\n");

    err = camera_i2c_comm_write (address, 1, &data, 1 );
    APP_ERR_RETURN(err, " ** camera_i2c_comm_write FAILED ** \r\n");

    return err;
}

static fsp_err_t ov7725_camera_read_reg(uint8_t address, uint8_t *data)
{
    fsp_err_t err = FSP_SUCCESS;

    // Set I2C slave device_address
    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV7725_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_SlaveAddressSet API FAILED ** \r\n");

    err = camera_i2c_comm_read (address, 1, data, 1 );
    APP_ERR_RETURN(err, " ** camera_i2c_comm_read FAILED ** \r\n");

    return err;
}

uint8_t ov7725_camera_read_reg_return(uint8_t address)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t data;

    // Set I2C slave device_address
    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV7725_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_SlaveAddressSet API FAILED ** \r\n");

    err = camera_i2c_comm_read (address, 1, &data, 1 );
    APP_ERR_RETURN(err, " ** camera_i2c_comm_read FAILED ** \r\n");

    return data;
}


static fsp_err_t ov7725_camera_write_array(uint8_t const * p_array)
{
    fsp_err_t err   = FSP_SUCCESS;
    uint8_t   value = RESET_VALUE;
    uint16_t i=0;
    // Set I2C slave device_address
    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV7725_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    APP_ERR_RETURN(err, " ** R_IIC_MASTER_SlaveAddressSet API FAILED ** \r\n");

//    for(uint16_t i=0; i<sizeof(OV7725_default_regs); i++){
    while ( 1 ){
        if((p_array[0+2*i]==0x00) && (p_array[1+2*i]==0x00))
        {
            break;
        }
        err = ov7725_camera_write_reg (p_array[0+2*i], p_array[1+2*i]);
//        APP_PRINT("I2C transfer count is %d \rn", i);
        APP_ERR_RETURN(err, " ** ov7725_camera_write_reg FAILED ** \r\n");

        /* Read-back data from the camera register */
        err = ov7725_camera_read_reg(p_array[0+2*i], &value);
        APP_ERR_RETURN(err, " ** ov7725_reg_read FAILED ** \r\n");

        APP_PRINT("Reg:0x%x value:0x%x target:0x%x\r\n",p_array[0+2*i], p_array[1+2*i], value );

        /* Compare data written and data read-back */
        if(value != p_array[1+2*i])
        {
            while(1){
                APP_ERR_RETURN(FSP_ERR_ASSERTION, " ** Data Write and read-back data do not match ** \r\n");
            }
        }


        i++;
//        R_BSP_SoftwareDelay(30, BSP_DELAY_UNITS_MILLISECONDS);
     }

    return err;
}


/*******************************************************************************************************************//**
 * @brief       Software reset the camera
 * @param       None
 * @retval      FSP_SUCCESS   Upon successful operation
 * @retval      Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
static fsp_err_t ov7725_software_reset (void)
{
    fsp_err_t err = FSP_SUCCESS;
//    uint8_t pid;

    err = ov7725_camera_write_reg (COM7, COM7_RESET);

    APP_ERR_RETURN(err, " ** ov7725_reg_write FAILED ** \r\n");
    R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);

    return err;
}

/*******************************************************************************************************************//**
 * @brief       Power on the camera
 * @param[in]   power_state : Power state wants to set the camera
 * @retval      None
 **********************************************************************************************************************/
static void ov7725_power (ov7725_power_t power_state)
{
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(OV7725_CAM_PWR_ON, (bsp_io_level_t)power_state);
    R_BSP_PinAccessDisable();

//    R_BSP_PinAccessEnable();
//    R_BSP_PinWrite(OV7725_CAM_RESET, (bsp_io_level_t)BSP_IO_LEVEL_HIGH);
//    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
//    R_BSP_PinWrite(OV7725_CAM_RESET, (bsp_io_level_t)BSP_IO_LEVEL_LOW);
//    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
//    R_BSP_PinWrite(OV7725_CAM_RESET, (bsp_io_level_t)BSP_IO_LEVEL_HIGH);
//    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
//    R_BSP_PinAccessDisable();
}

/*******************************************************************************************************************//**
 * @brief       Initialization the camera.
 * @param       None
 * @retval      FSP_SUCCESS    Upon successful initialization.
 * @retval      Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
fsp_err_t ov7725_open (void)
{
    fsp_err_t err = FSP_SUCCESS;

    uint8_t MIDH_ID = 0 , MIDL_ID = 0;;

    /* Initialize GPT module */
    err = R_GPT_Open(&g_timer_camera_xclk_ctrl, &g_timer_camera_xclk_cfg);
    if(err != FSP_SUCCESS){
        APP_PRINT( " ** R_GPT_Open API FAILED ** \r\n");
    }

    /* Start GPT module to provide the 24MHz clock frequency output for the camera clock source */
    err = R_GPT_Start(&g_timer_camera_xclk_ctrl);
    if(err != FSP_SUCCESS){
        APP_PRINT( " ** R_GPT_Open API FAILED ** \r\n");
    }
    /* Power on the camera */
    ov7725_power (ov7725_POWER_ON);

    /* Software reset the camera */
    err = ov7725_software_reset();
    APP_ERR_RETURN(err, " ** ov7725_software_reset FAILED ** \r\n");

    //Read OV7725 ID  -- GJ
    MIDH_ID = ov7725_camera_read_reg_return(MIDH);
    APP_PRINT("MIDH is 0x%x\r\n", MIDH_ID);
    if(MIDH_ID!=0x7F)
    {
        APP_PRINT("MIDH error!\r\n");
        while(1){;}
    }
    MIDL_ID = ov7725_camera_read_reg_return(MIDL);
    APP_PRINT("MIDL is 0x%x\r\n", MIDL_ID);
    if(MIDL_ID!=0xA2)
    {
        APP_PRINT("MIDL error!\r\n");
        while(1){;}
    }

    err = ov7725_camera_write_array(OV7725_default_regs); //OV7725_default_regs
    APP_ERR_RETURN(err, " ** ov7725_write_array FAILED ** \r\n");

    /* Delay 50ms to complete write array register */
//    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
    return err;
}



#define SCCB_WR_Reg(x,y) ov7725_camera_write_reg(x,y)
#define SCCB_RD_Reg(x) ov7725_camera_read_reg_return(x)
//设置图像输出窗口
//width:输出图像宽度,<=320
//height:输出图像高度,<=240
//mode:0，QVGA输出模式；1，VGA输出模式
//QVGA模式可视范围广但近物不是很清晰，VGA模式可视范围小近物清晰
void OV7725_Window_Set(uint16_t width,uint16_t height,uint8_t mode)
{
    uint8_t raw,temp;
    uint16_t sx,sy;

//    uint16_t i=0;
    //OV7725_default_regs

    APP_PRINT("window set\r\n");
    SCCB_WR_Reg(0x13, 0xff); //AWB on
    SCCB_WR_Reg(0x0e, 0x65);
    SCCB_WR_Reg(0x2d, 0x00);
    SCCB_WR_Reg(0x2e, 0x00);

    SCCB_WR_Reg(BRIGHTNESS, 0x06);
    SCCB_WR_Reg(SIGN_BIT, 0x06);

    SCCB_WR_Reg(0x9C,(0x30-(4-2)*4));

    SCCB_WR_Reg(0xa6, 0x06);//TSLB设置
    SCCB_WR_Reg(0x60, 0x80);//MANV,手动V值
    SCCB_WR_Reg(0x61, 0x80);//MANU,手动U值

    if(mode)
    {
        sx=(640-width)/2;
        sy=(480-height)/2;
        SCCB_WR_Reg(COM7, COM7_RES_VGA|COM7_FMT_RGB565);     //设置为VGA模式
        SCCB_WR_Reg(HSTART,0x23);   //水平起始位置
        SCCB_WR_Reg(HSIZE,0xA0);    //水平尺寸
        SCCB_WR_Reg(VSTART,0x07);   //垂直起始位置
        SCCB_WR_Reg(VSIZE,0xF0);    //垂直尺寸
        SCCB_WR_Reg(HREF,0x00);
        SCCB_WR_Reg(HOUTSIZE,0xA0); //输出尺寸
        SCCB_WR_Reg(VOUTSIZE,0xF0); //输出尺寸

    }
    else
    {
        sx=(320-width)/2;
        sy=(240-height)/2;
        SCCB_WR_Reg(COM7,0x46);     //设置为QVGA模式
        SCCB_WR_Reg(HSTART,0x3f);   //水平起始位置
        SCCB_WR_Reg(HSIZE, 0x50);   //水平尺寸
        SCCB_WR_Reg(VSTART, 0x03);   //垂直起始位置
        SCCB_WR_Reg(VSIZE, 0x78);   //垂直尺寸
        SCCB_WR_Reg(HREF,  0x00);
        SCCB_WR_Reg(HOUTSIZE,0x50); //输出尺寸
        SCCB_WR_Reg(VOUTSIZE,0x78); //输出尺寸
    }
    raw=SCCB_RD_Reg(HSTART);
    temp=raw+(sx>>2);//sx高8位存在HSTART,低2位存在HREF[5:4]
    SCCB_WR_Reg(HSTART,temp);
    SCCB_WR_Reg(HSIZE,width>>2);//width高8位存在HSIZE,低2位存在HREF[1:0]

    raw=SCCB_RD_Reg(VSTART);
    temp=raw+(sy>>1);//sy高8位存在VSTART,低1位存在HREF[6]
    SCCB_WR_Reg(VSTART,temp);
    SCCB_WR_Reg(VSIZE,height>>1);//height高8位存在VSIZE,低1位存在HREF[2]

    raw=SCCB_RD_Reg(HREF);
    temp=((sy&0x01)<<6)|((sx&0x03)<<4)|((height&0x01)<<2)|(width&0x03)|raw;
    SCCB_WR_Reg(HREF,temp);

    SCCB_WR_Reg(HOUTSIZE,width>>2);
    SCCB_WR_Reg(VOUTSIZE,height>>1);

    raw=SCCB_RD_Reg(EXHCH);
    temp = (raw|(width&0x03)|((height&0x01)<<2));
    SCCB_WR_Reg(EXHCH,temp);

    SCCB_WR_Reg(COM3,COM3_VFLIP);
//    SCCB_WR_Reg(COM7, COM7_RES_VGA|COM7_FMT_YUV);     //设置为VGA模式

    SCCB_WR_Reg(COM10, COM10_VSYNC_NEG);
    SCCB_WR_Reg(COM7, COM7_RES_VGA|COM7_FMT_RGB565|COM7_FMT_RGB);     //设置为VGA模式


//    SCCB_WR_Reg(HREF, 0x80);


}



/*******************************************************************************************************************//**
 * @} (end addtogroup ceu_ep)
 **********************************************************************************************************************/
