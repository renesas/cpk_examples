/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * ov5640_cfg.h
 *
 *  Created on: Nov 17, 2023
 *      Author: smile
 */

#ifndef OV5640_CFG_H_
#define OV5640_CFG_H_
#include "hal_data.h"
//#include"main.h"

#define OV5640_FW_DOWNLOAD_ADDR 0x8000

typedef struct
{
    uint16_t reg;
    uint8_t dat;
} ov5640_reg_cfg_t;

extern const uint8_t default_regs[][3];
extern ov5640_reg_cfg_t ov5640_init_cfg[207];
//extern ov5640_reg_cfg_t ov5640_init_cfg[254];
extern ov5640_reg_cfg_t ov5640_rgb565_cfg[46]; //45
//extern ov5640_reg_cfg_t ov5640_rgb565_cfg[2];

extern ov5640_reg_cfg_t ov5640_jpeg_cfg[41];
extern uint8_t ov5640_auto_focus_firmware[4077];

#endif
