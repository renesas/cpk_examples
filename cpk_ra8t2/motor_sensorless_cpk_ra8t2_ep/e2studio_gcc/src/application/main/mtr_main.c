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
* Copyright (C) 2025 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name   : mtr_main.c
* Description : The main function and the processes of motor control application layer
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version
*         : 25.03.2024 1.00
*         : 14.05.2025 1.01         Instance pointers are reffered.
***********************************************************************************************************************/

/***********************************************************************************************************************
* Includes <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include <stdint.h>
#include "mtr_main.h"
#include "hal_data.h"
#include "r_mtr_rmw.h"
#include "r_mtr_rmw_display_cfg.h"

/***********************************************************************************************************************
* Global variables
***********************************************************************************************************************/
float       g_f4_speed_ref = 0.0F;
uint8_t     g_u1_motor_status;          /* Motor status */
uint8_t     com_u1_sw_userif;           /* User interface switch */
uint8_t     g_u1_sw_userif;             /* User interface switch */
uint8_t     com_u1_mode_system;         /* System mode */
uint8_t     g_u1_mode_system;           /* System mode */
uint16_t    g_u2_max_speed_rpm;
uint8_t     g_u1_stop_req;
uint16_t    g_u2_chk_error;
uint16_t    g_u2_vr1_ad;
uint16_t    g_u2_conf_hw;
uint16_t    g_u2_conf_sw;
uint16_t    g_u2_conf_tool;
uint8_t     g_u1_conf_motor_type_len;
uint8_t     g_u1_conf_control_len;
uint8_t     g_u1_conf_inverter_len;
uint8_t     g_u1_conf_motor_type[CONF_MOTOR_TYPE_LEN];
uint8_t     g_u1_conf_control[CONF_CONTROL_LEN];
uint8_t     g_u1_conf_inverter[CONF_INVERTER_LEN];
uint8_t     g_u1_reset_req;             /* Reset request flag */
uint8_t     g_u1_sw_cnt;                /* Counter to remove chattering */

const motor_instance_t  *p_motor_instance;
motor_cfg_t g_user_motor_cfg;
motor_sensorless_instance_ctrl_t *p_user_motor_instance_ctrl;
motor_sensorless_extended_cfg_t g_user_motor_sensorless_extended_cfg;
motor_speed_cfg_t g_user_motor_speed_cfg;
motor_speed_extended_cfg_t g_user_motor_speed_extended_cfg;
motor_current_cfg_t g_user_motor_current_cfg;
motor_current_extended_cfg_t g_user_motor_current_extended_cfg;
motor_angle_cfg_t g_user_motor_angle_cfg;
motor_estimate_extended_cfg_t g_user_motor_estimate_extended_cfg;
motor_driver_cfg_t g_user_motor_driver_cfg;
motor_driver_extended_cfg_t g_user_motor_driver_extended_cfg;
motor_current_motor_parameter_t g_user_motor_current_motor_parameter;
motor_current_design_parameter_t g_user_motor_current_design_parameter;

/***********************************************************************************************************************
* Private functions
***********************************************************************************************************************/
static void     motor_fsp_init (void);
static void     mtr_board_led_control (uint8_t u1_motor_status);
static uint8_t  mtr_remove_sw_chattering (uint8_t u1_sw, uint8_t u1_on_off);
static void     mtr_set_reference(void);
static void     board_ui (void);           /* Board user interface */
static void     ics_ui (void);             /* ICS (Analyzer) user interface */
static void     software_init (void);      /* Software initialize */
static uint16_t get_vr1 (void);
static uint8_t  get_sw1 (void);
static uint8_t  get_sw2 (void);
static void     led1_on (void);
static void     led2_on (void);
static void     led3_on (void);
static void     led1_off (void);
static void     led2_off (void);
static void     led3_off (void);
static void     make_cpkexp_init (void);

static uint8_t  u1_init_flag = 0U;
static uint8_t  u1_stop_wait_count = 0U;

#define     MOTOR_STATUS_STOP       (0)
#define     MOTOR_STATUS_RUN        (1)
#define     MOTOR_STATUS_ERROR      (2)
#define     MOTOR_CTRL_EVENT_STOP   (0)
#define     MOTOR_CTRL_EVENT_RUN    (1)
#define     MOTOR_CTRL_EVENT_ERROR  (2)
#define     MOTOR_CTRL_EVENT_RESET  (3)

#define     MOTOR_STOP_WAIT_COUNT   (80)

/***********************************************************************************************************************
* Function Name : mtr_init
* Description   : Initialization for Motor Control
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_init(void)
{
    int i;
    uint8_t u1_conf_motor_type[] = CONF_MOTOR_TYPE;
    uint8_t u1_conf_control[] = CONF_CONTROL;
    uint8_t u1_conf_inverter[] = CONF_INVERTER;
    g_u1_conf_motor_type_len = CONF_MOTOR_TYPE_LEN;
    g_u1_conf_control_len    = CONF_CONTROL_LEN;
    g_u1_conf_inverter_len   = CONF_INVERTER_LEN;
    for (i = 0; i < g_u1_conf_motor_type_len; i++)
    {
        g_u1_conf_motor_type[i] = u1_conf_motor_type[i];
    }
    for (i = 0; i < g_u1_conf_control_len; i++)
    {
        g_u1_conf_control[i] = u1_conf_control[i];
    }
    for (i = 0; i < g_u1_conf_inverter_len; i++)
    {
        g_u1_conf_inverter[i] = u1_conf_inverter[i];
    }
    g_u2_conf_hw = 0x0008;                        /* 0000000000001000b */
    g_u2_conf_sw = 0x0000;                        /* 0000000000000000b */
    g_u2_conf_tool = 0x0300;                      /* 0000011000000000b */


    make_cpkexp_init();

    /* Set motor instance pointer */
    p_motor_instance = &g_motor_sensorless0;

    /* Start instances & set user parameters */
    motor_fsp_init();

    /* Initialize private global variables */
    software_init();
    g_u1_sw_userif   = CONFIG_DEFAULT_UI;
    com_u1_sw_userif = CONFIG_DEFAULT_UI;

    /* Set the flag of finsh intialization */
    u1_init_flag = 1U;

    /* Execute reset event */
    p_motor_instance->p_api->reset(p_motor_instance->p_ctrl);
} /* End of function mtr_init() */

/***********************************************************************************************************************
* Function Name : mtr_main
* Description   : Main routine for Motor Control
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_main(void)
{
    /*** select user interfaces ***/
    if (g_u1_sw_userif != com_u1_sw_userif)
    {
        g_u1_sw_userif = com_u1_sw_userif;
        if (ICS_UI == g_u1_sw_userif)
        {
            g_u1_mode_system = g_u1_motor_status;
        }
    }

    if (BOARD_UI == g_u1_sw_userif)
    {
        board_ui();                           /* User interface control routine */
    }
    else if (ICS_UI == g_u1_sw_userif)
    {
        ics_ui();                             /* User interface using ICS */
    }
    else
    {
        /* Do Nothing */
    }
} /* End of function mtr_main() */

/***********************************************************************************************************************
* Function Name : board_ui
* Description   : User interface using board UI
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void board_ui(void)
{
    uint8_t u1_temp_sw_signal;

    /*======================*/
    /*     Mode control     */
    /*======================*/
    /* Get status of motor control system */
    p_motor_instance->p_api->statusGet(p_motor_instance->p_ctrl, &g_u1_motor_status);

    switch (g_u1_motor_status)
    {
        case MOTOR_STATUS_STOP:
            u1_temp_sw_signal = get_sw1();

            /* Check SW1 */
            if ((MTR_FLG_SET == mtr_remove_sw_chattering(u1_temp_sw_signal, SW1_ON))
             && (MTR_FLG_SET != g_u1_stop_req))
            {
                /* SW1:ON and Stop request:None => Start the motor */
                if (0U == u1_stop_wait_count)
                {
                    p_motor_instance->p_api->run(p_motor_instance->p_ctrl);
                }
            }
        break;

        case MOTOR_STATUS_RUN:
            u1_temp_sw_signal = get_sw1();

            /* Check SW1 */
            if ((MTR_FLG_SET == mtr_remove_sw_chattering(u1_temp_sw_signal, SW1_OFF))
             || (MTR_FLG_CLR != g_u1_stop_req))
            {
                /* SW1:OFF or Stop request:ON => Stop the motor */
                p_motor_instance->p_api->stop(p_motor_instance->p_ctrl);
                u1_stop_wait_count = MOTOR_STOP_WAIT_COUNT;
            }
        break;

        case MOTOR_STATUS_ERROR:
            if (MTR_FLG_CLR == get_sw1())
            {
                u1_temp_sw_signal = get_sw2();

                /* check SW2 & reset request flag */
                if ((SW_OFF == g_u1_reset_req)
                 && (MTR_FLG_SET == mtr_remove_sw_chattering(u1_temp_sw_signal, SW2_ON)))
                {
                    g_u1_reset_req = SW_ON;
                }
                else if ((SW_ON == g_u1_reset_req)
                      && (MTR_FLG_SET == mtr_remove_sw_chattering(u1_temp_sw_signal, SW2_OFF)))
                {
                    g_u1_reset_req = SW_OFF;
                    p_motor_instance->p_api->reset(p_motor_instance->p_ctrl);
                }
                else
                {
                    /* Do nothing */
                }
            }
        break;

        default:
            /* Do nothing */
        break;
    }

    /***** LED control *****/
    mtr_board_led_control(g_u1_motor_status);
} /* End of function board_ui */

/***********************************************************************************************************************
* Function Name : ics_ui
* Description   : User interface using ICS
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void ics_ui(void)
{
    uint8_t u1_temp;

    /*============================*/
    /*        Execute event       */
    /*============================*/
    u1_temp = com_u1_mode_system;

    if (g_u1_mode_system != u1_temp)
    {
        if (u1_temp > MOTOR_CTRL_EVENT_RESET)
        {
            com_u1_mode_system = g_u1_mode_system;
        }
        else
        {
            g_u1_mode_system = u1_temp;
            switch (g_u1_mode_system)
            {
                case MOTOR_CTRL_EVENT_STOP:
                    p_motor_instance->p_api->stop(p_motor_instance->p_ctrl);
                break;

                case MOTOR_CTRL_EVENT_RUN:
                    p_motor_instance->p_api->run(p_motor_instance->p_ctrl);
                break;

                case MOTOR_CTRL_EVENT_RESET:
                    p_motor_instance->p_api->reset(p_motor_instance->p_ctrl);
                break;

                default:
                    /* Do nothing */
                break;
            }
        }
    }

    p_motor_instance->p_api->statusGet(p_motor_instance->p_ctrl, &g_u1_motor_status);

    if (MOTOR_CTRL_EVENT_RESET == g_u1_mode_system)
    {
        if (MOTOR_STATUS_STOP == g_u1_motor_status)
        {
            /* Initialize private global variables for reset event */
            software_init();
        }
        else if (MOTOR_STATUS_ERROR == g_u1_motor_status)
        {
            g_u1_mode_system   = MOTOR_CTRL_EVENT_ERROR;
            com_u1_mode_system = MOTOR_CTRL_EVENT_ERROR;
        }
        else
        {
            /* Do nothing */
        }
    }

    /***** LED control *****/
    mtr_board_led_control(g_u1_motor_status);
} /* End of function ics_ui */

/***********************************************************************************************************************
* Function Name : software_init
* Description   : Initialize private global variables
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void software_init(void)
{
    g_u1_motor_status  = MOTOR_STATUS_STOP;
    g_u2_max_speed_rpm = MTR_MAX_SPEED_RPM;
    g_u1_mode_system   = MOTOR_CTRL_EVENT_STOP;
    g_u1_stop_req      = MTR_FLG_SET;
    g_u1_reset_req     = SW_OFF;

    /* ICS variables initialization */
    com_u1_mode_system = MOTOR_CTRL_EVENT_STOP;

    mtr_ics_variables_init();
    mtr_set_com_variables();
} /* End of function software_init */

/***********************************************************************************************************************
* Function Name : g_poe_overcurrent
* Description   : Overcurrent(POEG) Interrupt callback function
* Arguments     : p_args - Callback argument
* Return Value  : None
***********************************************************************************************************************/
void g_poe_overcurrent(poeg_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        R_POEG_Reset(g_poeg0.p_ctrl);
        p_motor_instance->p_api->errorSet(p_motor_instance->p_ctrl, MOTOR_ERROR_OVER_CURRENT_HW);
        g_u2_chk_error |= MOTOR_ERROR_OVER_CURRENT_HW;
    }
} /* End of function g_poe_overcurrent */
/***********************************************************************************************************************
* Function Name : make_cpkexp_init
* Description   : Initialize using for CPKEXP_ESCMCB
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void make_cpkexp_init (void)
{
    uint32_t P400_PSEL;
    uint32_t P401_PSEL;

    // CPKEXP-ECSMCB use P706/P707 as COM UART1
    // ICS2 Lib (version xxx) hard coded P400/P401 as COM UART1
    // Restore P400/P401 settings after ICS2 Init.

    P400_PSEL=R_PFS->PORT[4].PIN[0].PmnPFS;
    P401_PSEL=R_PFS->PORT[4].PIN[1].PmnPFS;
    ics2_init(ICS_BRR, ICS_INT_MODE);
    R_BSP_PinAccessEnable();
    R_PFS->PORT[4].PIN[0].PmnPFS=P400_PSEL;
    R_PFS->PORT[4].PIN[1].PmnPFS=P401_PSEL;

    R_BSP_PinAccessDisable();

}

/***********************************************************************************************************************
* Function Name : motor_fsp_init
* Description   : Initialize Motor FSP modules
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void motor_fsp_init(void)
{
    /* Open motor instance */
    p_motor_instance->p_api->open(p_motor_instance->p_ctrl, p_motor_instance->p_cfg);

    /* Open POEG instance */
    R_POEG_Open(g_poeg0.p_ctrl, g_poeg0.p_cfg);

    /* Set user configuration */
    g_user_motor_cfg = *(p_motor_instance->p_cfg);
    g_user_motor_sensorless_extended_cfg = *(motor_sensorless_extended_cfg_t *)g_user_motor_cfg.p_extend;
    g_user_motor_cfg.p_extend = &g_user_motor_sensorless_extended_cfg;
    p_user_motor_instance_ctrl = p_motor_instance->p_ctrl;
    p_user_motor_instance_ctrl->p_cfg = &g_user_motor_cfg;

    g_user_motor_speed_cfg = *(g_user_motor_cfg.p_motor_speed_instance->p_cfg);
    g_user_motor_speed_extended_cfg = *(motor_speed_extended_cfg_t *)g_user_motor_speed_cfg.p_extend;
    g_user_motor_speed_cfg.p_extend = &g_user_motor_speed_extended_cfg;

    g_user_motor_current_cfg = *(g_user_motor_cfg.p_motor_current_instance->p_cfg);
    g_user_motor_current_extended_cfg = *(motor_current_extended_cfg_t *)g_user_motor_current_cfg.p_extend;
    g_user_motor_current_motor_parameter = *(g_user_motor_current_extended_cfg.p_motor_parameter);
    g_user_motor_current_design_parameter = *(g_user_motor_current_extended_cfg.p_design_parameter);
    g_user_motor_current_extended_cfg.p_motor_parameter = &g_user_motor_current_motor_parameter;
    g_user_motor_current_extended_cfg.p_design_parameter = &g_user_motor_current_design_parameter;
    g_user_motor_current_cfg.p_extend = &g_user_motor_current_extended_cfg;

    g_user_motor_angle_cfg = *(g_user_motor_cfg.p_motor_current_instance->p_cfg->p_motor_angle_instance->p_cfg);
    g_user_motor_estimate_extended_cfg = *(motor_estimate_extended_cfg_t *)g_user_motor_angle_cfg.p_extend;
    g_user_motor_angle_cfg.p_extend = &g_user_motor_estimate_extended_cfg;

    g_user_motor_driver_cfg = *(g_user_motor_cfg.p_motor_current_instance->p_cfg->p_motor_driver_instance->p_cfg);
    g_user_motor_driver_extended_cfg = *(motor_driver_extended_cfg_t *)g_user_motor_driver_cfg.p_extend;
    g_user_motor_driver_cfg.p_extend = &g_user_motor_driver_extended_cfg;
} /* End of function motor_fsp_init */

/***********************************************************************************************************************
* Function Name : mtr_callback_event
* Description   : Callback function of motor control
* Arguments     : p_args - Callback argument
* Return Value  : None
***********************************************************************************************************************/
void mtr_callback_event(motor_callback_args_t * p_args)
{
    /* Only valid after the finish of initialization */
    if (1U == u1_init_flag)
    {
        switch (p_args->event)
        {
            case MOTOR_CALLBACK_EVENT_SPEED_FORWARD:
            {
                if (u1_stop_wait_count)
                {
                    u1_stop_wait_count--;
                }
                break;
            }

            case MOTOR_CALLBACK_EVENT_SPEED_BACKWARD:
            {
                mtr_set_reference();
                break;
            }

            case MOTOR_CALLBACK_EVENT_CURRENT_FORWARD:
            {
                if (MOTOR_STATUS_ERROR != g_u1_motor_status)
                {
                    p_motor_instance->p_api->errorCheck(p_motor_instance->p_ctrl, &g_u2_chk_error);
                }

                /* Speed reference (VR1) is sampled here after A/D conversion. */
                g_u2_vr1_ad = get_vr1();
                break;
            }

            case MOTOR_CALLBACK_EVENT_CURRENT_BACKWARD:
            {
                /* Interface with RMW */
                mtr_ics_interrupt_process();
                break;
            }

            default:
            {
                /* Do nothing */
            }
            break;
        }
    }
} /* End of function mtr_callback_event */

/***********************************************************************************************************************
* Function Name : mtr_board_led_control
* Description   : Set LED pattern depend on motor status
* Arguments     : u1_motor_status - Motor control status
* Return Value  : None
***********************************************************************************************************************/
static void mtr_board_led_control(uint8_t u1_motor_status)
{
    /***** LED control *****/
    switch (u1_motor_status)
    {
        case MOTOR_STATUS_STOP:
        {
            led1_off();                                     /* LED1 off */
            led2_off();                                     /* LED2 off */
            led3_off();                                     /* LED3 off */
            break;
        }
        case MOTOR_STATUS_RUN:
        {
            led1_on();                                      /* LED1 on */
            led2_off();                                     /* LED2 off */
            break;
        }
        case MOTOR_STATUS_ERROR:
        {
            led1_off();                                     /* LED1 off */
            led2_on();                                      /* LED2 on */
            led3_off();                                     /* LED3 off */
            break;
        }

        default:
        {
            led1_on();                                      /* LED1 on */
            led2_on();                                      /* LED2 on */
            led3_on();                                      /* LED3 on */
            break;
        }
    }
} /* End of function mtr_board_led_control */

/***********************************************************************************************************************
* Function Name : mtr_remove_sw_chattering
* Description   : Get switch status with removing chattering
* Arguments     : u1_sw - Board interface switch signal
*                 u1_on_off - Detected status (ON/OFF)
* Return Value  : u1_remove_chattering_flg - Detection result
***********************************************************************************************************************/
static uint8_t mtr_remove_sw_chattering(uint8_t u1_sw, uint8_t u1_on_off)
{
    uint8_t u1_remove_chattering_flg;

    u1_remove_chattering_flg = 0;
    if (u1_on_off == u1_sw)
    {
        g_u1_sw_cnt++;
        if (CHATTERING_CNT < g_u1_sw_cnt)
        {
            u1_remove_chattering_flg = MTR_FLG_SET;
            g_u1_sw_cnt = 0;
        }
    }
    else
    {
        g_u1_sw_cnt = 0;
    }

    return (u1_remove_chattering_flg);
} /* End of function mtr_remove_sw_chattering */

/***********************************************************************************************************************
* Function Name : mtr_set_reference
* Description   : Set reference of speed by VR1
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void mtr_set_reference(void)
{
    if (BOARD_UI == g_u1_sw_userif)
    {
        /*=============================*/
        /*      Set speed reference    */
        /*=============================*/
        g_f4_speed_ref = -(((float)g_u2_vr1_ad - ADJUST_OFFSET) * VR1_SCALING);
        if (g_f4_speed_ref >= MTR_MAX_SPEED_RPM)
        {
            g_f4_speed_ref = MTR_MAX_SPEED_RPM;
        }
        if (g_f4_speed_ref <= -MTR_MAX_SPEED_RPM)
        {
            g_f4_speed_ref = -MTR_MAX_SPEED_RPM;
        }

        p_motor_instance->p_api->speedSet(p_motor_instance->p_ctrl, g_f4_speed_ref);

        /* Below low limit speed, stop the motor */
        if ((g_f4_speed_ref > (-STOP_RPM)) && (g_f4_speed_ref < STOP_RPM))
        {
            g_u1_stop_req = MTR_FLG_SET;
        }
        else
        {
            g_u1_stop_req = MTR_FLG_CLR;
        }
    }
}   /* End of function mtr_set_reference */

/***********************************************************************************************************************
* Function Name : get_vr1
* Description   : Get A/D converted value of VR1
* Arguments     : None
* Return Value  : A/D converted value of VR1
***********************************************************************************************************************/
static uint16_t get_vr1(void)
{
    uint16_t ad_data;

    g_adc0.p_api->read(g_adc0.p_ctrl, MTR_ADCH_VR1, &ad_data);

    return (ad_data);
} /* End of function get_vr1 */

/***********************************************************************************************************************
* Function Name : get_sw1
* Description   : Get state of SW1
* Arguments     : None
* Return Value  : State of SW1
***********************************************************************************************************************/
static uint8_t get_sw1(void)
{
    uint8_t tmp_port;

    R_IOPORT_PinRead(&g_ioport_ctrl, MTR_PORT_SW1, &tmp_port);

    return (tmp_port);
} /* End of function get_sw1 */

/***********************************************************************************************************************
* Function Name : get_sw2
* Description   : Get state of SW2
* Arguments     : None
* Return Value  : State of SW2
***********************************************************************************************************************/
static uint8_t get_sw2(void)
{
    uint8_t tmp_port;

    R_IOPORT_PinRead(&g_ioport_ctrl, MTR_PORT_SW2, &tmp_port);

    return (tmp_port);
} /* End of function get_sw2 */

/***********************************************************************************************************************
* Function Name : led1_on
* Description   : Turn on LED1
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led1_on(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED1, MTR_LED_ON);
} /* End of function led1_on */

/***********************************************************************************************************************
* Function Name : led2_on
* Description   : Turn on LED2
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led2_on(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED2, MTR_LED_ON);
} /* End of function led2_on */

/***********************************************************************************************************************
* Function Name : led3_on
* Description   : Turn on LED3
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led3_on(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED3, MTR_LED_ON);
} /* End of function led3_on */

/***********************************************************************************************************************
* Function Name : led1_off
* Description   : Turn off LED1
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led1_off(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED1, MTR_LED_OFF);
} /* End of function led1_off */

/***********************************************************************************************************************
* Function Name : led2_off
* Description   : Turn off LED2
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led2_off(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED2, MTR_LED_OFF);
} /* End of function led2_off */

/***********************************************************************************************************************
* Function Name : led3_off
* Description   : Turn off LED3
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
static void led3_off(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, MTR_PORT_LED3, MTR_LED_OFF);
} /* End of function led3_off */
