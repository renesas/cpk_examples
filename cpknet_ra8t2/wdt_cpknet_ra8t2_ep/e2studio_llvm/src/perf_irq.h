/*
 * perf_irq.h
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */

#ifndef PERF_IRQ_H_
#define PERF_IRQ_H_

#include "hal_data.h"

void irq0_callback (external_irq_callback_args_t * p_args);
void irq1_callback (external_irq_callback_args_t * p_args);
void irq_initialize (void);

#endif /* PERF_IRQ_H_ */
