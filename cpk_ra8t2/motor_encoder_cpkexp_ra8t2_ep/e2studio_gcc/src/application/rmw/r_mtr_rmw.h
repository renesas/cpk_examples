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
* Copyright (C) 2024 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name   : r_mtr_rmw.h
* Description : Definitions of user interface using ICS
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version
*         : 19.01.2024 1.00
***********************************************************************************************************************/

/* Guard against multiple inclusion */
#ifndef R_MTR_RMW_H
#define R_MTR_RMW_H

/***********************************************************************************************************************
* Includes <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "ICS2_RA8T2.h"

/***********************************************************************************************************************
* Macro definitions
***********************************************************************************************************************/
#define     USE_BUILT_IN             (0)                            /* 0:Normal 1:Built-in */

#if USE_BUILT_IN
#define     MTR_ICS_DECIMATION       (1)                            /* ICS watch skipping number */

/* For ICS */
#define     ICS_BRR                  (21)                           /* Baudrate select */
#define     ICS_INT_MODE             (1)                            /* Mode select */

#else

#define     MTR_ICS_DECIMATION       (5)                            /* ICS watch skipping number */

/* For ICS */
#define     ICS_BRR                  (19)                           /* Baudrate select */
#define     ICS_INT_MODE             (1)                            /* Mode select */
#endif

#define     MTR_SQRT_2                   (1.41421356f)
#define     MTR_TWO_PI                   (6.28318531f)
#define     MTR_RAD_RPM                  (60/MTR_TWO_PI)
#define     MTR_RAD_DEGREE               (360/MTR_TWO_PI)
#define     MTR_OVERCURRENT_MARGIN_MULT  (1.5f)                     /* Multiplier for over-current limit*/

/***********************************************************************************************************************
* Function Name : mtr_set_com_variables
* Description   : Set com variables to global value
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_set_com_variables(void);

/***********************************************************************************************************************
* Function Name : mtr_ics_variables_init
* Description   : Set global value to ics watch buffer
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_ics_variables_init(void);

/***********************************************************************************************************************
* Function Name : mtr_ics_interrupt_process
* Description   : Interrupt process to reflect data into RMW (called at current backward event)
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_ics_interrupt_process(void);

#endif /* R_MTR_RMW_H */
