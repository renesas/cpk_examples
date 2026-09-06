/*
 * perf_ulpt.c
 *
 *  Created on: April 27, 2026
 *      Author: Ran Qingling
 */
#include <perf_ulpt.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

extern bsp_leds_t g_bsp_leds;
extern volatile uint8_t g_periodic_timer_flag ;
extern volatile uint32_t g_error_flag ;
volatile uint8_t g_one_shot_timer_flag = RESET_FLAG;    /* Flag to check timer is enabled or not */
volatile uint8_t g_periodic_timer_flag = RESET_FLAG;    /* Flag to check timer1 is enabled or not */
volatile uint32_t g_error_flag = RESET_FLAG;            /* Flag to capture error in ISR's */
volatile uint32_t g_status_check_flag = RESET_FLAG;     /* Flag to capture the status of timers */
/*******************************************************************************************************************//**
 * @brief       This function initializes AGT module.
 * @param[IN]   None
 * @retval      FSP_SUCCESS     Upon successful open of AGT module
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful open
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t ulpt_init(void)
{
    fsp_err_t err = FSP_SUCCESS;     /* Error status */

    /* Open Timer0 in One-Shot mode */
    err = R_ULPT_Open(&g_timer0_one_shot_ctrl, &g_timer0_one_shot_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        printf ("\r\nULPT0 timer open failed\r\nRestart the Application\r\n");
        return err;
    }

    /* Open Timer1 in Periodic mode */
    err = R_ULPT_Open(&g_timer1_periodic_ctrl, &g_timer1_periodic_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* Close Timer0 in One-Shot mode */
        if ( (FSP_SUCCESS != R_ULPT_Close(&g_timer0_one_shot_ctrl)))
        {
            printf ("\r\nOne-Shot timer close failed\r\nRestart the Application\r\n");
        }
        printf ("\r\nAGT1 timer open failed\r\nRestart the Application\r\n");
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief       This function starts AGT0 in One-Shot mode.
 * @param[IN]   None
 * @retval      FSP_SUCCESS     Timer started successfully
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t ulpt_start_oneshot_timer(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Start Timer0 in One-Shot mode */
    err = R_ULPT_Start(&g_timer0_one_shot_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        printf("\r\nULPT0 timer start failed\r\n");
    }
    return err;
}

/*******************************************************************************************************************//**
 **********************************************************************************************************************/
CODE_AREA
void one_shot_timer_callback(timer_callback_args_t *p_args)
{
    if(TIMER_EVENT_CYCLE_END == p_args->event)
    {
        fsp_err_t err = FSP_SUCCESS;
        timer_status_t periodic_timer_status;

        /* Retrieve the status of timer running in Periodic mode */
        err = R_ULPT_StatusGet(&g_timer1_periodic_ctrl, &periodic_timer_status);
        if (FSP_SUCCESS != err)
        {
            g_error_flag = SET_FLAG;
        }
        else
        {
            if (TIMER_STATE_COUNTING != periodic_timer_status.state)
            {
                /* Start the timer in Periodic mode only if the timer is in stopped state */
                err = R_ULPT_Start(&g_timer1_periodic_ctrl);
                /* Handle error */
                if (FSP_SUCCESS != err)
                {
                    g_error_flag = SET_FLAG;
                    printf("\r\nAGT1 cannot be started in Periodic mode");
                }
                else
                {
                    g_periodic_timer_flag = SET_FLAG;   /* Set the flag since timer1 is started in Periodic mode */
                }
            }
            else
            {
                g_periodic_timer_flag = ALREADY_RUNNING;
            }
        }
    }
}

/*******************************************************************************************************************//**
 * @brief       This function is callback for periodic timer and blinks LED on every 1 Second.
 * @param[in]   p_args
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void periodic_timer_callback(timer_callback_args_t *p_args)
{

    static volatile bsp_io_level_t led_level = BSP_IO_LEVEL_HIGH;
    /* If this board has no LEDs then trap here */
    if (LED_COUNT_ZERO == g_bsp_leds.led_count)
    {
        g_error_flag = SET_FLAG;
        printf ("\r\nError: No LED was found on the board");
        return;
    }

    /* Change the state of the LED write value */
    led_level ^= BSP_IO_LEVEL_HIGH;

    if(TIMER_EVENT_CYCLE_END == p_args->event)
    {
        /* Change LED state */
        fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl, (bsp_io_port_pin_t) g_bsp_leds.p_leds[RESET_FLAG], led_level);
        /* Handle error */
        if (FSP_SUCCESS != err)
        {
            g_error_flag = SET_FLAG;
            printf("\r\nLED pin state cannot be toggled");
            return;
        }
    }
}

/*******************************************************************************************************************//**
 * @brief       This function closes opened AGT module before the project ends up in an error trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void ulpt_deinit(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close ulpt0 module */
    err = R_ULPT_Close(&g_timer0_one_shot_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* ulpt0 Close failure message */
        printf("** R_ULPT_Close API for channel 0 failed **\r\n");
    }

    /* Close AGT1 module */
    err = R_ULPT_Close(&g_timer1_periodic_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* AGT1 Close failure message */
        printf("** R_ULPT_Close API for channel 1 failed **\r\n");
    }
}
CODE_AREA
static fsp_err_t timer_status_check (void)
{
    fsp_err_t err = FSP_SUCCESS;
    timer_status_t periodic_timer_status =
    {
     .counter = RESET_VALUE,
     .state = (timer_state_t) RESET_VALUE,
    };
    timer_status_t oneshot_timer_status =
    {
     .counter = RESET_VALUE,
     .state = (timer_state_t) RESET_VALUE,
    };

    /* Retrieve the status of timer running in Periodic mode */
    err = R_ULPT_StatusGet(&g_timer1_periodic_ctrl, &periodic_timer_status);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        printf("\r\nR_ULPT_StatusGet API failed");
        return err;
    }

    /* Retrieve the status of timer running in One-Shot mode */
    err = R_ULPT_StatusGet(&g_timer0_one_shot_ctrl, &oneshot_timer_status);
    if (FSP_SUCCESS != err)
    {
        printf("\r\nR_ULPT_StatusGet API failed");
        return err;
    }

    if (TIMER_STATE_STOPPED != oneshot_timer_status.state)
    {
        err = R_ULPT_Stop(&g_timer0_one_shot_ctrl);
        if (FSP_SUCCESS != err)
        {
            printf("\r\nR_ULPT_Stop API failed");
            return err;
        }
        else
        {
            printf("\r\nOne-Shot timer stopped. Enter any key to start timers.\r\n");
        }
    }
    else if (TIMER_STATE_STOPPED != periodic_timer_status.state)
    {
        err = R_ULPT_Stop(&g_timer1_periodic_ctrl);
        if (FSP_SUCCESS != err)
        {
            printf("\r\nR_ULPT_Stop API failed");
            return err;
        }
        else
        {
            printf("\r\nPeriodic timer stopped. Enter any key to start timers.\r\n");
        }
    }
    else
    {
        err = ulpt_start_oneshot_timer();
        /* Handle error */
        if (FSP_SUCCESS != err)
        {
            printf("\r\n ulpt start failed");
            return err;
        }
        else
        {
            g_one_shot_timer_flag = SET_FLAG;        /* Set Timer Flag as timer is started */
        }
    }
    return err;
}
CODE_AREA
void ulpt_test(void)
{
    fsp_err_t err = FSP_SUCCESS;
    fsp_pack_version_t version = {RESET_VALUE};
    unsigned char rByte[BUFFER_SIZE_DOWN] = {RESET_VALUE};
    uint32_t time_period_ms_oneshot = RESET_VALUE;
    uint32_t time_period_ms_periodic = RESET_VALUE;
    uint32_t raw_counts_oneshot = RESET_VALUE;
    uint32_t raw_counts_periodic = RESET_VALUE;
    timer_info_t  one_shot_info =
    {
     .clock_frequency = RESET_VALUE,
     .count_direction = (timer_direction_t) RESET_VALUE,
     .period_counts = RESET_VALUE,
    };
    timer_info_t periodic_info =
    {
     .clock_frequency = RESET_VALUE,
     .count_direction = (timer_direction_t) RESET_VALUE,
     .period_counts = RESET_VALUE,
    };
    uint32_t clock_freq = RESET_VALUE;

    /* Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);
    /* Example Project information printed on the Console */
    printf("\r\nThis example project demonstrates the functionality of ULPT in Periodic mode and One-Shot mode."\
              "\r\nOn providing any input on the RTT Viewer, ULPT channel 0 starts in One-Shot mode. ULPT channel 1"\
              "\r\nstarts in Periodic mode when ULPT channel 0 expires. Timer in Periodic mode expires periodically"\
              "\r\nat a time period specified by the user and toggles the on-board LED.\r\n");
    /* Initialize ulpt driver */
    err = ulpt_init();
    if (FSP_SUCCESS != err)
    {   /* AGT module initialize failed */
        printf("\r\n ** AGT INIT FAILED ** \r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    //printf("\r\nPlease enter time period values for One-Shot and Periodic mode timers in milliseconds\n"
    //          "Valid range: 1 to 2000\r\n");
    printf("\r\nOne-Shot mode period set: 1000");
    time_period_ms_oneshot = 1000;


    printf("\r\nPeriodic mode period set: 200:");
    time_period_ms_periodic = 200;

    /* Calculation of raw counts value for given milliseconds value */
    err = R_ULPT_InfoGet(&g_timer0_one_shot_ctrl, &one_shot_info);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        ulpt_deinit();
        printf("\r\nR_AGT_InfoGet API failed. Closing all drivers. Restart the Application\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    /* Depending on the user selected clock source, raw counts value can be calculated
     * for the user given time-period values */
    clock_freq = one_shot_info.clock_frequency;
    raw_counts_oneshot = (uint32_t)((time_period_ms_oneshot * clock_freq ) / 1000);
    /* Set period value */
    err = R_ULPT_PeriodSet(&g_timer0_one_shot_ctrl, raw_counts_oneshot);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        ulpt_deinit();
        printf("\r\nR_ULPT_PeriodSet API failed. Closing all drivers. Restart the Application\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    /* Calculation of raw counts value for given milliseconds value */
    err = R_ULPT_InfoGet(&g_timer1_periodic_ctrl, &periodic_info);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        ulpt_deinit();
        printf("\r\nR_AGT_InfoGet API failed. Closing all drivers. Restart the Application\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    /* Depending on the user selected clock source, raw counts value can be calculated
     * for the user given time-period values */
    clock_freq = periodic_info.clock_frequency;
    raw_counts_periodic = (uint32_t)((time_period_ms_periodic * clock_freq ) / 1000);
    /* Set period value */
    err = R_ULPT_PeriodSet(&g_timer1_periodic_ctrl, raw_counts_periodic);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        ulpt_deinit();
        printf("\r\nR_AGT_PeriodSet API failed. Closing all drivers. Restart the Application\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }

    printf("\r\nEnter any key to start or stop the timers");

    while(true)
    {

        if (CONSOLE_HasData())
        {
            /* Cleaning buffer */
            //memset (&rByte[0], NULL_CHAR, BUFFER_SIZE_DOWN);
            //APP_READ(rByte);
            CONSOLE_Init();
            g_status_check_flag = SET_FLAG;
        }

        if (SET_FLAG == g_status_check_flag)
        {
            /* Check the status of timers and perform operation accordingly */
            g_status_check_flag = RESET_FLAG;
            err = timer_status_check();
            /* Handle error */
            if (FSP_SUCCESS != err)
            {
                ulpt_deinit();
                printf("\r\nTimer start/stop failed");
                printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
            }
        }

        /* Check if ULPT0 is enabled in One-Shot mode */
        if (SET_FLAG == g_one_shot_timer_flag)
        {
            g_one_shot_timer_flag = RESET_FLAG;
            printf("\r\nULPT0 is Enabled in One-Shot mode");
        }

        /* Check if ULPT1 is enabled in Periodic mode */
        if (SET_FLAG == g_periodic_timer_flag)
        {
            g_periodic_timer_flag = RESET_FLAG;
            printf ("\r\n\r\nOne-Shot mode ULPT timer elapsed");
            printf ("\r\n\r\nULPT1 is Enabled in Periodic mode");
            printf ("\r\nLED will toggle for set time period");
            printf ("\r\nEnter any key to stop timers\r\n");
        }

        /* Check if ULPT1 is already running in Periodic mode */
        if (ALREADY_RUNNING == g_periodic_timer_flag)
        {
            g_periodic_timer_flag = RESET_FLAG;
            printf ("\r\n\r\nOne-Shot mode ULPT timer elapsed\n");
            printf ("\r\n\r\nULPT1 is already running in Periodic mode");
            printf ("\r\nEnter any key to stop the timer\r\n");

        }
    }

}
