/*
 * perf_icu.h
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */

#ifndef PERF_ACMPHS_H_
#define PERF_ACMPHS_H_

#include "hal_data.h"

#define EP_INFO "\r\nIn this project DAC0 is used as reference voltage source and DAC1 is used as input"\
                "\r\nvoltage source for ACMPHS module. DAC0 value is set to 2048 (i.e., 1.65V)."\
                "\r\nThe user can enter DAC1 value within permitted range. When DAC1 input value is greater"\
                "\r\nthan set DAC0 reference voltage, the comparator output status is HIGH and on-board LED"\
                "\r\nis turned ON. When DAC1 input is less than reference voltage, output status is LOW"\
                "\r\nand the LED is turned OFF.\r\n"
void acmphs_test (void);

#endif /* PERF_ACMPHS_H_ */
