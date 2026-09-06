/*
 * perf_dma_normal.h
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#ifndef PERF_DMA_NORMAL_H_
#define PERF_DMA_NORMAL_H_
#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

// Global status flags (external declaration)
extern volatile bool g_dma_trans_complete;
extern volatile bool g_dma_trans_error;

void dma_callback(dmac_callback_args_t * cb_data);
fsp_err_t dma_init(void);
bool dma_data_verify(void);
void dma_test(void);
void dma_stop(void);
#endif /* PERF_DMA_NORMAL_H_ */
