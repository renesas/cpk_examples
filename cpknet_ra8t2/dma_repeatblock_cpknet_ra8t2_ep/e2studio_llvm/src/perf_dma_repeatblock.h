/*
 * perf_dma_repeat.h
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#ifndef PERF_DMA_REPEATBLOCK_H_
#define PERF_DMA_REPEATBLOCK_H_
#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

#define BLOCK_SIZE          (4)     // 每个块的大小：4字节
#define NUM_BLOCKS          (2)     // 每次重复的块数：2个块
#define TOTAL_REPEAT_TIMES  (2)     // 重复次数：2次
#define TOTAL_TRANSFERS     (BLOCK_SIZE * NUM_BLOCKS) // 总传输字节数


#define SRC_BUFFER_SIZE     (8)     // 源缓冲区大小：8字节（容纳2个块）
#define DST_BUFFER_SIZE     (8)     // 目标缓冲区大小：8字节（容纳2个块）


extern volatile bool g_dma_trans_complete;
extern volatile uint32_t g_dma_transfer_count;

// 缓冲区声明
extern uint8_t g_src_buf[SRC_BUFFER_SIZE];
extern uint8_t g_dst_buf[DST_BUFFER_SIZE];

// 函数声明
void dma_callback(dmac_callback_args_t * cb_data);
fsp_err_t dma_init(void);
bool dma_data_verify(void);
void dma_test(void);
void dma_stop(void);
void print_buffers(void);
#endif /* PERF_DMA_REPEATBLOCK_H_ */
