/*
 * perf_iwdt.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_IWDT_H_
#define PERF_IWDT_H_

#include "hal_data.h"
/* IWDT/NMI detect reset value */
#define SYSTEM_RSTSR1_IWDTRF_DETECT_RESET       (1u)
#define SYSTEM_RSTSR1_SWRF_DETECT_RESET         (1u)

/* Number of counts for printing IWDT Refresh status */
#define IWDT_REFRESH_COUNTER_VALUE              (2u)

#define BUFFER_SIZE                             (16u)
#define USED_TIMER                              "GPT"

#define IWDT_UNDERFLOW                          "17"
/* The user command input value */
#define ENABLE_IWDT                             (1u)
#define STOP_IWDT_REFRESH                       (2u)

/* Function declarations */
void irq0_callback (external_irq_callback_args_t * p_args);
void deinit_gpt_module(void);
void iwdt_test(void);
#endif /* PERF_IWDT_H_ */
