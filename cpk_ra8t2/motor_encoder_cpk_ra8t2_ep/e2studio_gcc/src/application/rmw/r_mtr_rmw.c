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
* File Name   : r_mtr_rmw.c
* Description : Processes of a user interface (tool)
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version
*         : 19.01.2024 1.00
***********************************************************************************************************************/

/***********************************************************************************************************************
* Includes <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include <stdint.h>
#include "mtr_main.h"
#include "r_mtr_rmw.h"
#include "hal_data.h"
#include "r_mtr_motor_parameter.h"
#include "r_mtr_control_parameter.h"
static uint8_t u1_cnt_ics;                      /* Counter for period of calling "scope_watchpoint" */

float    g_f4_id_ref_monitor;                   /* The reference d-axis current value [A] */
float    g_f4_id_ad_monitor;                    /* The d-axis current value [A] */
float    g_f4_iq_ref_monitor;                   /* The reference q-axis current value [A] */
float    g_f4_iq_ad_monitor;                    /* The q-axis current value [A] */
float    g_f4_iu_ad_monitor;                    /* U-phase current value [A] */
float    g_f4_iv_ad_monitor;                    /* V-phase current value [A] */
float    g_f4_iw_ad_monitor;                    /* W-phase current value [A] */
float    g_f4_vdc_ad_monitor;                   /* Main Line Voltage[V] */
float    g_f4_vd_ref_monitor;                   /* The reference d-axis voltage value [V] */
float    g_f4_vq_ref_monitor;                   /* The reference q-axis voltage value [V] */
float    g_f4_refu_monitor;                     /* U-phase reference voltage value [V] */
float    g_f4_refv_monitor;                     /* V-phase reference voltage value [V] */
float    g_f4_refw_monitor;                     /* W-phase reference voltage value [V] */
float    g_f4_ref_pos_rad_ctrl_monitor;         /* Angle of rotor [rad] */
float    g_f4_pos_rad_monitor;                  /* Angle of rotor [rad] */
float    g_f4_speed_rad_monitor;                /* Rotational speed [rad/sec] */
float    g_f4_ref_speed_rad_ctrl_monitor;       /* Speed reference for speed PI control [rad/sec] */
uint8_t  g_u1_state_id_ref_monitor;             /* The d-axis current command status */
uint8_t  g_u1_state_iq_ref_monitor;             /* The q-axis current command status */
uint8_t  g_u1_state_speed_ref_monitor;          /* The speed command status */
float    g_f4_speed_kp_monitor;                 /* Kp for speed loop */
float    g_f4_speed_ki_monitor;                 /* Ki for speed loop */
float    g_f4_current_kp_d_monitor;             /* Kp for d-axis current loop */
float    g_f4_current_ki_d_monitor;             /* Ki for d-axis current loop */
float    g_f4_current_kp_q_monitor;             /* Kp for q-axis current loop */
float    g_f4_current_ki_q_monitor;             /* Ki for q-axis current loop */
float    g_f4_pos_err_rad_monitor;              /* Position error [radian] */
float    g_f4_ref_pos_rad_monitor;              /* Position reference [radian] */
float    g_f4_ipd_ref_speed_rad_monitor;        /* Speed reference at I-PD control [rad/sec] */
float    g_f4_pi_param_f_err_monitor;           /* PI error */
float    g_f4_speed_rpm_monitor;                /* Rotational speed [rpm] */
float    g_f4_ref_speed_rpm_ctrl_monitor;       /* Speed reference [rpm] */
float    g_f4_pos_degree_monitor;               /* Position value [degree] */
float    g_f4_ref_pos_degree_ctrl_monitor;      /* Position reference [degree] */

uint8_t  com_u1_ctrl_loop_mode;                 /* Loop mode select (Position/Speed) */
float    com_f4_ref_speed_rpm = 0.0F;           /* Motor speed reference [rpm] (mechanical) */
int16_t  com_s2_ref_position_deg = 0;           /* motor position reference [degree] (mechanical) */
uint16_t com_u2_mtr_pp;                         /* Pole pairs */
float    com_f4_mtr_r;                          /* Resistance [ohm] */
float    com_f4_mtr_ld;                         /* D-axis inductance [H] */
float    com_f4_mtr_lq;                         /* Q-axis inductance [H] */
float    com_f4_mtr_m;                          /* Permanent magnetic flux [Wb] */
float    com_f4_mtr_j;                          /* Rotor inertia [kgm^2] */
float    com_f4_current_omega;                  /* Natural frequency for current loop [Hz] */
float    com_f4_current_zeta;                   /* Damping ratio for current loop */
float    com_f4_speed_omega;                    /* Natural frequency for speed loop [Hz] */
float    com_f4_speed_zeta;                     /* Damping ratio for speed loop */
float    com_f4_pos_omega;                      /* Natural frequency for position loop [Hz] */
float    com_f4_sob_omega;                      /* Natural frequency for speed observer [Hz] */
float    com_f4_sob_zeta;                       /* Damping ratio for speed observer */
float    com_f4_ol_ref_id;                      /* Refernce d-axis current at open-loop rotation [A] */
float    com_f4_id_up_time;                     /* Time for d-axis current piling up */
float    com_f4_speed_limit_rpm;                /* Limit of rotation speed [rpm] */
uint16_t com_u2_pos_dead_band;                  /* Position dead band */
uint16_t com_u2_pos_band_limit;                 /* Limit of position band */
uint16_t com_u2_interval_time;
float    com_f4_accel_time;                     /* Accelaration time */
float    com_f4_speed_rate_limit;               /* Step of speed piling up */
float    com_f4_max_speed_rpm;                  /* Maximum rotational speed [rpm] */
float    com_f4_overcurrent_limit;              /* Limit of over current [A] */
float    com_f4_iq_limit;                       /* Limit of q-axis current [A] */
float    com_f4_speed_ob_hpf_omega;             /* Natural frequency for high pass filter of speed observer */
uint16_t com_u2_encd_cpr;                       /* Count per rotation of the encoder sensor */

uint8_t  com_u1_enable_write = 0U;              /* ICS write enable flag */
uint8_t  g_u1_enable_write = 0U;                /* ICS write enable flag */

extern   uint8_t g_u1_loop_mode;                /* Current loop mode (Position/Speed) */
extern   uint8_t g_u1_mode_system;              /* Current mode of system (Inactive/Active/Error) */

extern   const motor_instance_t  *p_motor_instance;
extern   motor_cfg_t g_user_motor_cfg;
extern   motor_encoder_extended_cfg_t g_user_motor_encoder_extended_cfg;
extern   motor_speed_cfg_t g_user_motor_speed_cfg;
extern   motor_speed_extended_cfg_t g_user_motor_speed_extended_cfg;
extern   motor_position_cfg_t g_user_motor_position_cfg;
extern   motor_position_extended_cfg_t g_user_motor_position_extended_cfg;
extern   motor_current_cfg_t g_user_motor_current_cfg;
extern   motor_current_extended_cfg_t g_user_motor_current_extended_cfg;
extern   motor_angle_cfg_t g_user_motor_angle_cfg;
extern   motor_sense_encoder_extended_cfg_t g_user_motor_sense_encoder_extended_cfg;
extern   uint16_t g_u2_max_speed_rpm;

static   motor_speed_instance_t const    *p_speed_instance;
static   motor_current_instance_t const  *p_current_instance;
static   motor_angle_instance_t const    *p_angle_instance;
static   motor_position_instance_t const *p_position_instance;
static   uint8_t  g_u1_trig_enable_write = MTR_FLG_CLR;

/***********************************************************************************************************************
* Function Name : mtr_set_com_variables
* Description   : Set com variables
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_set_com_variables(void)
{
    motor_speed_instance_ctrl_t *p_speed_ctrl = (motor_speed_instance_ctrl_t *)p_speed_instance->p_ctrl;
    motor_speed_position_data_t temp_position_data;

    if (MOTOR_ENCODER_CTRL_EVENT_STOP == g_u1_mode_system)
    {
        p_position_instance->p_api->controlModeSet(p_position_instance->p_ctrl, 0);
    }

    if (com_u1_enable_write == g_u1_enable_write)
    {
        g_u1_loop_mode = com_u1_ctrl_loop_mode;

        /* Speed control */
        if (g_u1_loop_mode == LOOP_SPEED)
        {
            if (g_user_motor_sense_encoder_extended_cfg.loop_mode == LOOP_POSITION)
            {
                g_user_motor_sense_encoder_extended_cfg.loop_mode = MOTOR_SENSE_ENCODER_LOOP_SPEED;
                p_angle_instance->p_api->parameterUpdate(p_angle_instance->p_ctrl, &g_user_motor_angle_cfg);
            }

            if (com_f4_ref_speed_rpm > (float)g_u2_max_speed_rpm)
            {
                com_f4_ref_speed_rpm = (float)g_u2_max_speed_rpm;
            }
            else if (com_f4_ref_speed_rpm < -(float)g_u2_max_speed_rpm)
            {
                com_f4_ref_speed_rpm = -(float)g_u2_max_speed_rpm;
            }
            p_motor_instance->p_api->speedSet(p_motor_instance->p_ctrl, com_f4_ref_speed_rpm);

            temp_position_data.e_step_mode= MOTOR_SPEED_STEP_DISABLE;
            temp_position_data.e_loop_mode = MOTOR_SPEED_LOOP_MODE_SPEED;
            temp_position_data.position_reference_degree = 0;
            p_motor_instance->p_api->positionSet(p_motor_instance->p_ctrl, &temp_position_data);
        }
        /* Position control */
        else if (g_u1_loop_mode == LOOP_POSITION)
        {
            if (g_user_motor_sense_encoder_extended_cfg.loop_mode == LOOP_SPEED)
            {
                g_user_motor_sense_encoder_extended_cfg.loop_mode = MOTOR_SENSE_ENCODER_LOOP_POSITION;
                p_angle_instance->p_api->parameterUpdate(p_angle_instance->p_ctrl, &g_user_motor_angle_cfg);
            }
            p_motor_instance->p_api->speedSet(p_motor_instance->p_ctrl, 0.0F);
            temp_position_data.e_step_mode = MOTOR_SPEED_STEP_DISABLE;
            temp_position_data.e_loop_mode = MOTOR_SPEED_LOOP_MODE_POSITION;
            temp_position_data.position_reference_degree = com_s2_ref_position_deg;
            p_motor_instance->p_api->positionSet(p_motor_instance->p_ctrl, &temp_position_data);
        }
        else
        {
            /* Do nothing */
        }

        g_user_motor_encoder_extended_cfg.f_overspeed_limit                    = com_f4_speed_limit_rpm;
        g_user_motor_encoder_extended_cfg.f_overcurrent_limit
            = com_f4_overcurrent_limit * MTR_SQRT_2 * MTR_OVERCURRENT_MARGIN_MULT;
        g_user_motor_speed_extended_cfg.mtr_param.u2_mtr_pp                    = com_u2_mtr_pp;
        g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_r                     = com_f4_mtr_r;
        g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_ld                    = com_f4_mtr_ld;
        g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_lq                    = com_f4_mtr_lq;
        g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_m                     = com_f4_mtr_m;
        g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_j                     = com_f4_mtr_j;
        g_user_motor_speed_extended_cfg.d_param.f_speed_omega                  = com_f4_speed_omega;
        g_user_motor_speed_extended_cfg.d_param.f_speed_zeta                   = com_f4_speed_zeta;
        g_user_motor_speed_extended_cfg.d_param.f_observer_omega               = com_f4_sob_omega;
        g_user_motor_speed_extended_cfg.d_param.f_observer_zeta                = com_f4_sob_zeta;
        g_user_motor_speed_extended_cfg.ol_param.f4_ol_id_ref                  = com_f4_ol_ref_id;
        g_user_motor_speed_extended_cfg.ol_param.f4_ol_id_up_step              = com_f4_ol_ref_id / com_f4_id_up_time;
        g_user_motor_speed_extended_cfg.f_limit_speed_change                   = com_f4_speed_rate_limit * com_u2_mtr_pp;
        g_user_motor_speed_extended_cfg.f_iq_limit                             = com_f4_iq_limit;
        g_user_motor_speed_extended_cfg.f_maximum_speed_rpm                    = com_f4_max_speed_rpm * com_u2_mtr_pp;
        p_speed_ctrl->st_observer.f4_hpf_omega                                 = com_f4_speed_ob_hpf_omega;
        g_user_motor_current_extended_cfg.p_motor_parameter->u2_mtr_pp         = com_u2_mtr_pp;
        g_user_motor_current_extended_cfg.p_motor_parameter->f4_mtr_r          = com_f4_mtr_r;
        g_user_motor_current_extended_cfg.p_motor_parameter->f4_mtr_ld         = com_f4_mtr_ld;
        g_user_motor_current_extended_cfg.p_motor_parameter->f4_mtr_lq         = com_f4_mtr_lq;
        g_user_motor_current_extended_cfg.p_motor_parameter->f4_mtr_m          = com_f4_mtr_m;
        g_user_motor_current_extended_cfg.p_motor_parameter->f4_mtr_j          = com_f4_mtr_j;
        g_user_motor_current_extended_cfg.p_design_parameter->f_current_omega  = com_f4_current_omega;
        g_user_motor_current_extended_cfg.p_design_parameter->f_current_zeta   = com_f4_current_zeta;
        g_user_motor_sense_encoder_extended_cfg.encoder_config.u2_cpr          = com_u2_encd_cpr;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.u2_mtr_pp      = com_u2_mtr_pp;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.f4_mtr_r       = com_f4_mtr_r;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.f4_mtr_ld      = com_f4_mtr_ld;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.f4_mtr_lq      = com_f4_mtr_lq;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.f4_mtr_m       = com_f4_mtr_m;
        g_user_motor_sense_encoder_extended_cfg.st_motor_params.f4_mtr_j       = com_f4_mtr_j;
        g_user_motor_position_extended_cfg.u2_pos_dead_band                    = com_u2_pos_dead_band;
        g_user_motor_position_extended_cfg.u2_pos_band_limit                   = com_u2_pos_band_limit;
        g_user_motor_position_extended_cfg.st_motor.u2_mtr_pp                  = com_u2_mtr_pp;
        g_user_motor_position_extended_cfg.st_motor.f4_mtr_r                   = com_f4_mtr_r;
        g_user_motor_position_extended_cfg.st_motor.f4_mtr_ld                  = com_f4_mtr_ld;
        g_user_motor_position_extended_cfg.st_motor.f4_mtr_lq                  = com_f4_mtr_lq;
        g_user_motor_position_extended_cfg.st_motor.f4_mtr_m                   = com_f4_mtr_m;
        g_user_motor_position_extended_cfg.st_motor.f4_mtr_j                   = com_f4_mtr_j;
        g_user_motor_position_extended_cfg.f_pos_omega                         = com_f4_pos_omega;
        g_user_motor_position_extended_cfg.st_profiling_set.u2_interval_time   = com_u2_interval_time;
        g_user_motor_position_extended_cfg.st_profiling_set.f4_accel_time      = com_f4_accel_time;
        g_user_motor_position_extended_cfg.st_profiling_set.f4_accel_max_speed = com_f4_max_speed_rpm;

        g_u1_trig_enable_write = MTR_FLG_SET;
        g_u1_enable_write ^= 1;                         /* Change every time 0 and 1 */
    }
}

/***********************************************************************************************************************
* Function Name : mtr_ics_variables_init
* Description   : Initialize valiables for Analyzer interface
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_ics_variables_init(void)
{
    p_speed_instance    = g_user_motor_cfg.p_motor_speed_instance;
    p_current_instance  = g_user_motor_cfg.p_motor_current_instance;
    p_angle_instance    = g_user_motor_current_cfg.p_motor_angle_instance;
    p_position_instance = g_user_motor_speed_cfg.p_position_instance;

    motor_speed_instance_ctrl_t *p_speed_ctrl = (motor_speed_instance_ctrl_t *)p_speed_instance->p_ctrl;
    g_u1_enable_write = 0;
    com_u1_enable_write = 0;

#if MTR_MOTOR_PARAMETER
    com_u2_mtr_pp            = MP_POLE_PAIRS;
    com_f4_mtr_r             = MP_RESISTANCE;
    com_f4_mtr_ld            = MP_D_INDUCTANCE;
    com_f4_mtr_lq            = MP_Q_INDUCTANCE;
    com_f4_mtr_m             = MP_MAGNETIC_FLUX;
    com_f4_mtr_j             = MP_ROTOR_INERTIA;
    com_f4_overcurrent_limit = MP_NOMINAL_CURRENT_RMS;
    com_f4_iq_limit          = MP_NOMINAL_CURRENT_RMS;
#else
    com_u2_mtr_pp            = g_user_motor_speed_extended_cfg.mtr_param.u2_mtr_pp;
    com_f4_mtr_r             = g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_r;
    com_f4_mtr_ld            = g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_ld;
    com_f4_mtr_lq            = g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_lq;
    com_f4_mtr_m             = g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_m;
    com_f4_mtr_j             = g_user_motor_speed_extended_cfg.mtr_param.f4_mtr_j;
    com_f4_overcurrent_limit = g_user_motor_encoder_extended_cfg.f_overcurrent_limit
                                / (MTR_SQRT_2 * MTR_OVERCURRENT_MARGIN_MULT);
    com_f4_iq_limit          = g_user_motor_speed_extended_cfg.f_iq_limit;
#endif

#if MTR_CONTROL_PARAMETER
    com_f4_current_omega       = CP_CURRENT_OMEGA;
    com_f4_current_zeta        = CP_CURRENT_ZETA;
    com_f4_speed_omega         = CP_SPEED_OMEGA;
    com_f4_speed_zeta          = CP_SPEED_ZETA;
    com_f4_pos_omega           = CP_POS_OMEGA;
    com_f4_sob_omega           = CP_SOB_OMEGA;
    com_f4_sob_zeta            = CP_SOB_ZETA;
    com_f4_max_speed_rpm       = CP_MAX_SPEED_RPM;
    com_f4_speed_limit_rpm     = CP_OVERSPEED_LIMIT_RPM;
    com_f4_ol_ref_id           = CP_OL_ID_REF;
#else
    com_f4_current_omega       = g_user_motor_current_extended_cfg.p_design_parameter->f_current_omega;
    com_f4_current_zeta        = g_user_motor_current_extended_cfg.p_design_parameter->f_current_zeta;
    com_f4_speed_omega         = g_user_motor_speed_extended_cfg.d_param.f_speed_omega;
    com_f4_speed_zeta          = g_user_motor_speed_extended_cfg.d_param.f_speed_zeta;
    com_f4_pos_omega           = g_user_motor_position_extended_cfg.f_pos_omega;
    com_f4_sob_omega           = g_user_motor_speed_extended_cfg.d_param.f_observer_omega;
    com_f4_sob_zeta            = g_user_motor_speed_extended_cfg.d_param.f_observer_zeta;
    com_f4_max_speed_rpm       = g_user_motor_speed_extended_cfg.f_maximum_speed_rpm
                                    / g_user_motor_speed_extended_cfg.mtr_param.u2_mtr_pp;
    com_f4_speed_limit_rpm     = g_user_motor_encoder_extended_cfg.f_overspeed_limit;
    com_f4_ol_ref_id           = g_user_motor_speed_extended_cfg.ol_param.f4_ol_id_ref;
#endif

    com_u1_ctrl_loop_mode = CONFIG_LOOP_MODE;
    com_u2_pos_dead_band = g_user_motor_position_extended_cfg.u2_pos_dead_band;
    com_u2_pos_band_limit = g_user_motor_position_extended_cfg.u2_pos_band_limit;
    com_u2_encd_cpr = g_user_motor_sense_encoder_extended_cfg.encoder_config.u2_cpr;
    com_u2_interval_time = g_user_motor_position_extended_cfg.st_profiling_set.u2_interval_time;
    com_f4_accel_time = g_user_motor_position_extended_cfg.st_profiling_set.f4_accel_time;
    com_f4_id_up_time = com_f4_ol_ref_id / g_user_motor_speed_extended_cfg.ol_param.f4_ol_id_up_step;
    com_f4_speed_rate_limit = g_user_motor_speed_extended_cfg.f_limit_speed_change / com_u2_mtr_pp;
    com_f4_speed_ob_hpf_omega = p_speed_ctrl->st_observer.f4_hpf_omega;
}

/***********************************************************************************************************************
* Function Name : mtr_ics_interrupt_process
* Description   : Interrupt process to reflect data into RMW (called at current backward event)
* Arguments     : None
* Return Value  : None
***********************************************************************************************************************/
void mtr_ics_interrupt_process(void)
{
    motor_current_instance_ctrl_t *p_current_ctrl   = (motor_current_instance_ctrl_t *)p_current_instance->p_ctrl;
    motor_speed_instance_ctrl_t *p_speed_ctrl       = (motor_speed_instance_ctrl_t *)p_speed_instance->p_ctrl;
    motor_position_instance_ctrl_t *p_position_ctrl = (motor_position_instance_ctrl_t *)p_position_instance->p_ctrl;

    // Reflect user settig parameters (com_xxx)
    mtr_set_com_variables();

    /* Decimation of ICS call */
    u1_cnt_ics++;
    if (MTR_ICS_DECIMATION <= u1_cnt_ics)
    {
        /* Set monitor variables */
        g_f4_id_ref_monitor              = p_current_ctrl->f_id_ref;
        g_f4_id_ad_monitor               = p_current_ctrl->f_id_ad;
        g_f4_iq_ref_monitor              = p_current_ctrl->f_iq_ref;
        g_f4_iq_ad_monitor               = p_current_ctrl->f_iq_ad;
        g_f4_iu_ad_monitor               = p_current_ctrl->f_iu_ad;
        g_f4_iv_ad_monitor               = p_current_ctrl->f_iv_ad;
        g_f4_iw_ad_monitor               = p_current_ctrl->f_iw_ad;
        g_f4_vdc_ad_monitor              = p_current_ctrl->st_vcomp.f_vdc;
        g_f4_vd_ref_monitor              = p_current_ctrl->f_vd_ref;
        g_f4_vq_ref_monitor              = p_current_ctrl->f_vq_ref;
        g_f4_refu_monitor                = p_current_ctrl->f_refu;
        g_f4_refv_monitor                = p_current_ctrl->f_refv;
        g_f4_refw_monitor                = p_current_ctrl->f_refw;
        g_f4_speed_rad_monitor           = p_current_ctrl->f_speed_rad;
        g_f4_current_kp_d_monitor        = p_current_ctrl->st_pi_id.f_kp;
        g_f4_current_ki_d_monitor        = p_current_ctrl->st_pi_id.f_ki;
        g_f4_current_kp_q_monitor        = p_current_ctrl->st_pi_iq.f_kp;
        g_f4_current_ki_q_monitor        = p_current_ctrl->st_pi_iq.f_ki;

        g_f4_ref_speed_rad_ctrl_monitor  = p_speed_ctrl->f_ref_speed_rad_ctrl;
        g_u1_state_id_ref_monitor        = p_speed_ctrl->u1_state_id_ref;
        g_u1_state_iq_ref_monitor        = p_speed_ctrl->u1_state_iq_ref;
        g_u1_state_speed_ref_monitor     = p_speed_ctrl->u1_state_speed_ref;
        g_f4_pi_param_f_err_monitor      = p_speed_ctrl->pi_param.f_err;
        g_f4_speed_kp_monitor            = p_speed_ctrl->pi_param.f_kp;
        g_f4_speed_ki_monitor            = p_speed_ctrl->pi_param.f_ki;

        g_f4_ref_pos_rad_ctrl_monitor    = p_position_ctrl->st_variable.f4_ref_pos_rad_ctrl;
        g_f4_pos_rad_monitor             = p_position_ctrl->st_variable.f4_pos_rad;
        g_f4_pos_err_rad_monitor         = p_position_ctrl->st_variable.f4_pos_err_rad;
        g_f4_ref_pos_rad_monitor         = p_position_ctrl->st_variable.f4_ref_pos_rad;
        g_f4_ipd_ref_speed_rad_monitor   = p_position_ctrl->st_ipd.f4_ipd_ref_speed_rad;

        g_f4_speed_rpm_monitor = g_f4_speed_rad_monitor * MTR_RAD_RPM
                                    / (float)g_user_motor_current_extended_cfg.p_motor_parameter->u2_mtr_pp;
        g_f4_ref_speed_rpm_ctrl_monitor = g_f4_ref_speed_rad_ctrl_monitor * MTR_RAD_RPM
                                            / (float)g_user_motor_current_extended_cfg.p_motor_parameter->u2_mtr_pp;
        g_f4_pos_degree_monitor = g_f4_pos_rad_monitor * MTR_RAD_DEGREE;
        g_f4_ref_pos_degree_ctrl_monitor = g_f4_ref_pos_rad_ctrl_monitor * MTR_RAD_DEGREE;

        u1_cnt_ics = 0;

        /* Call ICS */
        ics2_watchpoint();
    }

    /* When trigger enable is set, reflect user settings */
    if (MTR_FLG_SET == g_u1_trig_enable_write)
    {
        p_speed_instance->p_api->parameterUpdate(p_speed_instance->p_ctrl, &g_user_motor_speed_cfg);
        p_current_instance->p_api->parameterUpdate(p_current_instance->p_ctrl, &g_user_motor_current_cfg);
        p_position_instance->p_api->parameterUpdate(p_position_instance->p_ctrl, &g_user_motor_position_cfg);
        if (MOTOR_ENCODER_CTRL_EVENT_STOP == g_u1_mode_system)
        {
            p_angle_instance->p_api->parameterUpdate(p_angle_instance->p_ctrl, &g_user_motor_angle_cfg);
        }
        g_u1_trig_enable_write = MTR_FLG_CLR;
    }
} /* End of function mtr_ics_interrupt_process */
