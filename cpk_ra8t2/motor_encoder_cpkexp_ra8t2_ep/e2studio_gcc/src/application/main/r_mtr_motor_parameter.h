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
* File Name   : r_mtr_motor_parameter.h
* Description : Definition of the target motor parameters
*                  (Renesas Motor Workbench Output file)
******************************************************************************/
/******************************************************************************
* Date :
******************************************************************************/

/* Guard against multiple inclusion */
#ifndef R_MTR_MOTOR_PARAMETER_H
#define R_MTR_MOTOR_PARAMETER_H

/******************************************************************************
* Macro definitions
******************************************************************************/
#define    MTR_MOTOR_PARAMETER     (1)

/* Target motor definitions */
#if 0
#define    MP_POLE_PAIRS           (4)                     /* Number of pole pairs */
#define    MP_RESISTANCE           (0.84F)                  /* Resistance [ohm] */
#define    MP_D_INDUCTANCE         (0.0011F)               /* D-axis inductance [H] */
#define    MP_Q_INDUCTANCE         (0.0011F)               /* Q-axis inductance [H] */
#define    MP_MAGNETIC_FLUX        (0.00623F)              /* Permanent magnetic flux [Wb] */
#define    MP_ROTOR_INERTIA        (0.0000041F)            /* Rotor inertia [kgm^2] */
#define    MP_NOMINAL_CURRENT_RMS  (1.8F)                  /* Nominal current [Arms] */
#endif
#define    MP_POLE_PAIRS           (2)                     /* Number of pole pairs */
#define    MP_MAGNETIC_FLUX        (0.01505694f)           /* Permanent magnetic flux [Wb] */
#define    MP_RESISTANCE           (1.237147f)             /* Resistance [ohm] */
#define    MP_D_INDUCTANCE         (0.001605095f)          /* D-axis inductance [H] */
#define    MP_Q_INDUCTANCE         (0.001843709f)          /* Q-axis inductance [H] */
#define    MP_ROTOR_INERTIA        (3.455322E-06f)         /* Rotor inertia [kgm^2] */
#define    MP_NOMINAL_CURRENT_RMS  (1.9f)                  /* Nominal current [Arms] */

#endif /* R_MTR_MOTOR_PARAMETER_H */
