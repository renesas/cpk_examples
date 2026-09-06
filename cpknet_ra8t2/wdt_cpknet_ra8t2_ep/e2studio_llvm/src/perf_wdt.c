/*
 * perf_iwdt.c
 * Created on: Feb 12, 2026
 * Author: Ran Qingling
 */
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include <perf_irq.h>
#include <perf_wdt.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "common_utils.h"
#define EP_INFO     "\r\nThis example project demonstrates the typical use of the WDT HAL module.\r\n"\
                    "the user can press 'USER KEY' to initialize the WDT and start the gpt timer. \r\n"\
                    "The WDT counter is refreshed periodically every 1 second when the gpt timer expires.\r\n"\
                    "Refresh status will be printed every 2 seconds. Once the user Press 'MD KEY', \r\n"\
                    "the WDT counter stops refreshing and resets the MCU.\r\n"

#define MENU        "\r\nPress 'USER KEY' to enable WDT"\
                    "\r\nPress 'MD KEY' to stop refresh WDT then the MCU will reset\r\n"
volatile uint32_t g_timer_underflow_counter = RESET_VALUE;
extern bool user_key, md_key;
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
    /* Refresh WDT */
    err = R_WDT_Refresh(&g_wdt_ctrl);
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

    /* Check if reset was caused by the WDT? If it was, turn on LED to indicate WDT reset triggered */
    if (SYSTEM_RSTSR1_WDTRF_DETECT_WDT_RESET == R_SYSTEM->RSTSR1_b.WDTRF)
    {
        /* Clear the flag once read the value */
        R_SYSTEM->RSTSR1_b.WDTRF = RESET_VALUE;
        printf ("\r\n************************ WDT Reset detected ************************\r\n");
    }
    else
    {
        /* None */
    }
}
CODE_AREA
static void enable_wdt_count_in_debug_mode(void)
{
    /* As per hardware manual's DBGREG module,
     * section 2.9.5.3:- Clear this bit to enable WDT Reset/NMI in debug mode */
    R_DEBUG->DBGSTOPCR_b.DBGSTOP_WDT = RESET_VALUE;
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
        enable_wdt_count_in_debug_mode();
        /* Open WDT. For every GPT timeout, WDT will get refreshed */
        err = R_WDT_Open(&g_wdt_ctrl, &g_wdt_cfg);
        if (FSP_SUCCESS != err)
        {
            printf("\r\n** R_WDT_Open API failed **\r\n");
            return err;
        }
        /* Start GPT timer in Periodic mode */
        gpt_start();
        /* Print message to indicate the user about application status */
        printf("\r\nWDT initialized, GPT Timer started");
        printf("\r\nTo stop WDT counter from refreshing, press the 'MD KEY' button\r\n");
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
                printf("\r\n** R_GPT_Stop API failed **\r\n");
                return err;
            }
            printf("\r\ngpt timer stopped");
            printf("\r\n** Watchdog Timer will underflow and MCU will reset **\r\n");
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
void wdt_test(void)
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
        if (2 == g_timer_underflow_counter)
        {
            g_timer_underflow_counter = RESET_VALUE;
            printf("\r\nWDT counter Refreshed");
        }
    }

}
