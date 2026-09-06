/*
 * perf_rtc.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_RTC_H_
#define PERF_RTC_H_

#include "hal_data.h"
#define SET_TIME            (1U)
#define SET_ALARM           (2U)
#define SET_PERIODIC_IRQ    (3U)
#define GET_CURRENT_TIME    (4U)

/* Macros to flag the status */
#define SET_FLAG                (0x01)
#define RESET_FLAG              (0x00)

/* Macro for ASCII value of zero */
#define ASCII_ZERO              (48)
/* Macro for null character */
#define NULL_CHAR               ('\0')

/* Macro for checking if no byte is received */
#define BYTES_RECEIVED_ZERO     (0U)

/* Macro for delay to be added */
#define LED_DELAY               (2U)

/* Macro for RTC version */
#define  RTC_TYPE               "RTC"

/* Macros to adjust month and year values */
#define MON_ADJUST_VALUE                (1)
#define YEAR_ADJUST_VALUE               (1900)

/* Macro to enable 7 bit used to set days of Alarm Day-of-Week Register to 0111 1111 */
#define ALARMWW                         (127)

/* Macros for RTT input processing */
#define PLACE_VALUE_TEN                 (10)
#define PLACE_VALUE_HUNDRED             (100)
#define PLACE_VALUE_THOUSAND            (1000)
#define BUFFER_SIZE_DOWN      (20U)
/***********************************************************************************************************************
 * User-defined APIs
 **********************************************************************************************************************/

void rtc_simple_test(void);
#endif /* PERF_RTC_H_ */
