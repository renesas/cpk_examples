/*
 * perf_iwdt.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_WDT_H_
#define PERF_WDT_H_

#include "hal_data.h"
/* WDT detect reset value */
#define SYSTEM_RSTSR1_WDTRF_DETECT_WDT_RESET        (1u)

#define BUFFER_SIZE                                 (16u)

/* Number of counts for printing WDT refresh status */
#define WDT_REFRESH_COUNTER_VALUE                   (3u)

/* The user command input value */
#define ENABLE_WDT                                  (1u)
/* Function declarations */

void deinit_gpt_module(void);
void wdt_test(void);
#endif /* PERF_WDT_H_ */
