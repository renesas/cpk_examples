/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#ifndef __ARDUCAM_H
#define __ARDUCAM_H
#include "hal_data.h"
fsp_err_t wrSensorReg16_8(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t regDat);
fsp_err_t rdSensorReg16_8(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t* regDat);
fsp_err_t rdSensorReg16_Multi(i2c_master_ctrl_t * p_api_ctrl, uint16_t regID, uint8_t* regDat, uint32_t len);

#endif
