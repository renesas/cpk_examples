/*
 * perf_dac.h
 *
 *  Created on: Jan 28, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_DAC_H_
#define PERF_DAC_H_
fsp_err_t init_dac_driver(void);
fsp_err_t adc_read_data(void);
fsp_err_t adc_scan_stop(void);
#endif /* PERF_DAC_H_ */
