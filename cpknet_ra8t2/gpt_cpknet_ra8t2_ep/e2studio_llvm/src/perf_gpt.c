/*
 * perf_gpt.c
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "perf_gpt.h"

/* Boolean flag to determine One-Shot mode timer is expired or not */
bool volatile g_one_shot_expired  = false;
/* Store Timer open state */
uint8_t g_timer_open_state = RESET_VALUE;

/***********************************************************************************************************************
 * @brief       Initialize GPT timer
 * @param[in]   p_timer_ctl     Timer instance control structure
 * @param[in]   p_timer_cfg     Timer instance Configuration structure
 * @param[in]   timer_mode      Mode of GPT Timer
 * @retval      FSP_SUCCESS     Upon successful open of timer
 * @retval      Any Other Error code apart from FSP_SUCCESS on Unsuccessful open
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t init_gpt_timer(timer_ctrl_t * const p_timer_ctl, timer_cfg_t const * const p_timer_cfg, uint8_t timer_mode)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Initialize GPT Timer */
    err = R_GPT_Open(p_timer_ctl, p_timer_cfg);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_GPT_Open API FAILED **\r\n");
        return err;
    }
    if (PERIODIC_MODE_TIMER == timer_mode)
    {
        g_timer_open_state = PERIODIC_MODE;
    }
    else if (PWM_MODE_TIMER == timer_mode)
    {
        g_timer_open_state = PWM_MODE;
    }
    else
    {
        g_timer_open_state = ONE_SHOT_MODE;
    }
    return err;
}

/***********************************************************************************************************************
 * @brief       Start GPT timers in Periodic, One-Shot, PWM mode
 * @param[in]   p_timer_ctl     Timer instance control structure
 * @retval      FSP_SUCCESS     Upon successful start of timer
 * @retval      Any Other Error code apart from FSP_SUCCESS on Unsuccessful start
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t start_gpt_timer (timer_ctrl_t * const p_timer_ctl)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Starts GPT timer */
    err = R_GPT_Start(p_timer_ctl);
    if (FSP_SUCCESS != err)
    {
        /* In case of GPT_open is successful and start fails, requires a immediate cleanup.
         * Since, cleanup for GPT open is done in start_gpt_timer, hence cleanup is not required */
        printf ("\r\n** R_GPT_Start API failed **\r\n");
    }
    return err;
}

/***********************************************************************************************************************
 *  @brief       Set duty cycle of PWM timer
 *  @param[in]   duty_cycle_percent
 *  @retval      FSP_SUCCESS on correct duty cycle set
 *  @retval      FSP_INVALID_ARGUMENT on invalid info
 **********************************************************************************************************************/
CODE_AREA
fsp_err_t set_timer_duty_cycle(uint8_t duty_cycle_percent)
{
    fsp_err_t err                           = FSP_SUCCESS;
    uint32_t duty_cycle_counts              = RESET_VALUE;
    uint32_t current_period_counts          = RESET_VALUE;
    timer_info_t info                       = {(timer_direction_t)RESET_VALUE, RESET_VALUE, RESET_VALUE};

    /* Get the current period setting */
    err = R_GPT_InfoGet(&g_timer_pwm_ctrl, &info);
    if (FSP_SUCCESS != err)
    {
        /* GPT Timer InfoGet Failure message */
        printf ("\r\n** R_GPT_InfoGet API failed **\r\n");
    }
    else
    {
        /* Update period counts locally */
        current_period_counts = info.period_counts;

        /* Calculate the desired duty cycle based on the current period. Note that if the period could be larger than
         * UINT32_MAX / 100, this calculation could overflow. A cast to uint64_t is used to prevent this. The cast is
         * not required for 16-bit timers. */
        duty_cycle_counts = (uint32_t) ((uint64_t) (current_period_counts * duty_cycle_percent) / GPT_MAX_PERCENT);

        /* Duty cycle set API set the desired intensity on the on-board LED */
        err = R_GPT_DutyCycleSet(&g_timer_pwm_ctrl, duty_cycle_counts, TIMER_PIN);
        if (FSP_SUCCESS != err)
        {
            /* GPT Timer duty cycle set failure message */
            /* In case of GPT_open is successful and duty cycle set fails, requires a immediate cleanup.
             * Since, cleanup for GPT open is done in timer_duty_cycle_set, hence cleanup is not required */
            printf ("\r\n** R_GPT_DutyCycleSet API failed **\r\n");
        }
    }
    return err;
}

/***********************************************************************************************************************
 *  @brief      Process input string to integer value
 *  @param[in]  None
 *  @retval     integer value of input string
 **********************************************************************************************************************/
CODE_AREA
uint32_t process_input_data(void)
{
    unsigned char buf[BUF_SIZE] = {INITIAL_VALUE};
    uint32_t num_bytes          = RESET_VALUE;
    uint32_t value              = RESET_VALUE;

    while (RESET_VALUE == num_bytes)
    {
        if (APP_CHECK_DATA)
        {
            //num_bytes = APP_READ(buf);
            num_bytes = CONSOLE_Read(buf, BUF_SIZE);
            if (RESET_VALUE == num_bytes)
            {
                printf("\r\nInvalid Input\r\n");
            }
        }
    }

    /* Conversion from input string to integer value */
    value =  (uint32_t) (atoi((char *)buf));

    return value;
}

/***********************************************************************************************************************
 * @brief      Close the GPT HAL driver
 * @param[in]  p_timer_ctl     Timer instance control structure
 * @retval     None
 **********************************************************************************************************************/
CODE_AREA
void deinit_gpt_timer(timer_ctrl_t * const p_timer_ctl)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Timer Close API call */
    err = R_GPT_Close(p_timer_ctl);
    if (FSP_SUCCESS != err)
    {
        /* GPT Close failure message */
        printf ("\r\n** R_GPT_Close FAILED **\r\n");
    }

    /* Reset open state of timer */
    g_timer_open_state = RESET_VALUE;
}

/***********************************************************************************************************************
 * @brief      Print GPT Timer menu option
 * @param[in]  None
 * @retval     None
 **********************************************************************************************************************/
CODE_AREA
void print_timer_menu(void)
{
    //SEGGER_RTT_Flush(SEGGER_INDEX);
    printf("Menu Options\r\n"
              "1. Enter 1 for Periodic mode\r\n"
              "2. Enter 2 for PWM mode\r\n"
              "3. Enter 3 for One-Shot mode\r\n");

    printf("User Input:  ");
}

/***********************************************************************************************************************
 *  @brief      The user defined GPT callback in One-Shot mode
 *  @param[in]  p_args  updates timer event.
 *  @retval     None
 **********************************************************************************************************************/
CODE_AREA
void user_gpt_one_shot_callback(timer_callback_args_t * p_args)
{
    if (NULL != p_args)
    {
        if (TIMER_EVENT_CYCLE_END  == p_args->event)
        {
            /* Set boolean flag on One-Shot mode timer expired */
            g_one_shot_expired = true;
        }
    }
}

/***********************************************************************************************************************
 * @brief       Wait for any key press from console
 * @param[in]   prompt  Message to display while waiting
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void wait_for_any_key(const char* prompt)
{
    unsigned char dummy;
    printf("\r\n%s", prompt);
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
CODE_AREA
void gpt_basic_test1(void)
{
    fsp_err_t err                           = FSP_SUCCESS;
    uint64_t period_counts                  = RESET_VALUE;
    uint32_t pclkd_freq_hz                  = RESET_VALUE;
    int32_t  one_shot_timeout               = INT32_MAX;
    fsp_pack_version_t version;
    unsigned char dummy;

    /* Fixed test parameters */
    #define FIXED_PERIOD_MS         (1000U)  /* Periodic mode: fixed 1-second interval */
    #define FIXED_DUTY_CYCLE_PERCENT (50U)   /* PWM mode: fixed 50% duty cycle */

    /* Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the Console */
    printf(BANNER_1);
    printf(BANNER_2);
    printf(BANNER_3,EP_VERSION);
    printf(BANNER_4,version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    printf(BANNER_5);

    printf(EP_INFO);
    printf("\r\n--- GPT Timer Test Suite ---\r\n");

    /*****************************************************************
     * Test preparation: wait for key press to start
     *****************************************************************/
    printf("\r\nPress any key to start GPT timer tests...\r\n");
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    /*****************************************************************
     * Test 1: Periodic Mode
     *****************************************************************/
    printf("\r\nPress any key to start Periodic Mode test...\r\n");
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    printf("\r\n[1] Testing Periodic Mode, Fixed Period = %d ms\r\n", FIXED_PERIOD_MS);

    /* Ensure no other timers are running */
    if (PERIODIC_MODE == g_timer_open_state)
    {
        deinit_gpt_timer(&g_timer_periodic_ctrl);
    }
    if (PWM_MODE == g_timer_open_state)
    {
        deinit_gpt_timer(&g_timer_pwm_ctrl);
    }
    if (ONE_SHOT_MODE == g_timer_open_state)
    {
        deinit_gpt_timer(&g_timer_one_shot_mode_ctrl);
    }

    /* Calculate period counts */
    pclkd_freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
    pclkd_freq_hz >>= (uint32_t)(g_timer_periodic_cfg.source_div);
    period_counts = (uint64_t)((FIXED_PERIOD_MS * (pclkd_freq_hz * CLOCK_TYPE_SPECIFIER)) /
                               TIMER_UNITS_MILLISECONDS);

    if (GPT_MAX_PERIOD_COUNT < period_counts)
    {
        printf("\r\n** ERROR: Fixed period %d ms is out of range **\r\n", FIXED_PERIOD_MS);
    }
    else
    {
        /* Initialize and start periodic timer */
        err = init_gpt_timer(&g_timer_periodic_ctrl, &g_timer_periodic_cfg, PERIODIC_MODE_TIMER);
        if (FSP_SUCCESS != err)
        {
            printf("** init_gpt_timer function failed (Periodic Mode) **\r\n");
            printf("\r\nReturned Error Code: 0x%x\r\n", (err));
        }
        else
        {
            printf("  Periodic mode timer opened\r\n");

            err = start_gpt_timer(&g_timer_periodic_ctrl);
            if (FSP_SUCCESS != err)
            {
                printf("** start_gpt_timer function failed (Periodic Mode) **\r\n");
                /* Close Periodic Timer instance */
                deinit_gpt_timer(&g_timer_periodic_ctrl);
                printf("\r\nReturned Error Code: 0x%x\r\n", (err));
            }
            else
            {
                printf("  Periodic mode timer started\r\n");

                /* Set period */
                err = R_GPT_PeriodSet(&g_timer_periodic_ctrl, (uint32_t)period_counts);
                if (FSP_SUCCESS != err)
                {
                    /* GPT Timer PeriodSet Failure message */
                    printf ("\r\n** R_GPT_PeriodSet API failed (Periodic Mode) **\r\n");
                    /* Close Periodic Timer instance */
                    deinit_gpt_timer(&g_timer_periodic_ctrl);
                    printf("\r\nReturned Error Code: 0x%x\r\n", (err));
                }
                else
                {
                    printf("  Period set to %d ms\r\n", FIXED_PERIOD_MS);

                    /* Keep running for a while (e.g., wait for 3 cycles) to observe effect */
                    printf("  Running for 3 periods...\r\n");
                    for(int i = 0; i < 10; i++) {
                        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
                    }
                    printf("  Periodic mode test completed.\r\n");
                    deinit_gpt_timer(&g_timer_periodic_ctrl);
                }
            }
        }
    }

    /*****************************************************************
     * Test 2: PWM Mode
     *****************************************************************/
    printf("\r\nPress any key to start PWM Mode test...\r\n");
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    printf("\r\n[2] Testing PWM Mode, Fixed Duty Cycle = %d%%\r\n", FIXED_DUTY_CYCLE_PERCENT);

    /* Ensure PWM timer is not running */
    if (PWM_MODE == g_timer_open_state)
    {
        deinit_gpt_timer(&g_timer_pwm_ctrl);
    }

    /* Check duty cycle parameter validity */
    if (GPT_MAX_PERCENT < FIXED_DUTY_CYCLE_PERCENT)
    {
        printf("\r\n** ERROR: Fixed duty cycle %d%% is out of range **\r\n", FIXED_DUTY_CYCLE_PERCENT);
    }
    else
    {
        /* Initialize and start PWM timer */
        err = init_gpt_timer(&g_timer_pwm_ctrl, &g_timer_pwm_cfg, PWM_MODE_TIMER);
        if (FSP_SUCCESS != err)
        {
            printf("** init_gpt_timer function failed (PWM Mode) **\r\n");
            printf("\r\nReturned Error Code: 0x%x\r\n", (err));
        }
        else
        {
            printf("  PWM mode timer opened\r\n");

            err = start_gpt_timer(&g_timer_pwm_ctrl);
            if (FSP_SUCCESS != err)
            {
                printf("** start_gpt_timer function failed (PWM Mode) **\r\n");
                /* Close PWM Timer instance */
                deinit_gpt_timer(&g_timer_pwm_ctrl);
                printf("\r\nReturned Error Code: 0x%x\r\n", (err));
            }
            else
            {
                printf("  PWM mode timer started\r\n");

                /* Set duty cycle */
                err = set_timer_duty_cycle((uint8_t)FIXED_DUTY_CYCLE_PERCENT);
                if (FSP_SUCCESS != err)
                {
                    /* GPT Timer duty cycle set failure message */
                    printf ("\r\n** set_timer_duty_cycle function failed (PWM Mode) **\r\n");
                    /* Close PWM Timer instance */
                    deinit_gpt_timer(&g_timer_pwm_ctrl);
                    printf("\r\nReturned Error Code: 0x%x\r\n", (err));
                }
                else
                {
                    printf("  Duty cycle set to %d%%\r\n", FIXED_DUTY_CYCLE_PERCENT);

                    /* Keep running for a while to observe effect */
                    printf("  Running PWM for observation...\r\n");
                    for(int i = 0; i < 10; i++) {
                                            R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
                                        }
                    printf("  PWM mode test completed.\r\n");
                    deinit_gpt_timer(&g_timer_pwm_ctrl);
                }
            }
        }
    }

    /*****************************************************************
     * Test 3: One-Shot Mode
     *****************************************************************/
    printf("\r\nPress any key to start One-Shot Mode test...\r\n");
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    printf("\r\n[3] Testing One-Shot Mode\r\n");

    /* Ensure one-shot timer is not running */
    if (ONE_SHOT_MODE == g_timer_open_state)
    {
        deinit_gpt_timer(&g_timer_one_shot_mode_ctrl);
    }

    /* Initialize and start one-shot timer */
    err = init_gpt_timer(&g_timer_one_shot_mode_ctrl, &g_timer_one_shot_mode_cfg, ONE_SHOT_MODE_TIMER);
    if (FSP_SUCCESS != err)
    {
        printf("** init_gpt_timer function failed (One-Shot Mode) **\r\n");
        printf("\r\nReturned Error Code: 0x%x\r\n", (err));
    }
    else
    {
        printf("  One-shot mode timer opened\r\n");

        err = start_gpt_timer(&g_timer_one_shot_mode_ctrl);
        if (FSP_SUCCESS != err)
        {
            printf("** start_gpt_timer function failed (One-Shot Mode) **\r\n");
            /* Close One-Shot Timer instance */
            deinit_gpt_timer(&g_timer_one_shot_mode_ctrl);
            printf("\r\nReturned Error Code: 0x%x\r\n", (err));
        }
        else
        {
            printf("  One-shot mode timer started, waiting for timer to expire...\r\n");

            /* Wait for one-shot timer to expire (via callback setting flag) */
            g_one_shot_expired = false;
            one_shot_timeout = 0x7FFFFFFF; // Reset timeout counter
            while (true != g_one_shot_expired)
            {
                --one_shot_timeout;
                if (RESET_VALUE == one_shot_timeout)
                {
                    printf("Callback event not received during One-Shot operation\r\n");
                    break;
                }
            }

            if (true == g_one_shot_expired)
            {
                printf("  One-shot mode timer expired.\r\n");
            }
            deinit_gpt_timer(&g_timer_one_shot_mode_ctrl);
        }
    }

    printf("\r\nPress any key to exit GPT timer tests...\r\n");
    while (1) {
        if (CONSOLE_HasData()) {
            CONSOLE_Read(&dummy, 1);
            break;
        }
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
    printf("\r\n--- All mode automated tests completed ---\r\n");
}
