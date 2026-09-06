/*
 * perf_elc.c
 * Created on: Feb 24, 2026
 * Author: Ran Qingling
 */
#include <perf_elc.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
/* Private functions */
static void elc_deinit(void);
static void gpt_deinit(gpt_instance_ctrl_t * p_ctrl);


/**********************************************************************************************************************
* Function implementations
**********************************************************************************************************************/
CODE_AREA
void elc_test(void)
{
    fsp_err_t err                               = FSP_SUCCESS;
    fsp_pack_version_t version                  = {RESET_VALUE};

    /* Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example project information printed on the Console */
    printf(BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor,\
              version.version_id_b.patch);
    printf(EP_INFO);

    /* Open ELC driver */
    err = R_ELC_Open(&g_elc_ctrl, &g_elc_cfg);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        /* ELC module initialize failed */
        printf("\r\n** R_ELC_Open API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    /* Enable links between modules:
     * For elc_ep run with GPT timer: 1. ELC Software event as start source for GPT1 and GPT0
     *                                2. GPT0 counter overflow event as stop source for GPT1
     */
    err = R_ELC_Enable(&g_elc_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        elc_deinit();
        printf("\r\n** R_ELC_Enable API failed **\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    /* Open Timer GPT1 in PWM mode */
    err = R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        elc_deinit();
        printf(OPEN_TIMER_PWM_FAIL);
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    /* Open Timer in One-Shot mode */
    err = R_GPT_Open(&g_timer0_ctrl,&g_timer0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        elc_deinit();
        gpt_deinit(&g_timer0_ctrl);
        printf(OPEN_TIMER_FAIL);
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    /* Enable start and stop sources for Timer in PWM mode */
    err = R_GPT_Enable(&g_timer1_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        elc_deinit();

        gpt_deinit(&g_timer0_ctrl);
        gpt_deinit(&g_timer1_ctrl);

        printf(ENABLE_TIMER_PULSE_FAIL);
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    /* Enable start source for Timer in One-Shot mode */
    err = R_GPT_Enable(&g_timer0_ctrl);


    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        elc_deinit();


        gpt_deinit(&g_timer0_ctrl);
        gpt_deinit(&g_timer1_ctrl);
        printf(ENABLE_TIMER_FAIL);
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    printf("\r\nInput any character. PWM output for 5 seconds and stops\r\n");
    while(1)
    {
        while (1) {
            R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_SECONDS);
            if (CONSOLE_HasData()) {
                break;
            }
            else {

            }
        }
        CONSOLE_Init();
        err = R_ELC_SoftwareEventGenerate(&g_elc_ctrl, ELC_SOFTWARE_EVENT_0);
        if (FSP_SUCCESS != err)
        {
            elc_deinit();
            printf("\r\n** R_ELC_SoftwareEventGenerate API failed **\r\n");
            printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
        }
        printf("\r\nInput any character. PWM output for 5 seconds and stops\r\n");
    }
}

/*******************************************************************************************************************//**
 * @brief       This function closes opened ELC module before the project ends up in an error trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
static void elc_deinit(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close ELC module */
    err = R_ELC_Close(&g_elc_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* ELC close failure message */
        APP_ERR_PRINT("** R_ELC_Close API failed **\r\n");
    }
}

#if (BSP_PERIPHERAL_TAU_PRESENT)
/*******************************************************************************************************************//**
 * @brief       This function closes opened TAU module before the project ends up in an error trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
static void tau_deinit(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close TAU Timer */
    err = R_TAU_Close(&g_timer1_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* TAU close failure message */
        APP_ERR_PRINT("** R_TAU_Close API failed **\r\n");
    }
}

/*******************************************************************************************************************//**
 * @brief       This function closes opened TAU PWM module before the project ends up in an error trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
static void tau_pwm_deinit(void)
{
    fsp_err_t err = FSP_SUCCESS;
    /* Close TAU Timer */
    err = R_TAU_PWM_Close(&g_timer0_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* TAU PWM close failure message */
        APP_ERR_PRINT("** R_TAU_PWM_Close API failed **\r\n");
    }
}
#else
/*******************************************************************************************************************//**
 * @brief       This function closes opened GPT module before the project ends up in an error trap.
 * @param[IN]   p_ctrl    Pointer to instance control.
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
static void gpt_deinit(gpt_instance_ctrl_t * p_ctrl)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close GPT timer */
    err = R_GPT_Close(p_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* GPT close failure message */
        APP_ERR_PRINT("** R_GPT_Close API failed **\r\n");
    }
}
#endif /* BSP_PERIPHERAL_TAU_PRESENT */
