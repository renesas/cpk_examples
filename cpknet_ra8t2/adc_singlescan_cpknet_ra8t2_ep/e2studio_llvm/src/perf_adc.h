/*
 * perf_adc.h
 *
 *  Created on: Jan 15, 2026
 *      Author: Ran QingLing
 */

#ifndef PERF_ADC_H_
#define PERF_ADC_H_

#include "hal_data.h"

void adc_callback (adc_callback_args_t * p_args);
fsp_err_t adc_init(void);
fsp_err_t adc_scan_stop(void);
void adc_ContinueScan_test(void);
fsp_err_t adc_read_data(void);
void adc_SingleScan_test(void);
#endif /* PERF_ADC_H_ */
