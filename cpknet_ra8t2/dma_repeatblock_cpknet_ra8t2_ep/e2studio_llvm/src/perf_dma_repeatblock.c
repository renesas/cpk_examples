/*
 * perf_dma_repeat.c
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#include <perf_dma_repeatblock.h>
#include "hal_data.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
/* 全局变量 */
volatile bool g_dma_trans_complete = false;
volatile uint32_t g_dma_transfer_count = 0;

/* 源和目标缓冲区 */
uint8_t g_src_buf[SRC_BUFFER_SIZE] __attribute__((section(".ram_nocache")));
uint8_t g_dst_buf[DST_BUFFER_SIZE] __attribute__((section(".ram_nocache")));

/************************* DMA 回调函数 *************************/
CODE_AREA
void dma_callback(dmac_callback_args_t * cb_data)
{
    FSP_PARAMETER_NOT_USED(cb_data);
    g_dma_transfer_count++;

    printf("DMA Repeat-Block Callback triggered! Transfer count: %u\r\n", g_dma_transfer_count);

    g_dma_trans_complete = true;
}

/************************* DMA 初始化 (Repeat-Block 模式) *************************/
CODE_AREA
fsp_err_t dma_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    printf("\r\n==============================================\r\n");
    printf("DMA Repeat-Block Transfer Mode Example\r\n");
    printf("Implementing Figure 17.5 from RA8T2 Manual\r\n");
    printf("==============================================\r\n");

    printf("\r\nConfiguration:\r\n");
    printf("  - Transfer Mode: Repeat-Block (Reload Mode)\r\n");
    printf("  - Block Size: %d bytes\r\n", BLOCK_SIZE);
    printf("  - Blocks per repeat: %d blocks\r\n", NUM_BLOCKS);
    printf("  - Repeat times: %d\r\n", TOTAL_REPEAT_TIMES);
    printf("  - Total transfers: %d bytes\r\n", TOTAL_TRANSFERS);
    printf("  - Source Buffer Size: %d bytes\r\n", SRC_BUFFER_SIZE);
    printf("  - Destination Buffer Size: %d bytes\r\n", DST_BUFFER_SIZE);

    printf("\r\nInitializing DMA in Repeat-Block Mode...\r\n");
    for (uint8_t i = 0; i < SRC_BUFFER_SIZE; i++)
    {
        g_src_buf[i] = i+1;
    }
    for (uint8_t i = 0; i < DST_BUFFER_SIZE; i++)
    {
        g_dst_buf[i] = 0;
    }
    /* 打印初始缓冲区状态 */
    //printf("\r\nInitial Buffer State:\r\n");
    //print_buffers();

    g_transfer0_cfg.p_info->p_src  = (void *)g_src_buf;
    g_transfer0_cfg.p_info->p_dest = (void *)g_dst_buf;

    /* 打开 DMA 通道 */
    err = R_DMAC_Open(&g_transfer0_ctrl, &g_transfer0_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA OPEN FAILED (0x%08X)\r\n", err);
        return err;
    }

    printf("DMA Repeat-Block Mode initialized successfully.\r\n");
    return err;
}

/************************* DMA 测试函数 *************************/
CODE_AREA
void dma_test(void)
{
    fsp_err_t err;
    g_dma_trans_complete = false;
    g_dma_transfer_count = 0;

    printf("\r\n--- Starting Repeat-Block DMA Transfer ---\r\n");

    /* 启用 DMA 通道 */
    err = R_DMAC_Enable(&g_transfer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA Enable Failed (0x%08X)\r\n", err);
        return;
    }
    err = R_DMAC_SoftwareStart(&g_transfer0_ctrl, TRANSFER_START_MODE_REPEAT);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA Software Start Failed (0x%08X)\r\n", err);
        return;
    }


    /* 等待传输完成 */
    uint32_t timeout_ms = 0;
    uint32_t max_timeout = 5000; // 5秒超时
    while (!g_dma_trans_complete)
    {
        if (timeout_ms >= max_timeout)
        {
            printf("Error: DMA Transfer Timeout after %u ms\r\n", max_timeout);
            break;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        timeout_ms++;
    }

    /* 获取DMA状态信息 */
   // transfer_properties_t dma_props;
 /*   err = R_DMAC_InfoGet(&g_transfer0_ctrl, &dma_props);
    if (FSP_SUCCESS == err)
    {
        printf("\r\n[DMA Status After Transfer]\r\n");
        printf("  transfer_length_max: %u\r\n", dma_props.transfer_length_max);
        printf("  transfer_length_remaining: %u\r\n", dma_props.transfer_length_remaining);
        printf("  block_count_max: %u\r\n", dma_props.block_count_max);
        printf("  block_count_remaining: %u\r\n", dma_props.block_count_remaining);
    }*/

    if (dma_data_verify())
    {
        printf("\r\nSUCCESS: Repeat-Block DMA transfer completed correctly!\r\n");
    }
    else
    {
        printf("\r\nFAILURE: Data verification failed!\r\n");
    }

}

/************************* 停止 DMA *************************/
CODE_AREA
void dma_stop(void)
{
    fsp_err_t err;

    /* 禁用 DMA 通道 */
    err = R_DMAC_Disable(&g_transfer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA Disable Failed (0x%08X)\r\n", err);
    }

    /* 关闭 DMA 通道 */
    err = R_DMAC_Close(&g_transfer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA Close Failed (0x%08X)\r\n", err);
    }

    printf("\r\nDMA Repeat-Block channel closed.\r\n");
}

/************************* 数据验证函数 *************************/
CODE_AREA
bool dma_data_verify(void)
{
    bool success = true;

    printf("\r\nVerifying data transfer...\r\n");

    /* 在Repeat-Block模式下，数据应该从源缓冲区传输到目标缓冲区
     * 这里我们验证目标缓冲区是否被正确填充
     */
    bool all_zeros = true;
    for (uint8_t i = 0; i < DST_BUFFER_SIZE; i++)
    {
        if (g_dst_buf[i] != 0)
        {
            all_zeros = false;
            break;
        }
    }

    if (all_zeros)
    {
        printf("  ERROR: Destination buffer is all zeros!\r\n");
        success = false;
    }
    else
    {
        printf("  Destination buffer contains data.\r\n");
    }

    /* 更详细的验证：比较源和目标的特定模式 */
    printf("  Source pattern:     ");
    for (uint8_t i = 0; i < SRC_BUFFER_SIZE; i++)
    {
        printf("%02X ", g_src_buf[i]);
    }
    printf("\r\n");

    printf("  Destination result: ");
    for (uint8_t i = 0; i < DST_BUFFER_SIZE; i++)
    {
        printf("%02X ", g_dst_buf[i]);
    }
    printf("\r\n");

    return success;
}

/************************* 打印缓冲区内容 *************************/
CODE_AREA
void print_buffers(void)
{
    /* 修复格式字符串：使用 %08X 代替 %08lX */
    printf("Source Buffer (%d bytes at 0x%08X):\r\n  ",
           SRC_BUFFER_SIZE, (uint32_t)g_src_buf);
    for (uint8_t i = 0; i < SRC_BUFFER_SIZE; i++)
    {
        printf("0x%02X ", g_src_buf[i]);
        if ((i + 1) % 4 == 0 && i != SRC_BUFFER_SIZE - 1)
        {
            printf("\r\n  ");
        }
    }
    printf("\r\n");

    printf("Destination Buffer (%d bytes at 0x%08X):\r\n  ",
           DST_BUFFER_SIZE, (uint32_t)g_dst_buf);
    for (uint8_t i = 0; i < DST_BUFFER_SIZE; i++)
    {
        printf("0x%02X ", g_dst_buf[i]);
        if ((i + 1) % 4 == 0 && i != DST_BUFFER_SIZE - 1)
        {
            printf("\r\n  ");
        }
    }
    printf("\r\n");
}
