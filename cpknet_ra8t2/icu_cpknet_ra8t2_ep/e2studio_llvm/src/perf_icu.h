/*
 * perf_icu.h
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */

#ifndef PERF_ICU_H_
#define PERF_ICU_H_

#include "hal_data.h"

#define EP_INFO    "\r\nThis example project demonstrates the functionality of ICU driver.\r\n"\
                   "On pressing the user push button, an external IRQ is triggered, \r\n"\
                   "which trigger on-board LED blink once time.\r\n"\
                    "And print:**Enter external interrupt**\r\n"

void irq0_callback (external_irq_callback_args_t * p_args);

void icu_test (void);

#endif /* PERF_ICU_H_ */
