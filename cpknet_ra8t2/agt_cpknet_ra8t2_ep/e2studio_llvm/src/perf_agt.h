/*
 * perf_agt.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_AGT_H_
#define PERF_AGT_H_

#include "hal_data.h"
#define ARRAY_INDEX                 (0U)

/* Macros to flag the status */
#define SET_FLAG                    (0x01)
#define RESET_FLAG                  (0x00)

/* Status of timer */
#define ALREADY_RUNNING             (0x02)

/* Macro to check if the LED count is zero */
#define LED_COUNT_ZERO              (0U)

/* Macros to define time-period value limits */
#define TIME_PERIOD_MAX             (2000U)
#define TIME_PERIOD_MIN             (0U)

/* Macro for null character */
#define NULL_CHAR                   ('\0')

/***********************************************************************************************************************
 * User-defined APIs
 **********************************************************************************************************************/

/* Function declarations */
fsp_err_t agt_init(void);
fsp_err_t agt_start_oneshot_timer(void);
void agt_deinit(void);
void apt_test(void);
#endif /* PERF_AGT_H_ */
