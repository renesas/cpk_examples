/*
 * perf_dma_normal.c
 *
 *  Created on: Jan 29, 2026
 *      Author: a5143926
 */

#include "hal_data.h"
#include "perf_dma_normal.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

#define transfer_length  (16)
// Definition of global status flags
volatile bool g_dma_trans_complete = false;
volatile bool g_dma_trans_error = false;

uint8_t g_src_buf[16]__attribute__((section(".ram_nocache")));

uint8_t g_dst_buf[16]__attribute__((section(".ram_nocache")));

/************************* Implementation of Interrupt Callback Functions *************************/
CODE_AREA
void dma_callback(dmac_callback_args_t * cb_data)
{
    FSP_PARAMETER_NOT_USED(cb_data);
    printf("DMA Callback triggered!\r\n");
    g_dma_trans_complete = true;
}
/************************* Implementation of DMA Initialization *************************/
CODE_AREA
fsp_err_t dma_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    printf("\r\nDMA normal mode example code to demonstrate the functionality.\r\n");
    printf("\r\nset DMA0 to normal mode, transfer 16 bytes data form g_src_buf to g_dst_buf \r\n");
    printf("DMA initialize......\r\n");

    for (uint8_t i = 0; i < transfer_length; i++)
    {
        g_src_buf[i] = i+1;
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
        printf("[DMA status] transfer_length_max: %u bytes\r\n", dma_props.transfer_length_max);
        printf("[DMA status] transfer_length_remaining: %u bytes\r\n", dma_props.transfer_length_remaining);
        printf("[DMA status] block_count_max: %u\r\n", dma_props.block_count_max);
        printf("[DMA status] block_count_remaining: %u\r\n", dma_props.block_count_remaining);
    }
    else
    {
        printf("err: R_DMAC_InfoGet (0x%08x)\r\n", info_err);
    }
    if (true == dma_data_verify())
    {
        printf("DMA Transfer successful\r\n");
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
    for (uint8_t i = 0; i < transfer_length; i++)
    {
        if (g_src_buf[i] != g_dst_buf[i])
        {
            return false; // Data inconsistent, transfer failed
        }
    }
    return true; // Data consistent, transfer successful
}

