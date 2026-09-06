/*
 * uart_perf.h
 *
 *  Created on: Jan 26, 2026
 *      Author: Ran QingLing
 */

#ifndef PERF_UART_H_
#define PERF_UART_H_

#define UART_TYPE      "SCI_UART"

void r_sci_b_uart_baud_set (uint32_t baudrate_value);
void uart_initialize(void);
void uart0_callback (uart_callback_args_t * p_args);
void uart_read_write(uart_ctrl_t * const p_api_ctrl);
void uart_perf_test(void);
void r_sci_b_uart_baud_set9 (uint32_t baudrate_value);
void bsp_peripheral_clock_set1 (volatile uint8_t * p_clk_ctrl_reg,
                                      volatile uint8_t * p_clk_div_reg,
                                      uint8_t            peripheral_clk_div,
                                      uint8_t            peripheral_clk_source);
#endif /* PERF_UART_H_ */
