/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "ov5640.h"
#include "ov5640_cfg.h"
//#include "ov5640_sccb.h"
//#include "ov5640_dcmi.h"
//#include "delay.h"
#include "camera_i2c_api.h"
#include "hal_data.h"


/* OV5640模块芯片ID */
#define OV5640_CHIP_ID  0x5640

/* OV5640模块固件下载超时时间，单位：毫秒（ms） */
#define OV5640_TIMEOUT  5000

uint32_t g_out_width= 0;
uint32_t g_out_height= 0;

#define OV5640_RST(x) do{ \
    R_BSP_PinAccessEnable();  \
    R_BSP_PinWrite(OV5640_CAM_RESET, (bsp_io_level_t) x);  \
    R_BSP_PinAccessDisable(); \
}while(0)

#define OV5640_PWDN(x) do{ \
    R_BSP_PinAccessEnable();  \
    R_BSP_PinWrite(OV5640_CAM_PWR_ON, (bsp_io_level_t) x);  \
    R_BSP_PinAccessDisable();  \
}while(0)

#define delay_ms(x)  R_BSP_SoftwareDelay(x, BSP_DELAY_UNITS_MILLISECONDS)


/* OV5640寄存器块枚举 */
typedef enum
{
    OV5640_REG_BANK_DSP = 0x00, /* DSP寄存器块 */
    OV5640_REG_BANK_SENSOR,     /* Sensor寄存器块 */
} ov5640_reg_bank_t;

/* OV5640模块数据结构体 */
static struct
{
    struct {
        uint16_t width;
        uint16_t height;
    } isp_input;
    struct {
        uint16_t width;
        uint16_t height;
    } pre_scaling;
    struct {
        uint16_t width;
        uint16_t height;
    } output;
} ov5640_sta = {0};

/**
 * @brief       OV5640模块写寄存器
 * @param       reg: 寄存器地址
 *              dat: 待写入的值
 * @retval      无
 */
static void ov5640_write_reg(uint16_t reg, uint8_t dat)
{
//    sccb_3_phase_write(OV5640_SCCB_ADDR, reg, dat);
    uint8_t temp[4] = {0};
    temp[0] = dat;
    R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV5640_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);

    camera_i2c_comm_write(reg , 2, &temp[0], 1);
}

/**
 * @brief       OV5640模块读寄存器
 * @param       reg: 寄存器的地址
 * @retval      读取到的寄存器值
 */
static uint8_t ov5640_read_reg(uint16_t reg)
{
    uint8_t dat = 0;
    
//    sccb_2_phase_write(OV5640_SCCB_ADDR, reg);
//    sccb_2_phase_read(OV5640_SCCB_ADDR, &dat);

        R_IIC_MASTER_SlaveAddressSet(&g_i2c_master1_ctrl, OV5640_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
        camera_i2c_comm_read(reg, 2, &dat, 1);

    
    return dat;
}

/**
 * @brief       获取OV5640模块ISP输入窗口尺寸
 * @param       无
 * @retval      无
 */
static void ov5640_get_isp_input_size(void)
{
    uint8_t reg3800;
    uint8_t reg3801;
    uint8_t reg3802;
    uint8_t reg3803;
    uint8_t reg3804;
    uint8_t reg3805;
    uint8_t reg3806;
    uint8_t reg3807;
    uint16_t x_addr_st;
    uint16_t y_addr_st;
    uint16_t x_addr_end;
    uint16_t y_addr_end;
    
    delay_ms(100);
    
    reg3800 = ov5640_read_reg(0x3800);
    reg3801 = ov5640_read_reg(0x3801);
    reg3802 = ov5640_read_reg(0x3802);
    reg3803 = ov5640_read_reg(0x3803);
    reg3804 = ov5640_read_reg(0x3804);
    reg3805 = ov5640_read_reg(0x3805);
    reg3806 = ov5640_read_reg(0x3806);
    reg3807 = ov5640_read_reg(0x3807);
    
    x_addr_st = (uint16_t)((reg3800 & 0x0F) << 8) | reg3801;
    y_addr_st = (uint16_t)((reg3802 & 0x07) << 8) | reg3803;
    x_addr_end = (uint16_t)((reg3804 & 0x0F) << 8) | reg3805;
    y_addr_end = (uint16_t)((reg3806 & 0x07) << 8) | reg3807;
    
    ov5640_sta.isp_input.width = x_addr_end - x_addr_st;
    ov5640_sta.isp_input.height = y_addr_end - y_addr_st;
}

/**
 * @brief       获取OV5640模块预缩放窗口尺寸
 * @param       无
 * @retval      无
 */
static void ov5640_get_pre_scaling_size(void)
{
    uint8_t reg3810;
    uint8_t reg3811;
    uint8_t reg3812;
    uint8_t reg3813;
    uint16_t x_offset;
    uint16_t y_offset;
    
    delay_ms(100);
    
    reg3810 = ov5640_read_reg(0x3810);
    reg3811 = ov5640_read_reg(0x3811);
    reg3812 = ov5640_read_reg(0x3812);
    reg3813 = ov5640_read_reg(0x3813);
    
    x_offset = (uint16_t)((reg3810 & 0x0F) << 8) | reg3811;
    y_offset = (uint16_t)((reg3812 & 0x07) << 8) | reg3813;
    
    ov5640_get_isp_input_size();
    
    ov5640_sta.pre_scaling.width = ov5640_sta.isp_input.width - (x_offset << 1);
    ov5640_sta.pre_scaling.height = ov5640_sta.isp_input.height - (y_offset << 1);
}

/**
 * @brief       获取OV5640模块输出图像尺寸
 * @param       无
 * @retval      无
 */
static void ov5640_get_output_size(void)
{
    uint8_t reg3808;
    uint8_t reg3809;
    uint8_t reg380A;
    uint8_t reg380B;
    uint16_t x_output_size;
    uint16_t y_output_size;
    
    delay_ms(100);
    
    reg3808 = ov5640_read_reg(0x3808);
    reg3809 = ov5640_read_reg(0x3809);
    reg380A = ov5640_read_reg(0x380A);
    reg380B = ov5640_read_reg(0x380B);
    
    x_output_size = (uint16_t)((reg3808 & 0x0F) << 8) | reg3809;
    y_output_size = (uint16_t)((reg380A & 0x07) << 8) | reg380B;
    
//    ov5640_sta.output.width = x_output_size;
//    ov5640_sta.output.height = y_output_size;
    g_out_width = x_output_size;
    g_out_height = y_output_size;
}

/**
 * @brief       OV5640模块硬件初始化
 * @param       无
 * @retval      无
 */
static void ov5640_hw_init(void)
{
    
    OV5640_RST(0);
    OV5640_PWDN(1);

}

/**
 * @brief       OV5640模块退出掉电模式
 * @param       无
 * @retval      无
 */
static void ov5640_exit_power_down(void)
{
    OV5640_RST(0);
    delay_ms(20);
    OV5640_PWDN(0);
    delay_ms(50);
    OV5640_RST(1);
    delay_ms(20);
}

/**
 * @brief       OV5640模块硬件复位
 * @param       无
 * @retval      无
 */
static void ov5640_hw_reset(void)
{
	OV5640_RST(0);
    delay_ms(20);
    OV5640_RST(1);
    delay_ms(20);
}

/**
 * @brief       OV5640模块软件复位
 * @param       无
 * @retval      无
 */
static void ov5640_sw_reset(void)
{
    uint8_t reg3103;
    
    reg3103 = ov5640_read_reg(0x3103);
    reg3103 &= ~(0x01 << 1);
    ov5640_write_reg(0x3103, reg3103);
    ov5640_write_reg(0x3008, 0x82);
    delay_ms(300);
}

/**
 * @brief       获取OV5640模块芯片ID
 * @param       无
 * @retval      芯片ID
 */
static uint16_t ov5640_get_chip_id(void)
{
    uint16_t chip_id;
    
    chip_id = ov5640_read_reg(0x300A) << 8;
    chip_id |= ov5640_read_reg(0x300B);
    
    return chip_id;
}

/**
 * @brief       初始化OV5640寄存器配置
 * @param       无
 * @retval      无
 */
static void ov5640_init_reg(void)
{
//    uint32_t cfg_index;
//
//    for (cfg_index=0; cfg_index<sizeof(ov5640_init_cfg)/sizeof(ov5640_init_cfg[0]); cfg_index++)
//    {
//        ov5640_write_reg(ov5640_init_cfg[cfg_index].reg, ov5640_init_cfg[cfg_index].dat);
//    }

    uint16_t temp;
    uint8_t   value = 0;
    uint16_t i=0;

    ov5640_write_reg(0x3103, 0x11);
    ov5640_write_reg(0x3008, 0x82);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
        // Set I2C slave device_address
//        err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master_ctrl, OV7725_I2C_SLAVE_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
//        APP_ERR_RETURN(err, " ** R_IIC_MASTER_SlaveAddressSet API FAILED ** \r\n");

        while ( 1 ){
            if((default_regs[i][0]==0x00) && (default_regs[i][1]==0x00) && (default_regs[i][2]==0x00))
            {
                break;
            }
            temp = (default_regs[i][0]<<8) | default_regs[i][1];
            ov5640_write_reg(temp, default_regs[i][2]);

            i++;
         }

//            for(uint16_t i=0; i<sizeof(ov5640_init_cfg); i++){
//
//                ov5640_write_reg(ov5640_init_cfg[i].reg, ov5640_init_cfg[i].dat);
//
//             }
    
}

/**
 * @brief       初始化OV5640模块
 * @param       无
 * @retval      OV5640_EOK   : OV5640模块初始化成功
 *              OV5640_ERROR : 通讯出错，OV5640模块初始化失败
 *              OV5640_ENOMEM: 内存不足，OV5640模块初始化失败
 */
uint8_t ov5640_init(void)
{
    uint16_t chip_id;
    
    /* Initialize GPT module */
//    R_GPT_Open(&g_timer_camera_xclk_ctrl, &g_timer_camera_xclk_cfg);
//
//    /* Start GPT module to provide the 24MHz clock frequency output for the camera clock source */
//    R_GPT_Start(&g_timer_camera_xclk_ctrl);

    ov5640_hw_init();               /* OV5640模块硬件初始化 */
    ov5640_exit_power_down();       /* OV5640模块退出掉电模式 */
//    ov5640_hw_reset();              /* OV5640模块硬件复位 */
//    sccb_init();                        /* OV5640 SCCB接口初始化 */


    ov5640_sw_reset();              /* OV5640模块软件复位 */
    delay_ms(200);
    chip_id = ov5640_get_chip_id(); /* 获取芯片ID */
    if (chip_id != OV5640_CHIP_ID)
    {
        while(1){
            ;
        } //for debug
//        return OV5640_ERROR;
    }
    
//    OV5640_Init();

    ov5640_init_reg();              /* 初始化OV5640寄存器配置 */
    delay_ms(300);
    delay_ms(200);
    return OV5640_EOK;
}

/**
 * @brief       初始化OV5640模块自动对焦
 * @param       无
 * @retval      OV5640_EOK     : OV5640模块自动对焦初始化成功
 *              OV5640_ETIMEOUT: OV5640模块下载固件超时
 */
uint8_t ov5640_auto_focus_init(void)
{
    uint32_t fw_index;
    uint16_t addr_index;
    uint8_t reg3029 = 0;
    uint16_t timeout = 0;
    
    ov5640_write_reg(0x3000, 0x20);
    
    for (addr_index=OV5640_FW_DOWNLOAD_ADDR, fw_index=0; fw_index<sizeof(ov5640_auto_focus_firmware);addr_index++, fw_index++)
    {
        ov5640_write_reg(addr_index, ov5640_auto_focus_firmware[fw_index]);
    }
    
    ov5640_write_reg(0x3022, 0x00);
    ov5640_write_reg(0x3023, 0x00);
    ov5640_write_reg(0x3024, 0x00);
    ov5640_write_reg(0x3025, 0x00);
    ov5640_write_reg(0x3026, 0x00);
    ov5640_write_reg(0x3027, 0x00);
    ov5640_write_reg(0x3028, 0x00);
    ov5640_write_reg(0x3029, 0x7F);
    ov5640_write_reg(0x3000, 0x00);
    
    while ((reg3029 != 0x70) && (timeout < OV5640_TIMEOUT))
    {
        delay_ms(1);
        reg3029 = ov5640_read_reg(0x3029);
        timeout++;
    }
    
    if (reg3029 != 0x70)
    {
        return OV5640_ETIMEOUT;
    }
    
    return OV5640_EOK;
}

/**
 * @brief       OV5640模块自动对焦一次
 * @param       无
 * @retval      OV5640_EOK     : OV5640模块自动对焦一次成功
 *              OV5640_ETIMEOUT: OV5640模块自动对焦一次超时
 */
uint8_t ov5640_auto_focus_once(void)
{
    uint8_t reg3029 = 0;
    uint16_t timeout = 0;
    
    ov5640_write_reg(0x3022, 0x03);
    
    while ((reg3029 != 0x10) && (timeout < OV5640_TIMEOUT))
    {
        delay_ms(1);
        reg3029 = ov5640_read_reg(0x3029);
        timeout++;
    }
    
    if (reg3029 != 0x10)
    {
        return OV5640_ETIMEOUT;
    }
    
    return OV5640_EOK;
}

/**
 * @brief       OV5640模块持续自动对焦
 * @param       无
 * @retval      OV5640_EOK     : OV5640模块持续自动对焦成功
 *              OV5640_ETIMEOUT: OV5640模块持续自动对焦超时
 */
uint8_t ov5640_auto_focus_continuance(void)
{
    uint8_t reg3023 = ~0;
    uint16_t timeout = 0;
    
    ov5640_write_reg(0x3023, 0x01);
    ov5640_write_reg(0x3022, 0x08);
    
    while ((reg3023 != 0x00) && (timeout < OV5640_TIMEOUT))
    {
        delay_ms(1);
        reg3023 = ov5640_read_reg(0x3023);
        timeout++;
    }
    
    if (reg3023 != 0x00)
    {
        return OV5640_ETIMEOUT;
    }
    
    reg3023 = ~0;
    timeout = 0;
    
    ov5640_write_reg(0x3023, 0x01);
    ov5640_write_reg(0x3022, 0x04);
    
    while ((reg3023 != 0x00) && (timeout < OV5640_TIMEOUT))
    {
        delay_ms(1);
        reg3023 = ov5640_read_reg(0x3023);
        timeout++;
    }
    
    if (reg3023 != 0x00)
    {
        return OV5640_ETIMEOUT;
    }
    
    return OV5640_EOK;
}

/**
 * @brief       开启OV5640模块闪光灯
 * @param       无
 * @retval      无
 */
void ov5640_led_on(void)
{
    ov5640_write_reg(0x3016, 0x02);
    ov5640_write_reg(0x301C, 0x02);
    ov5640_write_reg(0x3019, 0x02);
}

/**
 * @brief       关闭OV5640模块闪光灯
 * @param       无
 * @retval      无
 */
void ov5640_led_off(void)
{
    ov5640_write_reg(0x3016, 0x02);
    ov5640_write_reg(0x301C, 0x02);
    ov5640_write_reg(0x3019, 0x00);
}


/**
 * @brief       设置OV5640模块灯光模式
 * @param       mode: OV5640_LIGHT_MODE_ADVANCED_AWB : Advanced AWB
 *                    OV5640_LIGHT_MODE_SIMPLE_AWB   : Simple AWB
 *                    OV5640_LIGHT_MODE_MANUAL_DAY   : Manual day
 *                    OV5640_LIGHT_MODE_MANUAL_A     : Manual A
 *                    OV5640_LIGHT_MODE_MANUAL_CWF   : Manual cwf
 *                    OV5640_LIGHT_MODE_MANUAL_CLOUDY: Manual cloudy
 * @retval      OV5640_EOK   : 设置OV5640模块灯光模式成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_light_mode(ov5640_light_mode_t mode)
{
    switch (mode)
    {
        case OV5640_LIGHT_MODE_ADVANCED_AWB:
        {
            ov5640_write_reg(0x3406, 0x00);
            ov5640_write_reg(0x5192, 0x04);
            ov5640_write_reg(0x5191, 0xF8);
            ov5640_write_reg(0x5193, 0x70);
            ov5640_write_reg(0x5194, 0xF0);
            ov5640_write_reg(0x5195, 0xF0);
            ov5640_write_reg(0x518D, 0x3D);
            ov5640_write_reg(0x518F, 0x54);
            ov5640_write_reg(0x518E, 0x3D);
            ov5640_write_reg(0x5190, 0x54);
            ov5640_write_reg(0x518B, 0xA8);
            ov5640_write_reg(0x518C, 0xA8);
            ov5640_write_reg(0x5187, 0x18);
            ov5640_write_reg(0x5188, 0x18);
            ov5640_write_reg(0x5189, 0x6E);
            ov5640_write_reg(0x518A, 0x68);
            ov5640_write_reg(0x5186, 0x1C);
            ov5640_write_reg(0x5181, 0x50);
            ov5640_write_reg(0x5184, 0x25);
            ov5640_write_reg(0x5182, 0x11);
            ov5640_write_reg(0x5183, 0x14);
            ov5640_write_reg(0x5184, 0x25);
            ov5640_write_reg(0x5185, 0x24);
            break;
        }
        case OV5640_LIGHT_MODE_SIMPLE_AWB:
        {
            ov5640_write_reg(0x3406, 0x00);
            ov5640_write_reg(0x5183, 0x94);
            ov5640_write_reg(0x5191, 0xFF);
            ov5640_write_reg(0x5192, 0x00);
            break;
        }
        case OV5640_LIGHT_MODE_MANUAL_DAY:
        {
            ov5640_write_reg(0x3406, 0x01);
            ov5640_write_reg(0x3400, 0x06);
            ov5640_write_reg(0x3401, 0x1C);
            ov5640_write_reg(0x3402, 0x04);
            ov5640_write_reg(0x3403, 0x00);
            ov5640_write_reg(0x3404, 0x04);
            ov5640_write_reg(0x3405, 0xF3);
            break;
        }
        case OV5640_LIGHT_MODE_MANUAL_A:
        {
            ov5640_write_reg(0x3406, 0x01);
            ov5640_write_reg(0x3400, 0x04);
            ov5640_write_reg(0x3401, 0x10);
            ov5640_write_reg(0x3402, 0x04);
            ov5640_write_reg(0x3403, 0x00);
            ov5640_write_reg(0x3404, 0x08);
            ov5640_write_reg(0x3405, 0xB6);
            break;
        }
        case OV5640_LIGHT_MODE_MANUAL_CWF:
        {
            ov5640_write_reg(0x3406, 0x01);
            ov5640_write_reg(0x3400, 0x05);
            ov5640_write_reg(0x3401, 0x48);
            ov5640_write_reg(0x3402, 0x04);
            ov5640_write_reg(0x3403, 0x00);
            ov5640_write_reg(0x3404, 0x07);
            ov5640_write_reg(0x3405, 0xCF);
            break;
        }
        case OV5640_LIGHT_MODE_MANUAL_CLOUDY:
        {
            ov5640_write_reg(0x3406, 0x01);
            ov5640_write_reg(0x3400, 0x06);
            ov5640_write_reg(0x3401, 0x48);
            ov5640_write_reg(0x3402, 0x04);
            ov5640_write_reg(0x3403, 0x00);
            ov5640_write_reg(0x3404, 0x04);
            ov5640_write_reg(0x3405, 0xD3);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块色彩饱和度
 * @param       saturation: OV5640_COLOR_SATURATION_0: +4
 *                          OV5640_COLOR_SATURATION_1: +3
 *                          OV5640_COLOR_SATURATION_2: +2
 *                          OV5640_COLOR_SATURATION_3: +1
 *                          OV5640_COLOR_SATURATION_4: 0
 *                          OV5640_COLOR_SATURATION_5: -1
 *                          OV5640_COLOR_SATURATION_6: -2
 *                          OV5640_COLOR_SATURATION_7: -3
 *                          OV5640_COLOR_SATURATION_8: -4
 * @retval      OV5640_EOK   : 设置OV5640模块色彩饱和度成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_color_saturation(ov5640_color_saturation_t saturation)
{
    switch (saturation)
    {
        case OV5640_COLOR_SATURATION_0:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x80);
            ov5640_write_reg(0x5584, 0x80);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_1:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x70);
            ov5640_write_reg(0x5584, 0x70);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_2:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x60);
            ov5640_write_reg(0x5584, 0x60);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_3:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x50);
            ov5640_write_reg(0x5584, 0x50);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_4:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x40);
            ov5640_write_reg(0x5584, 0x40);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_5:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x30);
            ov5640_write_reg(0x5584, 0x30);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_6:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x20);
            ov5640_write_reg(0x5584, 0x20);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_7:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x10);
            ov5640_write_reg(0x5584, 0x10);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_COLOR_SATURATION_8:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5583, 0x00);
            ov5640_write_reg(0x5584, 0x00);
            ov5640_write_reg(0x5580, 0x02);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块亮度
 * @param       brightness: OV5640_BRIGHTNESS_0: +4
 *                          OV5640_BRIGHTNESS_1: +3
 *                          OV5640_BRIGHTNESS_2: +2
 *                          OV5640_BRIGHTNESS_3: +1
 *                          OV5640_BRIGHTNESS_4: 0
 *                          OV5640_BRIGHTNESS_5: -1
 *                          OV5640_BRIGHTNESS_6: -2
 *                          OV5640_BRIGHTNESS_7: -3
 *                          OV5640_BRIGHTNESS_8: -4
 * @retval      OV5640_EOK   : 设置OV5640模块亮度成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_brightness(ov5640_brightness_t brightness)
{
    switch (brightness)
    {
        case OV5640_BRIGHTNESS_0:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x40);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_BRIGHTNESS_1:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x30);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_BRIGHTNESS_2:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x20);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_BRIGHTNESS_3:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x10);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_BRIGHTNESS_4:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x00);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_BRIGHTNESS_5:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x10);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x09);
            break;
        }
        case OV5640_BRIGHTNESS_6:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x20);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x09);
            break;
        }
        case OV5640_BRIGHTNESS_7:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x30);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x09);
            break;
        }
        case OV5640_BRIGHTNESS_8:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5587, 0x40);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5588, 0x09);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块对比度
 * @param       contrast: OV5640_CONTRAST_0: +4
 *                        OV5640_CONTRAST_1: +3
 *                        OV5640_CONTRAST_2: +2
 *                        OV5640_CONTRAST_3: +1
 *                        OV5640_CONTRAST_4: 0
 *                        OV5640_CONTRAST_5: -1
 *                        OV5640_CONTRAST_6: -2
 *                        OV5640_CONTRAST_7: -3
 *                        OV5640_CONTRAST_8: -4
 * @retval      OV5640_EOK   : 设置OV5640模块对比度成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_contrast(ov5640_contrast_t contrast)
{
    switch (contrast)
    {
        case OV5640_CONTRAST_0:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x30);
            ov5640_write_reg(0x5585, 0x30);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_1:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x2C);
            ov5640_write_reg(0x5585, 0x2C);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_2:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x28);
            ov5640_write_reg(0x5585, 0x28);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_3:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x24);
            ov5640_write_reg(0x5585, 0x24);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_4:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x20);
            ov5640_write_reg(0x5585, 0x20);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_5:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x1C);
            ov5640_write_reg(0x5585, 0x1C);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_6:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x18);
            ov5640_write_reg(0x5585, 0x18);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_7:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x14);
            ov5640_write_reg(0x5585, 0x14);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        case OV5640_CONTRAST_8:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x04);
            ov5640_write_reg(0x5586, 0x10);
            ov5640_write_reg(0x5585, 0x10);
            ov5640_write_reg(0x5588, 0x41);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块色相
 * @param       contrast: OV5640_HUE_0 : -180 degree
 *                        OV5640_HUE_1 : -150 degree
 *                        OV5640_HUE_2 : -120 degree
 *                        OV5640_HUE_3 : -90 degree
 *                        OV5640_HUE_4 : -60 degree
 *                        OV5640_HUE_5 : -30 degree
 *                        OV5640_HUE_6 : 0 degree
 *                        OV5640_HUE_7 : +30 degree
 *                        OV5640_HUE_8 : +60 degree
 *                        OV5640_HUE_9 : +90 degree
 *                        OV5640_HUE_10: +120 degree
 *                        OV5640_HUE_11: +150 degree
 * @retval      OV5640_EOK   : 设置OV5640模块色相成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_hue(ov5640_hue_t hue)
{
    switch (hue)
    {
        case OV5640_HUE_0:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x80);
            ov5640_write_reg(0x5582, 0x00);
            ov5640_write_reg(0x5588, 0x32);
            break;
        }
        case OV5640_HUE_1:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x6F);
            ov5640_write_reg(0x5582, 0x40);
            ov5640_write_reg(0x5588, 0x32);
            break;
        }
        case OV5640_HUE_2:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x40);
            ov5640_write_reg(0x5582, 0x6F);
            ov5640_write_reg(0x5588, 0x32);
            break;
        }
        case OV5640_HUE_3:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x00);
            ov5640_write_reg(0x5582, 0x80);
            ov5640_write_reg(0x5588, 0x02);
            break;
        }
        case OV5640_HUE_4:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x40);
            ov5640_write_reg(0x5582, 0x6F);
            ov5640_write_reg(0x5588, 0x02);
            break;
        }
        case OV5640_HUE_5:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x6F);
            ov5640_write_reg(0x5582, 0x40);
            ov5640_write_reg(0x5588, 0x02);
            break;
        }
        case OV5640_HUE_6:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x80);
            ov5640_write_reg(0x5582, 0x00);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_HUE_7:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x6F);
            ov5640_write_reg(0x5582, 0x40);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_HUE_8:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x40);
            ov5640_write_reg(0x5582, 0x6F);
            ov5640_write_reg(0x5588, 0x01);
            break;
        }
        case OV5640_HUE_9:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x00);
            ov5640_write_reg(0x5582, 0x80);
            ov5640_write_reg(0x5588, 0x31);
            break;
        }
        case OV5640_HUE_10:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x40);
            ov5640_write_reg(0x5582, 0x6F);
            ov5640_write_reg(0x5588, 0x31);
            break;
        }
        case OV5640_HUE_11:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x01);
            ov5640_write_reg(0x5581, 0x6F);
            ov5640_write_reg(0x5582, 0x40);
            ov5640_write_reg(0x5588, 0x31);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块特殊效果
 * @param       contrast: OV5640_SPECIAL_EFFECT_NORMAL  : Normal
 *                        OV5640_SPECIAL_EFFECT_BW      : B&W
 *                        OV5640_SPECIAL_EFFECT_BLUISH  : Bluish
 *                        OV5640_SPECIAL_EFFECT_SEPIA   : Sepia
 *                        OV5640_SPECIAL_EFFECT_REDDISH : Reddish
 *                        OV5640_SPECIAL_EFFECT_GREENISH: Greenish
 *                        OV5640_SPECIAL_EFFECT_NEGATIVE: Negative
 * @retval      OV5640_EOK   : 设置OV5640模块特殊效果成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_special_effect(ov5640_special_effect_t effect)
{
    switch (effect)
    {
        case OV5640_SPECIAL_EFFECT_NORMAL:
        {
            ov5640_write_reg(0x5001, 0x7F);
            ov5640_write_reg(0x5580, 0x00);
            break;
        }
        case OV5640_SPECIAL_EFFECT_BW:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x18);
            ov5640_write_reg(0x5583, 0x80);
            ov5640_write_reg(0x5584, 0x80);
            break;
        }
        case OV5640_SPECIAL_EFFECT_BLUISH:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x18);
            ov5640_write_reg(0x5583, 0xA0);
            ov5640_write_reg(0x5584, 0x40);
            break;
        }
        case OV5640_SPECIAL_EFFECT_SEPIA:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x18);
            ov5640_write_reg(0x5583, 0x40);
            ov5640_write_reg(0x5584, 0xA0);
            break;
        }
        case OV5640_SPECIAL_EFFECT_REDDISH:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x18);
            ov5640_write_reg(0x5583, 0x80);
            ov5640_write_reg(0x5584, 0xC0);
            break;
        }
        case OV5640_SPECIAL_EFFECT_GREENISH:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x18);
            ov5640_write_reg(0x5583, 0x60);
            ov5640_write_reg(0x5584, 0x60);
            break;
        }
        case OV5640_SPECIAL_EFFECT_NEGATIVE:
        {
            ov5640_write_reg(0x5001, 0xFF);
            ov5640_write_reg(0x5580, 0x40);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块曝光度
 * @param       contrast: OV5640_EXPOSURE_LEVEL_0 : -1.7EV
 *                        OV5640_EXPOSURE_LEVEL_1 : -1.3EV
 *                        OV5640_EXPOSURE_LEVEL_2 : -1.0EV
 *                        OV5640_EXPOSURE_LEVEL_3 : -0.7EV
 *                        OV5640_EXPOSURE_LEVEL_4 : -0.3EV
 *                        OV5640_EXPOSURE_LEVEL_5 : default
 *                        OV5640_EXPOSURE_LEVEL_6 : 0.3EV
 *                        OV5640_EXPOSURE_LEVEL_7 : 0.7EV
 *                        OV5640_EXPOSURE_LEVEL_8 : 1.0EV
 *                        OV5640_EXPOSURE_LEVEL_9 : 1.3EV
 *                        OV5640_EXPOSURE_LEVEL_10: 1.7EV
 * @retval      OV5640_EOK   : 设置OV5640模块曝光度成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_exposure_level(ov5640_exposure_level_t level)
{
    switch (level)
    {
        case OV5640_EXPOSURE_LEVEL_0:
        {
            ov5640_write_reg(0x3A0F, 0x10);
            ov5640_write_reg(0x3A10, 0x08);
            ov5640_write_reg(0x3A1B, 0x10);
            ov5640_write_reg(0x3A1E, 0x08);
            ov5640_write_reg(0x3A11, 0x20);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_1:
        {
            ov5640_write_reg(0x3A0F, 0x18);
            ov5640_write_reg(0x3A10, 0x10);
            ov5640_write_reg(0x3A1B, 0x18);
            ov5640_write_reg(0x3A1E, 0x10);
            ov5640_write_reg(0x3A11, 0x30);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_2:
        {
            ov5640_write_reg(0x3A0F, 0x20);
            ov5640_write_reg(0x3A10, 0x18);
            ov5640_write_reg(0x3A11, 0x41);
            ov5640_write_reg(0x3A1B, 0x20);
            ov5640_write_reg(0x3A1E, 0x18);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_3:
        {
            ov5640_write_reg(0x3A0F, 0x28);
            ov5640_write_reg(0x3A10, 0x20);
            ov5640_write_reg(0x3A11, 0x51);
            ov5640_write_reg(0x3A1B, 0x28);
            ov5640_write_reg(0x3A1E, 0x20);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_4:
        {
            ov5640_write_reg(0x3A0F, 0x30);
            ov5640_write_reg(0x3A10, 0x28);
            ov5640_write_reg(0x3A11, 0x61);
            ov5640_write_reg(0x3A1B, 0x30);
            ov5640_write_reg(0x3A1E, 0x28);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_5:
        {
            ov5640_write_reg(0x3A0F, 0x38);
            ov5640_write_reg(0x3A10, 0x30);
            ov5640_write_reg(0x3A11, 0x61);
            ov5640_write_reg(0x3A1B, 0x38);
            ov5640_write_reg(0x3A1E, 0x30);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_6:
        {
            ov5640_write_reg(0x3A0F, 0x40);
            ov5640_write_reg(0x3A10, 0x38);
            ov5640_write_reg(0x3A11, 0x71);
            ov5640_write_reg(0x3A1B, 0x40);
            ov5640_write_reg(0x3A1E, 0x38);
            ov5640_write_reg(0x3A1F, 0x10);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_7:
        {
            ov5640_write_reg(0x3A0F, 0x48);
            ov5640_write_reg(0x3A10, 0x40);
            ov5640_write_reg(0x3A11, 0x80);
            ov5640_write_reg(0x3A1B, 0x48);
            ov5640_write_reg(0x3A1E, 0x40);
            ov5640_write_reg(0x3A1F, 0x20);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_8:
        {
            ov5640_write_reg(0x3A0F, 0x50);
            ov5640_write_reg(0x3A10, 0x48);
            ov5640_write_reg(0x3A11, 0x90);
            ov5640_write_reg(0x3A1B, 0x50);
            ov5640_write_reg(0x3A1E, 0x48);
            ov5640_write_reg(0x3A1F, 0x20);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_9:
        {
            ov5640_write_reg(0x3A0F, 0x58);
            ov5640_write_reg(0x3A10, 0x50);
            ov5640_write_reg(0x3A11, 0x91);
            ov5640_write_reg(0x3A1B, 0x58);
            ov5640_write_reg(0x3A1E, 0x50);
            ov5640_write_reg(0x3A1F, 0x20);
            break;
        }
        case OV5640_EXPOSURE_LEVEL_10:
        {
            ov5640_write_reg(0x3A0F, 0x60);
            ov5640_write_reg(0x3A10, 0x58);
            ov5640_write_reg(0x3A11, 0xA0);
            ov5640_write_reg(0x3A1B, 0x60);
            ov5640_write_reg(0x3A1E, 0x58);
            ov5640_write_reg(0x3A1F, 0x20);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块锐度
 * @param       contrast: OV5640_SHARPNESS_OFF  : Sharpness OFF
 *                        OV5640_SHARPNESS_1    : Sharpness 1
 *                        OV5640_SHARPNESS_2    : Sharpness 2
 *                        OV5640_SHARPNESS_3    : Sharpness 3
 *                        OV5640_SHARPNESS_4    : Sharpness 4
 *                        OV5640_SHARPNESS_5    : Sharpness 5
 *                        OV5640_SHARPNESS_6    : Sharpness 6
 *                        OV5640_SHARPNESS_7    : Sharpness 7
 *                        OV5640_SHARPNESS_8    : Sharpness 8
 *                        OV5640_SHARPNESS_AUTO : Sharpness Auto
 * @retval      OV5640_EOK   : 设置OV5640模块锐度成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_sharpness_level(ov5640_sharpness_t sharpness)
{
    switch (sharpness)
    {
        case OV5640_SHARPNESS_OFF:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x00);
            break;
        }
        case OV5640_SHARPNESS_1:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x02);
            break;
        }
        case OV5640_SHARPNESS_2:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x04);
            break;
        }
        case OV5640_SHARPNESS_3:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x08);
            break;
        }
        case OV5640_SHARPNESS_4:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x0C);
            break;
        }
        case OV5640_SHARPNESS_5:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x10);
            break;
        }
        case OV5640_SHARPNESS_6:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x14);
            break;
        }
        case OV5640_SHARPNESS_7:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x18);
            break;
        }
        case OV5640_SHARPNESS_8:
        {
            ov5640_write_reg(0x5308, 0x65);
            ov5640_write_reg(0x5302, 0x20);
            break;
        }
        case OV5640_SHARPNESS_AUTO:
        {
            ov5640_write_reg(0x5308, 0x25);
            ov5640_write_reg(0x5300, 0x08);
            ov5640_write_reg(0x5301, 0x30);
            ov5640_write_reg(0x5302, 0x10);
            ov5640_write_reg(0x5303, 0x00);
            ov5640_write_reg(0x5309, 0x08);
            ov5640_write_reg(0x530A, 0x30);
            ov5640_write_reg(0x530B, 0x04);
            ov5640_write_reg(0x530C, 0x06);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块镜像/翻转
 * @param       contrast: OV5640_MIRROR_FLIP_0: MIRROR
 *                        OV5640_MIRROR_FLIP_1: FLIP
 *                        OV5640_MIRROR_FLIP_2: MIRROR & FLIP
 *                        OV5640_MIRROR_FLIP_3: Normal
 * @retval      OV5640_EOK   : 设置OV5640模块镜像/翻转成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_mirror_flip(ov5640_mirror_flip_t mirror_flip)
{
    uint8_t reg3820;
    uint8_t reg3821;
    
    switch (mirror_flip)
    {
        case OV5640_MIRROR_FLIP_0:
        {
            reg3820 = ov5640_read_reg(0x3820);
            reg3820 = reg3820 & 0xF9;
            reg3820 = reg3820 | 0x00;
            ov5640_write_reg(0x3820, reg3820);
            reg3821 = ov5640_read_reg(0x3821);
            reg3821 = reg3821 & 0xF9;
            reg3821 = reg3821 | 0x06;
            ov5640_write_reg(0x3821, reg3821);
            break;
        }
        case OV5640_MIRROR_FLIP_1:
        {
            reg3820 = ov5640_read_reg(0x3820);
            reg3820 = reg3820 & 0xF9;
            reg3820 = reg3820 | 0x06;
            ov5640_write_reg(0x3820, reg3820);
            reg3821 = ov5640_read_reg(0x3821);
            reg3821 = reg3821 & 0xF9;
            reg3821 = reg3821 | 0x00;
            ov5640_write_reg(0x3821, reg3821);
            break;
        }
        case OV5640_MIRROR_FLIP_2:
        {
            reg3820 = ov5640_read_reg(0x3820);
            reg3820 = reg3820 & 0xF9;
            reg3820 = reg3820 | 0x06;
            ov5640_write_reg(0x3820, reg3820);
            reg3821 = ov5640_read_reg(0x3821);
            reg3821 = reg3821 & 0xF9;
            reg3821 = reg3821 | 0x06;
            ov5640_write_reg(0x3821, reg3821);
            break;
        }
        case OV5640_MIRROR_FLIP_3:
        {
            reg3820 = ov5640_read_reg(0x3820);
            reg3820 = reg3820 & 0xF9;
            reg3820 = reg3820 | 0x00;
            ov5640_write_reg(0x3820, reg3820);
            reg3821 = ov5640_read_reg(0x3821);
            reg3821 = reg3821 & 0xF9;
            reg3821 = reg3821 | 0x00;
            ov5640_write_reg(0x3821, reg3821);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块测试图案
 * @param       contrast: OV5640_TEST_PATTERN_OFF         : OFF
 *                        OV5640_TEST_PATTERN_COLOR_BAR   : Color bar
 *                        OV5640_TEST_PATTERN_COLOR_SQUARE: Color square
 * @retval      OV5640_EOK   : 设置OV5640模块测试图案成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_test_pattern(ov5640_test_pattern_t pattern)
{
    switch (pattern)
    {
        case OV5640_TEST_PATTERN_OFF:
        {
            ov5640_write_reg(0x503D, 0x00);
            ov5640_write_reg(0x4741, 0x00);
            break;
        }
        case OV5640_TEST_PATTERN_COLOR_BAR:
        {
            ov5640_write_reg(0x503D, 0x80);
            ov5640_write_reg(0x4741, 0x00);
            break;
        }
        case OV5640_TEST_PATTERN_COLOR_SQUARE:
        {
            ov5640_write_reg(0x503D, 0x82);
            ov5640_write_reg(0x4741, 0x00);
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块输出图像格式
 * @param       mode: OV5640_OUTPUT_FORMAT_RGB565: RGB565
 *                    OV5640_OUTPUT_FORMAT_JPEG  : JPEG
 * @retval      OV5640_EOK   : 设置OV5640模块输出图像格式成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_output_format(ov5640_output_format_t format)
{
    uint32_t cfg_index;
    
    switch (format)
    {
        case OV5640_OUTPUT_FORMAT_RGB565:
        {
            for (cfg_index=0; cfg_index<sizeof(ov5640_rgb565_cfg)/sizeof(ov5640_rgb565_cfg[0]); cfg_index++)
            {
                ov5640_write_reg(ov5640_rgb565_cfg[cfg_index].reg, ov5640_rgb565_cfg[cfg_index].dat);
            }
//            ov5640_write_reg(0x4300 , 0x6F);
//            ov5640_write_reg(0x501f , 0x1);
//            ov5640_write_reg(0x3821 , 0x1);
//            ov5640_write_reg(0x3002 , 0x1c);
//            ov5640_write_reg(0x3006 , 0xc3);
            break;
        }
        case OV5640_OUTPUT_FORMAT_JPEG:
        {
            for (cfg_index=0; cfg_index<sizeof(ov5640_jpeg_cfg)/sizeof(ov5640_jpeg_cfg[0]); cfg_index++)
            {
                ov5640_write_reg(ov5640_jpeg_cfg[cfg_index].reg, ov5640_jpeg_cfg[cfg_index].dat);
            }
            break;
        }
        default:
        {
            return OV5640_EINVAL;
        }
    }
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块ISP输入窗口尺寸
 * @note        OV5640模块ISP输入窗口的最大尺寸为0x0A3F*0x079F
 * @param       x     : ISP输入窗口起始X坐标
 *              y     : ISP输入窗口起始Y坐标
 *              width : ISP输入窗口宽度
 *              height: ISP输入窗口高度
 * @retval      OV5640_EOK   : 设置OV5640模块ISP输入窗口尺寸成功
 *              OV5640_EINVAL: 传入参数错误
 */
uint8_t ov5640_set_isp_input_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint8_t reg3800;
    uint8_t reg3801;
    uint8_t reg3802;
    uint8_t reg3803;
    uint8_t reg3804;
    uint8_t reg3805;
    uint8_t reg3806;
    uint8_t reg3807;
    uint16_t x_end;
    uint16_t y_end;
    
    x_end = x + width;
    y_end = y + height;
    
    if ((x_end > OV5640_ISP_INPUT_WIDTH_MAX) || (y_end > OV5640_ISP_INPUT_HEIGHT_MAX))
    {
        return OV5640_EINVAL;
    }
    
    reg3800 = ov5640_read_reg(0x3800);
    reg3802 = ov5640_read_reg(0x3802);
    reg3804 = ov5640_read_reg(0x3804);
    reg3806 = ov5640_read_reg(0x3806);
    
    reg3800 &= ~0x0F;
    reg3800 |= (uint8_t)(x >> 8) & 0x0F;
    reg3801 = (uint8_t)x & 0xFF;
    reg3802 &= ~0x0F;
    reg3802 |= (uint8_t)(y >> 8) & 0x0F;
    reg3803 = (uint8_t)y & 0xFF;
    reg3804 &= ~0x0F;
    reg3804 |= (uint8_t)(x_end >> 8) & 0x0F;
    reg3805 = (uint8_t)x_end & 0xFF;
    reg3806 &= ~0x07;
    reg3806 |= (uint8_t)(y_end >> 8) & 0x07;
    reg3807 = (uint8_t)y_end & 0xFF;
    
    ov5640_write_reg(0x3212, 0x03);
    ov5640_write_reg(0x3800, reg3800);
    ov5640_write_reg(0x3801, reg3801);
    ov5640_write_reg(0x3802, reg3802);
    ov5640_write_reg(0x3803, reg3803);
    ov5640_write_reg(0x3804, reg3804);
    ov5640_write_reg(0x3805, reg3805);
    ov5640_write_reg(0x3806, reg3806);
    ov5640_write_reg(0x3807, reg3807);
    ov5640_write_reg(0x3212, 0x13);
    ov5640_write_reg(0x3212, 0xA3);
    
    ov5640_get_isp_input_size();
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块预缩放窗口偏移
 * @note        OV5640模块预缩放窗口尺寸必须小于ISP输入窗口尺寸
 * @param       x_offset: 预缩放窗口X偏移
 *              y_offset: 预缩放窗口Y偏移
 * @retval      OV5640_EOK   : 设置OV5640模块预缩放窗口偏移成功
 */
uint8_t ov5640_set_pre_scaling_window(uint16_t x_offset, uint16_t y_offset)
{
    uint8_t reg3810;
    uint8_t reg3811;
    uint8_t reg3812;
    uint8_t reg3813;
    
    reg3810 = (uint8_t)(x_offset >> 8) & 0x0F;
    reg3811 = (uint8_t)x_offset & 0xFF;
    reg3812 = (uint8_t)(y_offset >> 8) & 0x07;
    reg3813 = (uint8_t)y_offset & 0xFF;
    
    ov5640_write_reg(0x3212, 0x03);
    ov5640_write_reg(0x3810, reg3810);
    ov5640_write_reg(0x3811, reg3811);
    ov5640_write_reg(0x3812, reg3812);
    ov5640_write_reg(0x3813, reg3813);
    ov5640_write_reg(0x3212, 0x13);
    ov5640_write_reg(0x3212, 0xA3);
    
    ov5640_get_pre_scaling_size();
    
    return OV5640_EOK;
}

/**
 * @brief       设置OV5640模块输出图像尺寸
 * @param       width : 实际输出图像的宽度（可能被缩放）
 *              height: 实际输出图像的高度（可能被缩放）
 * @retval      OV5640_EOK   : 设置OV5640模块输出图像窗口成功
 */
uint8_t ov5640_set_output_size(uint16_t width, uint16_t height)
{
    uint8_t reg3808;
    uint8_t reg3809;
    uint8_t reg380A;
    uint8_t reg380B;
    
    reg3808 = ov5640_read_reg(0x3808);
    reg380A = ov5640_read_reg(0x380A);
    
    reg3808 &= ~0x0F;
    reg3808 |= (uint8_t)(width >> 8) & 0x0F;
    reg3809 = (uint8_t)width & 0xFF;
    reg380A &= ~0x07;
    reg380A |= (uint8_t)(height >> 8) & 0x07;
    reg380B = (uint8_t)height & 0xFF;
    

//    reg3808 = 0x1;
//    reg3809 = 0x14;
//    reg380A = 0;
//    reg380B = 0xf0;
    ov5640_write_reg(0x3212, 0x03);
    ov5640_write_reg(0x3808, reg3808);
    ov5640_write_reg(0x3809, reg3809);
    ov5640_write_reg(0x380A, reg380A);
    ov5640_write_reg(0x380B, reg380B);
    ov5640_write_reg(0x3212, 0x13);
    ov5640_write_reg(0x3212, 0xA3);

    ov5640_get_output_size();


//    ov5640_write_reg(0x3800 , 0x0);
//    ov5640_write_reg(0x3801 , 0x8);
//    ov5640_write_reg(0x3802 , 0x0);
//    ov5640_write_reg(0x3803 , 0x2);
//    ov5640_write_reg(0x3804 , 0xa);
//    ov5640_write_reg(0x3805 , 0xa3);
//    ov5640_write_reg(0x3806 , 0x7);
//    ov5640_write_reg(0x3807 , 0x7a);
//    ov5640_write_reg(0x3808 , 0x1);
//    ov5640_write_reg(0x3809 , 0x14);
//    ov5640_write_reg(0x380a , 0x0);
//    ov5640_write_reg(0x380b , 0xf0);
//    ov5640_write_reg(0x380c , 0x6);
//    ov5640_write_reg(0x380d , 0x6b);
//    ov5640_write_reg(0x380e , 0x3);
//    ov5640_write_reg(0x380f , 0x3e);
//    ov5640_write_reg(0x3810 , 0x0);
//    ov5640_write_reg(0x3811 , 0x4);
//    ov5640_write_reg(0x3812 , 0x0);
//    ov5640_write_reg(0x3813 , 0x2);
//    ov5640_write_reg(0x3814 , 0x31);
//    ov5640_write_reg(0x3815 , 0x31);
//    ov5640_write_reg(0x3820 , 0x47);
//    ov5640_write_reg(0x3821 , 0x1);
//    ov5640_write_reg(0x4602 , 0x1);
//    ov5640_write_reg(0x4603 , 0x14);
//    ov5640_write_reg(0x4604 , 0x0);
//    ov5640_write_reg(0x4605 , 0xf0);


    return OV5640_EOK;
}

void OV5640_set_night_mode_VGA(){
    ov5640_write_reg(0x3034 ,0x1a);
    ov5640_write_reg(0x3035 ,0x21);
    ov5640_write_reg(0x3036 ,0x46);
    ov5640_write_reg(0x3037 ,0x13);
    ov5640_write_reg(0x3038 ,0x00);
    ov5640_write_reg(0x3039 ,0x00);
    ov5640_write_reg(0x3a00 ,0x7c);
    ov5640_write_reg(0x3a08 ,0x01);
    ov5640_write_reg(0x3a09 ,0x27);
    ov5640_write_reg(0x3a0a ,0x00);
    ov5640_write_reg(0x3a0b ,0xf6);
    ov5640_write_reg(0x3a0d ,0x04);
    ov5640_write_reg(0x3a0e ,0x04);
    ov5640_write_reg(0x3a02 ,0x0b);
    ov5640_write_reg(0x3a03 ,0x88);
    ov5640_write_reg(0x3a14 ,0x0b);
    ov5640_write_reg(0x3a15 ,0x88);


//    ov5640_write_reg(0x3034 ,0x1a);
//    ov5640_write_reg(0x3035 ,0x61);
//    ov5640_write_reg(0x3036 ,0x46);
//    ov5640_write_reg(0x3037 ,0x13);
//    ov5640_write_reg(0x3038 ,0x00);
//    ov5640_write_reg(0x3039 ,0x00);
//    ov5640_write_reg(0x3a00 ,0x78);
//    ov5640_write_reg(0x3a08 ,0x01);
//    ov5640_write_reg(0x3a09 ,0x27);
//    ov5640_write_reg(0x3a0a ,0x00);
//    ov5640_write_reg(0x3a0b ,0xf6);
//    ov5640_write_reg(0x3a0d ,0x04);
//    ov5640_write_reg(0x3a0e ,0x04);
//    ov5640_write_reg(0x3a02 ,0x03);
//    ov5640_write_reg(0x3a03 ,0xd8);
//    ov5640_write_reg(0x3a14 ,0x03);
//    ov5640_write_reg(0x3a15 ,0xd8);


//    ov5640_write_reg(0x3c00, 0x00); //bit[2]select 50/60hz banding, 0:50hz
//    ov5640_write_reg(0x3c01, 0x80); //bit[7] banding filter Auto Detection on/off, 1 off
//    ov5640_write_reg(0x3a08, 0x01); //50Hz banding filter value 8 MSB
//    ov5640_write_reg(0x3a09, 0x27); //50Hz banding filter value 8 LSB
//    ov5640_write_reg(0x3a0a, 0x00); //60Hz banding filter value 8MSB
//    ov5640_write_reg(0x3a0b, 0xf6); //60Hz banding filter value 8 LSB
//    ov5640_write_reg(0x3a0e, 0x04); //50Hz maximum banding step
//    ov5640_write_reg(0x3a0d, 0x06); //60Hz maximum banding step

//    ov5640_write_reg(0x3622 ,0x01);
//    ov5640_write_reg(0x3635 ,0x1c);
//    ov5640_write_reg(0x3634 ,0x40);
//    ov5640_write_reg(0x3c01 ,0x34);
//    ov5640_write_reg(0x3c00 ,0x00);
//    ov5640_write_reg(0x3c04 ,0x28);
//    ov5640_write_reg(0x3c05 ,0x98);
//    ov5640_write_reg(0x3c06 ,0x00);
//    ov5640_write_reg(0x3c07 ,0x08);
//    ov5640_write_reg(0x3c08 ,0x00);
//    ov5640_write_reg(0x3c09 ,0x1c);
//    ov5640_write_reg(0x300c ,0x22);
//    ov5640_write_reg(0x3c0a ,0x9c);
//    ov5640_write_reg(0x3c0b ,0x40);
//
//    ov5640_write_reg(0x5000, 0xff); // Pixel Correction ON,bit[2:1]: 11,select enable
//
//    ov5640_write_reg(0x3103 ,0x11);
//    ov5640_write_reg(0x3008 ,0x82);
//    ov5640_write_reg(0x3008 ,0x42);
//    ov5640_write_reg(0x3103 ,0x03);
//    ov5640_write_reg(0x3017 ,0xff);
//    ov5640_write_reg(0x3018 ,0xff);
//    ov5640_write_reg(0x3034 ,0x1a);
//    ov5640_write_reg(0x3035 ,0x11);
//    ov5640_write_reg(0x3036 ,0x46);
//    ov5640_write_reg(0x3037 ,0x13);
//    ov5640_write_reg(0x3108 ,0x01);
//    ov5640_write_reg(0x3630 ,0x36);
//    ov5640_write_reg(0x3631 ,0x0e);
//    ov5640_write_reg(0x3632 ,0xe2);
//    ov5640_write_reg(0x3633 ,0x12);
//    ov5640_write_reg(0x3621 ,0xe0);
//    ov5640_write_reg(0x3704 ,0xa0);
//    ov5640_write_reg(0x3703 ,0x5a);
//    ov5640_write_reg(0x3715 ,0x78);
//    ov5640_write_reg(0x3717 ,0x01);
//    ov5640_write_reg(0x370b ,0x60);
//    ov5640_write_reg(0x3705 ,0x1a);
//    ov5640_write_reg(0x3905 ,0x02);
//    ov5640_write_reg(0x3906 ,0x10);
//    ov5640_write_reg(0x3901 ,0x0a);
//    ov5640_write_reg(0x3731 ,0x12);
//    ov5640_write_reg(0x3600 ,0x08);
//    ov5640_write_reg(0x3601 ,0x33);
//    ov5640_write_reg(0x302d ,0x60);
//    ov5640_write_reg(0x3620 ,0x52);
//    ov5640_write_reg(0x371b ,0x20);
//    ov5640_write_reg(0x471c ,0x50);
//    ov5640_write_reg(0x3a13 ,0x43);
//    ov5640_write_reg(0x3a18 ,0x00);
//    ov5640_write_reg(0x3a19 ,0xf8);
//    ov5640_write_reg(0x3635 ,0x13);
//    ov5640_write_reg(0x3636 ,0x03);
//    ov5640_write_reg(0x3634 ,0x40);
//    ov5640_write_reg(0x3622 ,0x01);
//
//    ov5640_write_reg(0x3c01 ,0x34);
//    ov5640_write_reg(0x3c04 ,0x28);
//    ov5640_write_reg(0x3c05 ,0x98);
//    ov5640_write_reg(0x3c06 ,0x00);
//    ov5640_write_reg(0x3c07 ,0x08);
//    ov5640_write_reg(0x3c08 ,0x00);
//    ov5640_write_reg(0x3c09 ,0x1c);
//    ov5640_write_reg(0x3c0a ,0x9c);
//    ov5640_write_reg(0x3c0b ,0x40);
//    ov5640_write_reg(0x3820 ,0x41);
//    ov5640_write_reg(0x3821 ,0x07);
//    ov5640_write_reg(0x3814 ,0x31);
//    ov5640_write_reg(0x3815 ,0x31);
//    ov5640_write_reg(0x3800 ,0x00);
//    ov5640_write_reg(0x3801 ,0x00);
//    ov5640_write_reg(0x3802 ,0x00);
//    ov5640_write_reg(0x3803 ,0x04);
//    ov5640_write_reg(0x3804 ,0x0a);
//    ov5640_write_reg(0x3805 ,0x3f);
//    ov5640_write_reg(0x3806 ,0x07);
//    ov5640_write_reg(0x3807 ,0x9b);
//    ov5640_write_reg(0x3808 ,0x02);
//    ov5640_write_reg(0x3809 ,0x80);
//    ov5640_write_reg(0x380a ,0x01);
//    ov5640_write_reg(0x380b ,0xe0);
//    ov5640_write_reg(0x380c ,0x07);
//    ov5640_write_reg(0x380d ,0x68);
//    ov5640_write_reg(0x380e ,0x03);
//    ov5640_write_reg(0x380f ,0xd8);
//    ov5640_write_reg(0x3810 ,0x00);
//    ov5640_write_reg(0x3811 ,0x10);
//    ov5640_write_reg(0x3812 ,0x00);
//    ov5640_write_reg(0x3813 ,0x06);
//    ov5640_write_reg(0x3618 ,0x00);
//    ov5640_write_reg(0x3612 ,0x29);
//    ov5640_write_reg(0x3708 ,0x64);
//    ov5640_write_reg(0x3709 ,0x52);
//    ov5640_write_reg(0x370c ,0x03);
//    ov5640_write_reg(0x3a02 ,0x03);
//    ov5640_write_reg(0x3a03 ,0xd8);
//    ov5640_write_reg(0x3a08 ,0x01);
//    ov5640_write_reg(0x3a09 ,0x27);
//    ov5640_write_reg(0x3a0a ,0x00);
//    ov5640_write_reg(0x3a0b ,0xf6);
//    ov5640_write_reg(0x3a0e ,0x03);
//    ov5640_write_reg(0x3a0d ,0x04);
//    ov5640_write_reg(0x3a14 ,0x03);
//    ov5640_write_reg(0x3a15 ,0xd8);
//
//    ov5640_write_reg(0x4001 ,0x02);
//    ov5640_write_reg(0x4004 ,0x02);
//    ov5640_write_reg(0x3000 ,0x00);
//    ov5640_write_reg(0x3002 ,0x1c);
//    ov5640_write_reg(0x3004 ,0xff);
//    ov5640_write_reg(0x3006 ,0xc3);
//    ov5640_write_reg(0x300e ,0x58);
//    ov5640_write_reg(0x302e ,0x00);
//    ov5640_write_reg(0x4300 ,0x30);
//    ov5640_write_reg(0x501f ,0x00);
//    ov5640_write_reg(0x4713 ,0x03);
//    ov5640_write_reg(0x4407 ,0x04);
//    ov5640_write_reg(0x440e ,0x00);
//    ov5640_write_reg(0x460b ,0x35);
//    ov5640_write_reg(0x460c ,0x22);
//    ov5640_write_reg(0x3824 ,0x02);
//    ov5640_write_reg(0x5000 ,0xa7);
//    ov5640_write_reg(0x5001 ,0xa3);
//    ov5640_write_reg(0x5180 ,0xff);
//    ov5640_write_reg(0x5181 ,0xf2);
//    ov5640_write_reg(0x5182 ,0x00);
//    ov5640_write_reg(0x5183 ,0x14);
//    ov5640_write_reg(0x5184 ,0x25);
//    ov5640_write_reg(0x5185 ,0x24);
//    ov5640_write_reg(0x5186 ,0x09);
//    ov5640_write_reg(0x5187 ,0x09);
//    ov5640_write_reg(0x5188 ,0x09);
//    ov5640_write_reg(0x5189 ,0x75);
//    ov5640_write_reg(0x518a ,0x54);
//    ov5640_write_reg(0x518b ,0xe0);
//    ov5640_write_reg(0x518c ,0xb2);
//    ov5640_write_reg(0x518d ,0x42);
//    ov5640_write_reg(0x518e ,0x3d);
//    ov5640_write_reg(0x518f ,0x56);
//    ov5640_write_reg(0x5190 ,0x46);
//    ov5640_write_reg(0x5191 ,0xf8);
//    ov5640_write_reg(0x5192 ,0x04);
//    ov5640_write_reg(0x5193 ,0x70);
//    ov5640_write_reg(0x5194 ,0xf0);
//    ov5640_write_reg(0x5195 ,0xf0);
//    ov5640_write_reg(0x5196 ,0x03);
//    ov5640_write_reg(0x5197 ,0x01);
//    ov5640_write_reg(0x5198 ,0x04);
//    ov5640_write_reg(0x5199 ,0x12);
//    ov5640_write_reg(0x519a ,0x04);
//    ov5640_write_reg(0x519b ,0x00);
//    ov5640_write_reg(0x519c ,0x06);
//    ov5640_write_reg(0x519d ,0x82);
//    ov5640_write_reg(0x519e ,0x38);
//    ov5640_write_reg(0x5381 ,0x1e);
//    ov5640_write_reg(0x5382 ,0x5b);
//    ov5640_write_reg(0x5383 ,0x08);
//    ov5640_write_reg(0x5384 ,0x0a);
//    ov5640_write_reg(0x5385 ,0x7e);
//    ov5640_write_reg(0x5386 ,0x88);
//    ov5640_write_reg(0x5387 ,0x7c);
//    ov5640_write_reg(0x5388 ,0x6c);
//    ov5640_write_reg(0x5389 ,0x10);
//    ov5640_write_reg(0x538a ,0x01);
//    ov5640_write_reg(0x538b ,0x98);
//    ov5640_write_reg(0x5300 ,0x08);
//    ov5640_write_reg(0x5301 ,0x30);
//    ov5640_write_reg(0x5302 ,0x10);
//    ov5640_write_reg(0x5303 ,0x00);
//    ov5640_write_reg(0x5304 ,0x08);
//    ov5640_write_reg(0x5305 ,0x30);
//    ov5640_write_reg(0x5306 ,0x08);
//    ov5640_write_reg(0x5307 ,0x16);
//    ov5640_write_reg(0x5309 ,0x08);
//    ov5640_write_reg(0x530a ,0x30);
//    ov5640_write_reg(0x530b ,0x04);
//    ov5640_write_reg(0x530c ,0x06);
//    ov5640_write_reg(0x5480 ,0x01);
//    ov5640_write_reg(0x5481 ,0x08);
//    ov5640_write_reg(0x5482 ,0x14);
//    ov5640_write_reg(0x5483 ,0x28);
//    ov5640_write_reg(0x5484 ,0x51);
//    ov5640_write_reg(0x5485 ,0x65);
//    ov5640_write_reg(0x5486 ,0x71);
//    ov5640_write_reg(0x5487 ,0x7d);
//    ov5640_write_reg(0x5488 ,0x87);
//    ov5640_write_reg(0x5489 ,0x91);
//    ov5640_write_reg(0x548a ,0x9a);
//    ov5640_write_reg(0x548b ,0xaa);
//    ov5640_write_reg(0x548c ,0xb8);
//    ov5640_write_reg(0x548d ,0xcd);
//    ov5640_write_reg(0x548e ,0xdd);
//    ov5640_write_reg(0x548f ,0xea);
//    ov5640_write_reg(0x5490 ,0x1d);
//    ov5640_write_reg(0x5580 ,0x02);
//    ov5640_write_reg(0x5583 ,0x40);
//    ov5640_write_reg(0x5584 ,0x10);
//    ov5640_write_reg(0x5589 ,0x10);
//    ov5640_write_reg(0x558a ,0x00);
//    ov5640_write_reg(0x558b ,0xf8);
//    ov5640_write_reg(0x5800 ,0x23);
//
//    ov5640_write_reg(0x5801 ,0x14);
//    ov5640_write_reg(0x5802 ,0x0f);
//    ov5640_write_reg(0x5803 ,0x0f);
//    ov5640_write_reg(0x5804 ,0x12);
//    ov5640_write_reg(0x5805 ,0x26);
//    ov5640_write_reg(0x5806 ,0x0c);
//    ov5640_write_reg(0x5807 ,0x08);
//    ov5640_write_reg(0x5808 ,0x05);
//    ov5640_write_reg(0x5809 ,0x05);
//    ov5640_write_reg(0x580a ,0x08);
//    ov5640_write_reg(0x580b ,0x0d);
//    ov5640_write_reg(0x580c ,0x08);
//    ov5640_write_reg(0x580d ,0x03);
//    ov5640_write_reg(0x580e ,0x00);
//    ov5640_write_reg(0x580f ,0x00);
//    ov5640_write_reg(0x5810 ,0x03);
//    ov5640_write_reg(0x5811 ,0x09);
//    ov5640_write_reg(0x5812 ,0x07);
//    ov5640_write_reg(0x5813 ,0x03);
//    ov5640_write_reg(0x5814 ,0x00);
//    ov5640_write_reg(0x5815 ,0x01);
//    ov5640_write_reg(0x5816 ,0x03);
//    ov5640_write_reg(0x5817 ,0x08);
//    ov5640_write_reg(0x5818 ,0x0d);
//    ov5640_write_reg(0x5819 ,0x08);
//    ov5640_write_reg(0x581a ,0x05);
//    ov5640_write_reg(0x581b ,0x06);
//    ov5640_write_reg(0x581c ,0x08);
//    ov5640_write_reg(0x581d ,0x0e);
//    ov5640_write_reg(0x581e ,0x29);
//    ov5640_write_reg(0x581f ,0x17);
//    ov5640_write_reg(0x5820 ,0x11);
//    ov5640_write_reg(0x5821 ,0x11);
//    ov5640_write_reg(0x5822 ,0x15);
//    ov5640_write_reg(0x5823 ,0x28);
//    ov5640_write_reg(0x5824 ,0x46);
//    ov5640_write_reg(0x5825 ,0x26);
//    ov5640_write_reg(0x5826 ,0x08);
//    ov5640_write_reg(0x5827 ,0x26);
//    ov5640_write_reg(0x5828 ,0x64);
//    ov5640_write_reg(0x5829 ,0x26);
//    ov5640_write_reg(0x582a ,0x24);
//    ov5640_write_reg(0x582b ,0x22);
//    ov5640_write_reg(0x582c ,0x24);
//    ov5640_write_reg(0x582d ,0x24);
//    ov5640_write_reg(0x582e ,0x06);
//    ov5640_write_reg(0x582f ,0x22);
//    ov5640_write_reg(0x5830 ,0x40);
//
//    ov5640_write_reg(0x5831 ,0x42);
//    ov5640_write_reg(0x5832 ,0x24);
//    ov5640_write_reg(0x5833 ,0x26);
//    ov5640_write_reg(0x5834 ,0x24);
//    ov5640_write_reg(0x5835 ,0x22);
//    ov5640_write_reg(0x5836 ,0x22);
//    ov5640_write_reg(0x5837 ,0x26);
//    ov5640_write_reg(0x5838 ,0x44);
//    ov5640_write_reg(0x5839 ,0x24);
//    ov5640_write_reg(0x583a ,0x26);
//    ov5640_write_reg(0x583b ,0x28);
//    ov5640_write_reg(0x583c ,0x42);
//    ov5640_write_reg(0x583d ,0xce);
//    ov5640_write_reg(0x5025 ,0x00);
//    ov5640_write_reg(0x3a0f ,0x30);
//    ov5640_write_reg(0x3a10 ,0x28);
//    ov5640_write_reg(0x3a1b ,0x30);
//    ov5640_write_reg(0x3a1e ,0x26);
//    ov5640_write_reg(0x3a11 ,0x60);
//    ov5640_write_reg(0x3a1f ,0x14);
//    ov5640_write_reg(0x3008 ,0x02);
//    ov5640_write_reg(0x3035 ,0x21);
//
//    ov5640_write_reg(0x3c01,0xb4);
//    ov5640_write_reg(0x3c00,0x04);
//
//    ov5640_write_reg(0x3a19,0x7c);
//    ov5640_write_reg(0x5800 ,0x2c);
//    ov5640_write_reg(0x5801 ,0x17);
//    ov5640_write_reg(0x5802 ,0x11);
//    ov5640_write_reg(0x5803 ,0x11);
//    ov5640_write_reg(0x5804 ,0x15);
//    ov5640_write_reg(0x5805 ,0x29);
//    ov5640_write_reg(0x5806 ,0x08);
//    ov5640_write_reg(0x5807 ,0x06);
//    ov5640_write_reg(0x5808 ,0x04);
//    ov5640_write_reg(0x5809 ,0x04);
//    ov5640_write_reg(0x580a ,0x05);
//    ov5640_write_reg(0x580b ,0x07);
//    ov5640_write_reg(0x580c ,0x06);
//    ov5640_write_reg(0x580d ,0x03);
//    ov5640_write_reg(0x580e ,0x01);
//    ov5640_write_reg(0x580f ,0x01);
//    ov5640_write_reg(0x5810 ,0x03);
//    ov5640_write_reg(0x5811 ,0x06);
//
//    ov5640_write_reg(0x5812 ,0x06);
//    ov5640_write_reg(0x5813 ,0x02);
//    ov5640_write_reg(0x5814 ,0x01);
//    ov5640_write_reg(0x5815 ,0x01);
//    ov5640_write_reg(0x5816 ,0x04);
//    ov5640_write_reg(0x5817 ,0x07);
//    ov5640_write_reg(0x5818 ,0x06);
//    ov5640_write_reg(0x5819 ,0x07);
//    ov5640_write_reg(0x581a ,0x06);
//    ov5640_write_reg(0x581b ,0x06);
//    ov5640_write_reg(0x581c ,0x06);
//    ov5640_write_reg(0x581d ,0x0e);
//    ov5640_write_reg(0x581e ,0x31);
//    ov5640_write_reg(0x581f ,0x12);
//    ov5640_write_reg(0x5820 ,0x11);
//    ov5640_write_reg(0x5821 ,0x11);
//    ov5640_write_reg(0x5822 ,0x11);
//    ov5640_write_reg(0x5823 ,0x2f);
//    ov5640_write_reg(0x5824 ,0x12);
//    ov5640_write_reg(0x5825 ,0x25);
//    ov5640_write_reg(0x5826 ,0x39);
//    ov5640_write_reg(0x5827 ,0x29);
//    ov5640_write_reg(0x5828 ,0x27);
//    ov5640_write_reg(0x5829 ,0x39);
//    ov5640_write_reg(0x582a ,0x26);
//    ov5640_write_reg(0x582b ,0x33);
//    ov5640_write_reg(0x582c ,0x24);
//    ov5640_write_reg(0x582d ,0x39);
//    ov5640_write_reg(0x582e ,0x28);
//    ov5640_write_reg(0x582f ,0x21);
//    ov5640_write_reg(0x5830 ,0x40);
//    ov5640_write_reg(0x5831 ,0x21);
//    ov5640_write_reg(0x5832 ,0x17);
//    ov5640_write_reg(0x5833 ,0x17);
//    ov5640_write_reg(0x5834 ,0x15);
//    ov5640_write_reg(0x5835 ,0x11);
//    ov5640_write_reg(0x5836 ,0x24);
//    ov5640_write_reg(0x5837 ,0x27);
//    ov5640_write_reg(0x5838 ,0x26);
//    ov5640_write_reg(0x5839 ,0x26);
//    ov5640_write_reg(0x583a ,0x26);
//    ov5640_write_reg(0x583b ,0x28);
//    ov5640_write_reg(0x583c ,0x14);
//    ov5640_write_reg(0x583d ,0xee);
//    ov5640_write_reg(0x4005 ,0x1a);
//
//
//    ov5640_write_reg(0x5381,0x26);
//    ov5640_write_reg(0x5382,0x50);
//
//    ov5640_write_reg(0x5383,0x0c);
//    ov5640_write_reg(0x5384,0x09);
//    ov5640_write_reg(0x5385,0x74);
//    ov5640_write_reg(0x5386,0x7d);
//    ov5640_write_reg(0x5387,0x7e);
//    ov5640_write_reg(0x5388,0x75);
//    ov5640_write_reg(0x5389,0x09);
//    ov5640_write_reg(0x538b,0x98);
//    ov5640_write_reg(0x538a,0x01);
//
//    ov5640_write_reg(0x5580,0x02);
//    ov5640_write_reg(0x5588,0x01);
//    ov5640_write_reg(0x5583,0x40);
//    ov5640_write_reg(0x5584,0x10);
//    ov5640_write_reg(0x5589,0x0f);
//    ov5640_write_reg(0x558a,0x00);
//    ov5640_write_reg(0x558b,0x3f);
//
//    ov5640_write_reg(0x5308,0x25);
//    ov5640_write_reg(0x5304,0x08);
//    ov5640_write_reg(0x5305,0x30);
//    ov5640_write_reg(0x5306,0x10);
//    ov5640_write_reg(0x5307,0x20);
//
//    ov5640_write_reg(0x5180,0xff);
//    ov5640_write_reg(0x5181,0xf2);
//    ov5640_write_reg(0x5182,0x11);
//    ov5640_write_reg(0x5183,0x14);
//    ov5640_write_reg(0x5184,0x25);
//    ov5640_write_reg(0x5185,0x24);
//    ov5640_write_reg(0x5186,0x10);
//    ov5640_write_reg(0x5187,0x12);
//    ov5640_write_reg(0x5188,0x10);
//    ov5640_write_reg(0x5189,0x80);
//    ov5640_write_reg(0x518a,0x54);
//    ov5640_write_reg(0x518b,0xb8);
//    ov5640_write_reg(0x518c,0xb2);
//    ov5640_write_reg(0x518d,0x42);
//    ov5640_write_reg(0x518e,0x3a);
//    ov5640_write_reg(0x518f,0x56);
//    ov5640_write_reg(0x5190,0x46);
//    ov5640_write_reg(0x5191,0xf0);
//    ov5640_write_reg(0x5192,0xf);
//    ov5640_write_reg(0x5193,0x70);
//
//    ov5640_write_reg(0x5194,0xf0);
//    ov5640_write_reg(0x5195,0xf0);
//    ov5640_write_reg(0x5196,0x3);
//    ov5640_write_reg(0x5197,0x1);
//    ov5640_write_reg(0x5198,0x6);
//    ov5640_write_reg(0x5199,0x62);
//    ov5640_write_reg(0x519a,0x4);
//    ov5640_write_reg(0x519b,0x0);
//    ov5640_write_reg(0x519c,0x4);
//    ov5640_write_reg(0x519d,0xe7);
//    ov5640_write_reg(0x519e,0x38);



}
