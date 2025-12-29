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
* Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

/******************************************************************************
* File Name   : r_mtr_control_parameter.h
* Description : Definition of default control parameters for sensorless speed control
*                  (Renesas Motor Workbench Output file)
******************************************************************************/
/******************************************************************************
* Date :
******************************************************************************/

/* Guard against multiple inclusion */
#ifndef R_MTR_CONTROL_PARAMETER_H
#define R_MTR_CONTROL_PARAMETER_H

/******************************************************************************
* Macro definitions
******************************************************************************/
#define    MTR_CONTROL_PARAMETER     (1)

/* Target control parameter definitions */
#if 0
#define    CP_CURRENT_OMEGA        (300.0f)                /* Natural frequency for current loop */
#define    CP_CURRENT_ZETA         (1.0f)                  /* Damping ratio for current loop */
#define    CP_SPEED_OMEGA          (30.0f)                 /* Natural frequency for speed loop */
#define    CP_SPEED_ZETA           (1.0f)                  /* Damping ratio for speed loop */
#define    CP_POS_OMEGA            (10.0f)                 /* natural frequency for position loop */
#define    CP_SOB_OMEGA            (200.0f)                /* natural frequency for speed observer */
#define    CP_SOB_ZETA             (1.0f)                  /* damping ratio for speed observer */
#define    CP_MIN_SPEED_RPM        (0)                     /* minimum speed [rpm] (mechanical angle) */
#define    CP_MAX_SPEED_RPM        (4000)                  /* Maximum speed[rpm] (mechanical) */
#define    CP_OVERSPEED_LIMIT_RPM  (4500)                  /* Over speed limit [rpm] (mechanical) */
#define    CP_OL_ID_REF            (1.0f)                  /* Id reference when low speed [A] */
#endif
#define     CP_CURRENT_OMEGA            (300.0f)      /* natural frequency for current loop */
#define     CP_CURRENT_ZETA             (1.0f)        /* damping ratio for current loop */
#define     CP_SPEED_OMEGA              (12.0f)//(30.0f)       /* natural frequency for speed loop */
#define     CP_SPEED_ZETA               (1.0f)        /* damping ratio for speed loop */
#define     CP_POS_OMEGA                (80.0f)//(40.0f)//(10.0f)       /* natural frequency for position loop */
#define     CP_SOB_OMEGA                (100.0f)//(200.0f)      /* natural frequency for speed observer */
#define     CP_SOB_ZETA                 (1.0f)        /* damping ratio for speed observer */
#define     CP_MIN_SPEED_RPM            (0)           /* minimum speed [rpm] (mechanical angle) */
//#define     CP_MAX_SPEED_RPM            (2000)        /* maximum speed [rpm] (mechanical angle) */
//#define     CP_SPEED_LIMIT_RPM          (2100)        /* over speed limit [rpm] (mechanical angle) */
//#define     CP_OL_ID_REF                (1.5f)        /* id reference when low speed [A] */
#define    CP_MAX_SPEED_RPM        (4194.439f)             /* Maximun speed[rpm] (mechanical) */
#define    CP_OVERSPEED_LIMIT_RPM      (6291.659f)             /* Over speed limit [rpm] (mechanical) */
#define    CP_OL_ID_REF            (1.861612f)             /* Id reference when low speed [A] */

#endif /* R_MTR_CONTROL_PARAMETER_H */
