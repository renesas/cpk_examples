/*
 * perf_icu.h
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */

#ifndef PERF_ACMPHS_H_
#define PERF_ACMPHS_H_

#include "hal_data.h"

#define EP_INFO    "\r\nThis example project demonstrates basic functionalities of ACMPHS driver."\
                   "\r\nRefrence voltage select Vref=0.8V,Analog input voltage select DA1."\
                   "\r\nStatus of the test is displayed on J-Link RTT Viewer/PuTTY.\r\n"


void acmphs_test (void);

#endif /* PERF_ACMPHS_H_ */
