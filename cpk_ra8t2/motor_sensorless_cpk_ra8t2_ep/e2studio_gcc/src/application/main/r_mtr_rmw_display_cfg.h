/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2024 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

/******************************************************************************
* File Name   : r_mtr_rmw_display_cfg.h
* Description : Definition to display software information in RMW
******************************************************************************/
/******************************************************************************
* Date :
******************************************************************************/

/* Guard against multiple inclusion */
#ifndef R_MTR_RMW_DISPLAY_CFG_H
#define R_MTR_RMW_DISPLAY_CFG_H

/******************************************************************************
* Macro definitions
******************************************************************************/
/* For RMW display */
#define     CONF_MOTOR_TYPE ("Brushless DC Motor")
#define     CONF_CONTROL ("Sensorless vector control (Speed control)")
#define     CONF_INVERTER ("MCI-LV-1")
#define     CONF_MOTOR_TYPE_LEN (18)
#define     CONF_CONTROL_LEN (41)
#define     CONF_INVERTER_LEN (8)

#endif /* R_MTR_RMW_DISPLAY_CFG_H */
