/*
 * perf_dma_block.h
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#ifndef PERF_DMA_BLOCK_H_
#define PERF_DMA_BLOCK_H_

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>


#define BLOCK_SIZE      (4)
#define NUM_BLOCKS      (3)
#define TOTAL_LENGTH    (BLOCK_SIZE * NUM_BLOCKS)

extern volatile bool g_dma_trans_complete;
extern volatile bool g_dma_trans_error;
extern volatile uint8_t g_current_block;


extern uint8_t g_src_buf[TOTAL_LENGTH];
extern uint8_t g_dst_buf[BLOCK_SIZE];


void dma_callback(dmac_callback_args_t * cb_data);
fsp_err_t dma_init(void);
bool dma_data_verify(void);
void dma_test(void);
void dma_stop(void);

#endif /* PERF_DMA_BLOCK_H_ */
