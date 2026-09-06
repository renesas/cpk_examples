/*
 * perf_iwdt.c
 * Created on: Feb 12, 2026
 * Author: Ran Qingling
 */
#include <perf_iwdt.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include <perf_irq.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "common_utils.h"
#define EP_INFO     "This example project demonstrates the typical use of the IWDT HAL module.\r\n"\
                    "the user can press 'USER KEY' to initialize the IWDT and start the "USED_TIMER" timer. The IWDT\r\n"\
                    "counter is refreshed periodically every 1 second when the "USED_TIMER" timer expires.\r\n"\
                    "Refresh status will be printed every 2 seconds. Once the user Press 'MD KEY', the IWDT counter\r\n"\
                    "stops refreshing and resets the MCU.\r\n"

#define MENU        "\r\nPress 'USER KEY' to enable IWDT"\
                    "\r\nPress 'MD KEY' to stop refresh IWDT then the MCU will reset"
volatile uint32_t g_timer_underflow_counter = RESET_VALUE;
extern bool user_key, md_key;
CODE_AREA
void NMI_callback(wdt_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    /* Issue a software reset to reset the MCU */
    __NVIC_SystemReset();
}
CODE_AREA
void gpt_initialize(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    if (FSP_SUCCESS != err)
    {
        /* Print error on RTT Viewer console */
        printf ("\r\n** R_GPT_Open API failed **\r\n");
    }
}
CODE_AREA
void gpt_start(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_GPT_Start(&g_timer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        /* Print error on RTT Viewer console */
        printf ("\r\n** R_GPT_Start API failed **\r\n");
    }
}
CODE_AREA
void deinit_gpt_module(void)
{
    /* Variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;

    /* Close the GPT module */
    err=  R_GPT_Close(&g_timer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        /* Print error on RTT Viewer console */
        printf ("\r\n** R_GPT_Close API failed **\r\n");
    }
}

/*******************************************************************************************************************//**
 * @brief       This function is called when GPT timer's counter wrapped around.
 *              Refresh IWDT counter and toggle LED state.
 * @param[IN]   p_args   Callback function parameter data
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void gpt_callback(timer_callback_args_t *p_args)
{
    /* Variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;
    FSP_PARAMETER_NOT_USED(p_args);
    /* Refresh IWDT */
    err = R_IWDT_Refresh(&g_wdt0_ctrl);
    if (FSP_SUCCESS != err)
    {
        // turn on led to indicate the err.
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
        printf ("\r\n** R_IWDT_Refresh API failed **\r\n");
    }
    else
    {
        /* Counter is used to count the number of times GPT callback triggered */
        g_timer_underflow_counter++;
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_LOW);
    }
}


CODE_AREA
static void check_reset_status(void)
{
    /* Check if reset was caused by the IWDT/NMI */

    if (SYSTEM_RSTSR1_IWDTRF_DETECT_RESET == R_SYSTEM->RSTSR1_b.IWDTRF)
    {
        /* Clear the flag once read the value */
        R_SYSTEM->RSTSR1_b.IWDTRF = RESET_VALUE;
        printf ("\r\n************************ IWDT Reset detected ************************\r\n");
    }

    else if (SYSTEM_RSTSR1_SWRF_DETECT_RESET == R_SYSTEM->RSTSR1_b.SWRF)
    {
        /* Clear the flag once read the value */
        R_SYSTEM->RSTSR1_b.SWRF = RESET_VALUE;
        printf ("\r\n******************* NMI Software Reset detected *********************\r\n");
    }
    else
    {
        /* None */
    }
}
CODE_AREA
static void enable_iwdt_count_in_debug_mode(void)
{
    /* As per hardware manual's DBGREG module, section 2.6.4.2:-
     * Clear this bit to enable IWDT Reset/NMI in debug mode */
    R_DEBUG->DBGSTOPCR_b.DBGSTOP_IWDT = RESET_VALUE;
}
CODE_AREA
static fsp_err_t read_Input_from_irq(void)
{
    fsp_err_t err = FSP_SUCCESS;
    static bool b_is_iwdt_enable = false;
    if(user_key == 1)
    {
        /* Setting the variable */
        user_key=0;
        b_is_iwdt_enable = true;
        /* Enable IWDT to count and generate NMI or reset when the debugger (J-Link) is connected */
        enable_iwdt_count_in_debug_mode();
        err = R_IWDT_Open (&g_wdt0_ctrl, &g_wdt0_cfg);
        if (FSP_SUCCESS != err)
        {
            printf ("\r\n** R_IWDT_Open API failed **\r\n");
            return err;
        }
        /* Start GPT timer in Periodic mode */
        gpt_start();
        /* Print message to indicate the user about application status */
        printf("\r\nIWDT initialized, "USED_TIMER" Timer Started");
        printf("\r\nTo stop IWDT counter from refreshing, Press 'MD KEY'\r\n");
    }
    else if(md_key==1)
    {
        md_key=0;
        if (true == b_is_iwdt_enable)
        {
            /* Resetting the variable */
            b_is_iwdt_enable = false;
            /* GPT timer will stops running */
            err = R_GPT_Stop(&g_timer0_ctrl);
            if (FSP_SUCCESS != err)
            {
                /* Print Error on RTT Viewer console */
                printf("\r\n** R_"USED_TIMER"_Stop API failed **\r\n");
                return err;
            }
            printf("\r\n"USED_TIMER" timer stopped");
            printf("\r\n** Independent Watchdog Timer underflow in "IWDT_UNDERFLOW" seconds **\r\n");
        }
        else
        {
            printf (MENU);
        }
    }
    else
    {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return err;
}
CODE_AREA
void iwdt_test(void)
{
    fsp_pack_version_t version = {RESET_VALUE};
    fsp_err_t err = FSP_SUCCESS;
    /* Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);
    /* Example project information printed on the Console */
/*    printf (BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor,
               version.version_id_b.patch);*/
    printf (EP_INFO);

    /* Check whether reset is caused by IWDT */
    check_reset_status();

    gpt_initialize();
    irq_initialize ();
    /* Menu for the user selection */
    printf (MENU);
    md_key=0;
    user_key=0;
    while(true)
    {
        /* Process input only when the user has provided one */
        err = read_Input_from_irq();
        if (FSP_SUCCESS != err)
        {
            /* Close timer module */
            deinit_gpt_module();
            printf ("\r\nReturned Error Code: 0x%x  \r\n", (err));
        }
        /* For every 2 Second. RTT Viewer prints IWDT refresh message.
         * This is done to avoid the continuous print message on RTT Viewer */
        if (IWDT_REFRESH_COUNTER_VALUE == g_timer_underflow_counter)
        {
            g_timer_underflow_counter = RESET_VALUE;
            printf("\r\nIWDT counter Refreshed");
        }
    }

}
