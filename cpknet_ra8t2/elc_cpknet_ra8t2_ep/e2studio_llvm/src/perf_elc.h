/*
 * perf_elc.h
 *
 *  Created on: Feb 24, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_ELC_H_
#define PERF_ELC_H_

#include "hal_data.h"

#define EP_INFO                 "\r\nThis example project demonstrates the basic usage of ELC driver."\
		                        "\r\nELC Software Event, GPT1 and GPT0 events are linked using ELC."\
                                "\r\nThe start source for GPT1 and GPT0 is ELC Software Event, and the"\
                                "\r\nstop source for GPT1 is GPT0 counter overflow."\
                                "\r\nGPT1 runs in PWM mode, and GPT0 runs in One-Shot mode."\
                                "\r\nOn giving valid input, an ELC Software Event is generated that"\
                                "\r\ntriggers LED blinking. LED stops blinking after 5 seconds when GPT0 expires.\r\n"

#define OPEN_TIMER_FAIL         "\r\n** GPT module Open for One-Shot mode failed **\r\n"
#define OPEN_TIMER_PWM_FAIL     "\r\n** GPT module Open for PWM mode failed **\r\n"
#define ENABLE_TIMER_PULSE_FAIL "\r\n** GPT Enable for PWM timer failed **\r\n"
#define STATUS_DISPLAY          "\r\nInput any character. LED blinks for 5 seconds and stop."
#define ENABLE_TIMER_FAIL       "\r\n** GPT Enable for One-Shot mode failed **\r\n"

/* Macro for NULL character */
#define NULL_CHAR ('\0')

/* Function declaration */
void elc_test (void);


#endif /* PERF_ELC_H_ */
