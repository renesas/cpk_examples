/*
 * perf_dma_block.c
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#include "hal_data.h"
#include "perf_dma_block.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

#define BLOCK_SIZE      (4)
#define NUM_BLOCKS      (3)
#define TOTAL_LENGTH    (BLOCK_SIZE * NUM_BLOCKS)

// Definition of global status flags
DATA_AREA_BSS volatile bool g_dma_trans_complete = false;
DATA_AREA_BSS volatile bool g_dma_trans_error = false;
DATA_AREA_BSS volatile uint8_t g_current_block = 0;
uint8_t g_src_buf[TOTAL_LENGTH] __attribute__((section(".ram_nocache")));
uint8_t g_dst_buf[BLOCK_SIZE] __attribute__((section(".ram_nocache")));


/************************* Implementation of Interrupt Callback Functions *************************/
CODE_AREA
void dma_callback(dmac_callback_args_t * cb_data)
{
    FSP_PARAMETER_NOT_USED(cb_data);
    g_current_block++;
    printf("DMA Callback occur.\r\n");
    g_dma_trans_complete = true;
}
/************************* Implementation of DMA Initialization *************************/
CODE_AREA
fsp_err_t dma_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    printf("\r\nDMA BLOCK mode example: 3 source blocks -> 1 destination area (Overwrite).\r\n");
    printf("Configuration according to Figure 17.4 in RA8T2 Manual.\r\n");
    printf("Set DMA0 to BLOCK mode. Destination is specified as the Block Area.\r\n");
    printf("Transfer %d blocks, each block is %d bytes.\r\n", NUM_BLOCKS, BLOCK_SIZE);
    printf("Source address increments. Destination address resets after each block.\r\n");
    printf("DMA initializing......\r\n");

    for (uint8_t i = 0; i < TOTAL_LENGTH; i++)
    {
        g_src_buf[i] = i+1;
    }
    for (uint8_t i = 0; i < BLOCK_SIZE; i++)
    {
        g_dst_buf[i] = 0;
    }
    g_transfer0_cfg.p_info->p_src = (void *) g_src_buf;
    g_transfer0_cfg.p_info->p_dest = (void *) g_dst_buf;

    err = R_DMAC_Open(&g_transfer0_ctrl, &g_transfer0_cfg);
    /* Handle any errors. This function should be defined by the user. */
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA OPEN FAILED\r\n");
    }
    return err;

}
CODE_AREA
void dma_test(void)
{
    g_dma_trans_complete = false;
    g_current_block = 0;
    printf("\r\n--- Starting DMA Block Transfer  ---\r\n");
    /* Trigger the transfer using software. */
    /* Enable DMAC transfers. */
    R_DMAC_Enable(&g_transfer0_ctrl);
    fsp_err_t err = R_DMAC_SoftwareStart(&g_transfer0_ctrl, TRANSFER_START_MODE_REPEAT);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA software start FAILED\r\n");
    }
    uint32_t timeout_ms = 0;
    while (!g_dma_trans_complete)
    {
        if (timeout_ms >= 1000)
        {
            printf("Error: DMA Transfer Timeout 1000ms\r\n");
            break;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        timeout_ms++;
    }
    transfer_properties_t dma_props;
    fsp_err_t info_err = R_DMAC_InfoGet(&g_transfer0_ctrl, &dma_props);
    if (FSP_SUCCESS == info_err)
    {
        printf("[DMA Status After Transfer]\r\n");
        printf("transfer_length_remaining: %u bytes\r\n", dma_props.transfer_length_remaining);
        printf("block_count_remaining: %u\r\n", dma_props.block_count_remaining); // 完成後应为0
    }
    printf("\r\nVerifying transfer result...\r\n");
    printf("Source blocks: [1,2,3,4], [5,6,7,8], [9,10,11,12]\r\n");
    printf("Destination (after 3 blocks overwrite): ");
    for (uint8_t i = 0; i < BLOCK_SIZE; i++)
    {
        printf("0x%02X ", g_dst_buf[i]);
    }
    printf("\r\n");
    if (true == dma_data_verify())
    {
        printf("SUCCESS: DMA Block Transfer (Overwrite) completed correctly.\r\n");
        printf("         Destination contains data from the LAST source block (9,10,11,12).\r\n");
    }
    else printf("Error: DMA Transfer failed\r\n");

}
CODE_AREA
void dma_stop(void)
{
    fsp_err_t err = R_DMAC_Disable(&g_transfer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA disable  FAILED\r\n");
    }
    err = R_DMAC_Close(&g_transfer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("Error: DMA close  FAILED\r\n");
    }
    printf("DMA closed\r\n");
}
/************************* Implementation of Data Verification *************************/
CODE_AREA
bool dma_data_verify(void)
{
    for (uint8_t i = 0; i < BLOCK_SIZE; i++)
    {
        uint8_t expected_data = g_src_buf[i + (BLOCK_SIZE * (NUM_BLOCKS - 1))]; // 索引 8,9,10,11
        if (expected_data != g_dst_buf[i])
        {
            printf("Data mismatch at dest[%d]: expected=0x%02X, actual=0x%02X\r\n",
                    i, expected_data, g_dst_buf[i]);
            return false;
        }
    }
    return true;
}

