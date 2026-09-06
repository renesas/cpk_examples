/*
 * perf_agt.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_CRC_H_
#define PERF_CRC_H_

#include "hal_data.h"

#define LED_ON                  (BSP_IO_LEVEL_HIGH)
#define LED_OFF                 (BSP_IO_LEVEL_LOW)

/* Length of input buffer to calculate CRC in normal mode */
#define NUM_BYTES               (4U)

/* Size of input buffer */
#define BUF_LEN                 (8U)

/* LED toggle delay */
#define TOGGLE_DELAY            (0x15E)

/* 8 and 16 bit seed value and data length */
#define SEED_VALUE              (0x00000000)
#define EIGHT_BIT_DATA_LEN      (5U)
#define SIXTEEN_BIT_DATA_LEN    (6U)

#define EP_INFO "\r\nThe example project demonstrates the typical use of the CRC HAL module APIs.\r\n"\
                "It demonstrates CRC operation for data transmission in normal mode and reception in snoop\r\n"\
                "mode through SCI interface, reception is performed in normal mode through SCI interface.\r\n"\
                "\r\nOnce the transfer is complete, if CRC value for snoop mode is zero and the transmit\r\n"\
                "and receive buffers are equal, the on-board LED blinks as a sign of successful CRC operation.\r\n"\
                "On data mismatch, LED stays ON. Failure and status messages are displayed on RTT Viewer/PuTTY.\r\n"\

/* Check IO-port API return and trap error (if any error occurs) cleans up and display failure details on RTT Viewer */
#define VALIDATE_IO_PORT_API(API)   ({\
                                    if (FSP_SUCCESS != (API))\
                                    {   APP_PRINT("%s API failed at Line number %d", \
                                                    #API, __LINE__);\
                                        cleanup();\
                                        APP_ERR_TRAP(true);\
                                    }\
                                    })

/* Function declarations */
void toggle_led(void);
void cleanup(void);
void deinit_crc(void);
void deinit_uart(void);
void crc_test(void);
#endif /* PERF_CRC_H_ */
