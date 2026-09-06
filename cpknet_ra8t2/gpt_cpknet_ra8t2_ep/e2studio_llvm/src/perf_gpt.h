/*
 * perf_gpt.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_GPT_H_
#define PERF_GPT_H_

/* Macros definitions */
#define GPT_MAX_PERCENT          (100U)             /* Max duty cycle percentage */
#define BUF_SIZE                 (16U)              /* Size of buffer for RTT input data */
#define PERIODIC_MODE_TIMER      (1U)               /* To perform GPT Timer in Periodic mode */
#define PWM_MODE_TIMER           (2U)               /* To perform GPT Timer in PWM mode */
#define ONE_SHOT_MODE_TIMER      (3U)               /* To perform GPT Timer in One-Shot mode */
#define INITIAL_VALUE            ('\0')
#define TIMER_UNITS_MILLISECONDS (1000U)            /* Timer unit in millisecond */
#define CLOCK_TYPE_SPECIFIER     (1ULL)             /* Type specifier */

#define RESET_VALUE             (0x00)

/* GPT Timer Pin for boards */
#define TIMER_PIN                (GPT_IO_PIN_GTIOCA)
#define GPT_MAX_PERIOD_COUNT     (0XFFFFFFFF)    /* Max Period Count for 32-bit Timer */
/* GPT timer period range */
#define PERIOD_RANGE            "(0 to 17179)"
#define PERIODIC_MODE            (1U)            /* To check status of GPT Timer in Periodic mode */
#define PWM_MODE                 (2U)            /* To check status of GPT Timer in PWM mode */
#define ONE_SHOT_MODE            (3U)            /* To check status of GPT Timer in One-Shot mode */

#define EP_INFO    "\r\nThis example project demonstrates the basic usage of GPT driver."\
                    "\r\nThe project initializes GPT module in Periodic, PWM or One-Shot mode."\
                   "\r\nPress any key to start the next mode test\r\n"

/* Function declarations */
fsp_err_t init_gpt_timer(timer_ctrl_t * const p_timer_ctl, timer_cfg_t const * const p_timer_cfg, uint8_t timer_mode);
fsp_err_t start_gpt_timer(timer_ctrl_t * const p_timer_ctl);
fsp_err_t set_timer_duty_cycle(uint8_t duty_cycle_percent);
uint32_t  process_input_data(void);
void deinit_gpt_timer(timer_ctrl_t * const p_timer_ctl);
void print_timer_menu(void);
void gpt_basic_test(void);
void gpt_basic_test1(void);
#endif /* PERF_GPT_H_ */
