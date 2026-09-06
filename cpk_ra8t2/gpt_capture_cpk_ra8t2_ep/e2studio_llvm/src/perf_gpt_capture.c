/*
 * perf_gpt.c
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */
#include <perf_gpt_capture.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"


#define BUFF_SIZE       (30U)
#define EP_INFO         "\r\nThe EP demonstrates the functionality of GPT Input capture module."\
                        "\r\nGPT5 is used to generate periodic pulses of 500msec duration and"\
                        "\r\nprovided as input to GPT Input capture (GPT3). GPT3 counts the event"\
                        "\r\npulse received at its input. Based on the period and capture event,"\
                        "\r\nConnect (P105)CN4:20 to (P205)CN4:3,continues to test.\r\n"\
                        "\r\nthe time period of pulse is calculated and displayed on RTT Viewer/PuTTY.\r\n"


/* Global variables */
volatile bool b_start_measurement   = false;
volatile uint64_t g_capture_count            = RESET_VALUE;
volatile uint32_t g_capture_overflow         = RESET_VALUE;
CODE_AREA
void gpt_deinit(timer_ctrl_t * p_ctrl)
{
    fsp_err_t err = FSP_SUCCESS;
    /* De-initialize GPT instances */
    err = R_GPT_Close(p_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_GPT_Close API failed **\r\n");
    }
}

 /*******************************************************************************************************************//**
 * @brief       User defined callback
 * @param[in]   p_args
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void input_capture_user_callback(timer_callback_args_t *p_args)
{
    /* Check for the event */
    switch(p_args->event)
    {
        case TIMER_EVENT_CAPTURE_A :
        {
            /* Capture the count in a variable */
            g_capture_count     = p_args->capture;
            /* Set start measurement */
            b_start_measurement = true;
            break;
        }
        case TIMER_EVENT_CYCLE_END:
        {
            /* An overflow occurred during capture */
            g_capture_overflow++;
            break;
        }
        default:
        {
            break;
        }
    }
}
CODE_AREA
void gpt_capture_test(void)
{
    fsp_err_t err                   = FSP_SUCCESS;
    fsp_pack_version_t version      = {RESET_VALUE};
    char timer_buffer[BUFF_SIZE]    = {RESET_VALUE};
    timer_info_t info               = {.clock_frequency = RESET_VALUE, .count_direction = RESET_VALUE,\
                                       .period_counts =  RESET_VALUE};
    float pulse_time                = 0.0f;

    printf(EP_INFO);

    /* Open GPT instance as a periodic timer */
    err = R_GPT_Open(&g_timer_ctrl, &g_timer_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_GPT_Open API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", err);
    }

    /* Open GPT instance as input capture */
    err = R_GPT_Open(&g_input_capture_ctrl, &g_input_capture_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        gpt_deinit(&g_timer_ctrl);
        printf("\r\n** R_GPT_Open API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", err);
    }

    /* Enable GPT input capture */
    err = R_GPT_Enable(&g_input_capture_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        gpt_deinit(&g_timer_ctrl);
        gpt_deinit(&g_input_capture_ctrl);
        printf("\r\n** R_GPT_Enable API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", err);
    }

    /* Start GPT timer in periodic mode */
    err = R_GPT_Start(&g_timer_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        gpt_deinit(&g_timer_ctrl);
        gpt_deinit(&g_input_capture_ctrl);
        printf("\r\n** R_GPT_Start API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", err);
    }

    while(true)
    {
        /* Check for the flag from ISR callback */
        if (true == b_start_measurement)
        {
            /* Reset the flag */
            b_start_measurement = false;
            /* Get the period count and clock frequency */
            err =  R_GPT_InfoGet(&g_input_capture_ctrl, &info);
            /* Handle error */
            if (FSP_SUCCESS != err)
            {
                gpt_deinit(&g_timer_ctrl);
                gpt_deinit(&g_input_capture_ctrl);
                printf("\r\n** R_GPT_InfoGet API failed **\r\n");
                printf("\r\nReturned Error Code: 0x%x  \r\n", err);
            }

            //g_capture_count = (info.period_counts * g_capture_overflow) + g_capture_count;

            /* Calculate the pulse time */
            pulse_time =(float)(((float)g_capture_count)/((float)info.clock_frequency));

            /* Reset the variables */
            g_capture_count = RESET_VALUE;
            g_capture_overflow  = RESET_VALUE;
            sprintf (timer_buffer, "%.05f", pulse_time);
            printf("\r\nPulse width measurement value(in second) - %s\r\n", timer_buffer);

        }
    }
}
