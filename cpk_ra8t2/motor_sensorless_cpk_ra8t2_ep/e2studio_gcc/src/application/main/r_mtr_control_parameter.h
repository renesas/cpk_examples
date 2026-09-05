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
* Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.      
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
#define    MTR_CONTROL_PARAMETER     (0)

/* Target control parameter definitions */                                      
#define    CP_CURRENT_OMEGA        (300.0f)                /* Natural frequency for current loop */
#define    CP_CURRENT_ZETA         (1.0f)                  /* Damping ratio for current loop */
#define    CP_SPEED_OMEGA          (5.0f)                  /* Natural frequency for speed loop */
#define    CP_SPEED_ZETA           (1.0f)                  /* Damping ratio for speed loop */
#define    CP_E_OBS_OMEGA          (1000.0f)               /* Natural frequency of BEMF observer */
#define    CP_E_OBS_ZETA           (1.0f)                  /* Damping ratio of BEMF observer */
#define    CP_PLL_EST_OMEGA        (20.0f)                 /* Natural frequency of PLL Speed estimate loop */
#define    CP_PLL_EST_ZETA         (1.0f)                  /* Damping ratio of PLL Speed estimate loop */
#define    CP_ID_DOWN_SPEED_RPM    (500)                   /* Speed to start decreasing id [rpm]  (mechanical) */
#define    CP_ID_UP_SPEED_RPM      (400)                   /* Speed to start increasing id [rpm]  (mechanical) */
#define    CP_MAX_SPEED_RPM        (2400)                  /* Maximum speed[rpm] (mechanical) */
#define    CP_OVERSPEED_LIMIT_RPM  (4500)                  /* Over speed limit [rpm] (mechanical) */
#define    CP_OL_ID_REF            (0.3f)                  /* Id reference when low speed [A] */

#endif /* R_MTR_CONTROL_PARAMETER_H */                                          
