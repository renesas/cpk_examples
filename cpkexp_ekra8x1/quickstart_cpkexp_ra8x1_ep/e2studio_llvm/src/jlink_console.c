/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : usb_console_main.c
 * Description  : Entry function.
 *********************************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "r_typedefs.h"

#include "common_utils.h"
#include "common_init.h"
#include "board_cfg.h"
#include "jlink_console.h"
#include "log.h"

#define TRANSFER_LENGTH    (1024)

static uint8_t  g_out_of_band_received[TRANSFER_LENGTH];
static uint32_t g_out_of_band_index = 0;
static uint8_t  s_rx_buf;

uint32_t g_transfer_complete = 0;
uint32_t g_receive_complete  = 0;

static void Jlink_console_write(const char_t * buffer);

void jlink_console_init (void)
{
    R_SCI_B_UART_Open(&g_jlink_console_ctrl, &g_jlink_console_cfg);
    R_SCI_B_UART_Open(g_uart4.p_ctrl, g_uart4.p_cfg);
    LOG_Reset();
}

fsp_err_t print_to_console (char_t * p_data)
{
    fsp_err_t err = FSP_SUCCESS;

    Jlink_console_write(p_data);

    return err;
}

int8_t input_from_console (void)
{
    start_key_check();

    while (key_pressed() == false)
    {
        vTaskDelay(1);
    }

    return (int8_t)get_detected_key();
}

#if LOG_CFG_EN_SEGGER_RTT == 0
/**
 * @brief   LOG port function. Implementation of LOG_Puts
 * @param   str Output string
 */
void LOG_Puts(const char *str)
{
    int i;

    for (i = 0; str[i]; i++) {
        R_SCI_B4->TDR_BY = str[i];
        while ((R_SCI_B4->CSR & 0x20000000) == 0) {}
    }
}
#endif

void __assert_func(const char * file, int line, const char * func, const char * expr)
{
    __disable_irq();
    LOG_E(__FUNCTION__, "====== Assert Error ======");
    LOG_Puts("File ");
    LOG_Puts(file);
    LOG_PrintfEndl(" at line: %d", line);
    LOG_Puts("Function: ");
    LOG_PutsEndl(func);
    LOG_Puts("Assert: ");
    LOG_PutsEndl(expr);
    while (1) {
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_10_PIN_01, BSP_IO_LEVEL_LOW);
        R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_10_PIN_01, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

void HardFault_Handler(void)
{
    uint32_t msp;

    uint32_t *p_stack = NULL;

    __disable_irq();
    LOG_E(NULL, "In HardFault");
    if (SCB->HFSR & 0x80000000) {
        LOG_PutsEndl("HardFault occurred by DEBUG");
    }
    else if (SCB->HFSR & 0x40000000) {
        LOG_PutsEndl("HardFault occurred by other exception forced");
    }

    msp = __get_MSP();
    p_stack = (uint32_t *)msp;
    LOG_PutsEndl("========== MSP Stack  ==========");
    LOG_PrintfEndl("R0:  0x%08X", p_stack[0]);
    LOG_PrintfEndl("R1:  0x%08X", p_stack[1]);
    LOG_PrintfEndl("R2:  0x%08X", p_stack[2]);
    LOG_PrintfEndl("R3:  0x%08X", p_stack[3]);
    LOG_PrintfEndl("R12: 0x%08X", p_stack[4]);
    LOG_PrintfEndl("LR:  0x%08X", p_stack[5]);
    LOG_PrintfEndl("PC:  0x%08X", p_stack[6]);
    LOG_PrintfEndl("PSR: 0x%08X", p_stack[7]);

    LOG_PutsEndl("========= SCB Register =========");
    LOG_PrintfEndl("SCB->CSFR: 0x%08X", SCB->CFSR);
    LOG_PrintfEndl("SCB->ICSR: 0x%08X", SCB->ICSR);
    LOG_PrintfEndl("SCB->HFSR: 0x%08X", SCB->HFSR);

    LOG_PutsEndl("======== Other Register ========");
    LOG_PrintfEndl("IPSR: 0x%08X", __get_IPSR());

    if (SCB->CFSR & 0x02000000) {
        LOG_PutsEndl("Checked Error: Divide By Zero");
    }
    if (SCB->CFSR & 0x01000000) {
        LOG_PutsEndl("Checked Error: Unaligned Access");
    }
    if (SCB->CFSR & 0x00100000) {
        LOG_PutsEndl("Checked Error: Stack Overflow");
        LOG_PrintfEndl("MSP:    0x%08X", msp);
        LOG_PrintfEndl("MSPLIM: 0x%08X", __get_MSPLIM());
        LOG_PrintfEndl("PSP:    0x%08X", __get_PSP());
        LOG_PrintfEndl("PSPLIM: 0x%08X", __get_PSPLIM());
    }
    if (SCB->CFSR & 0x00080000) {
        LOG_PutsEndl("Checked Error: No Coprocessor");
    }
    if (SCB->CFSR & 0x00040000) {
        LOG_PutsEndl("Checked Error: Invalid PC");
    }
    if (SCB->CFSR & 0x00020000) {
        LOG_PutsEndl("Checked Error: Invalid State");
    }
    if (SCB->CFSR & 0x00010000) {
        LOG_PutsEndl("Checked Error: Undefined Instruction");
    }
    if (SCB->CFSR & 0x00000100) {
        LOG_PutsEndl("Checked Error: Instruction Bus Error");
    }

    while (1) {
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_00, BSP_IO_LEVEL_LOW);
        R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_00, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

#if LOG_CFG_EN_TIMESTAMP
/**
 * @brief   LOG port function. Implementation of LOG_GetTime
 * @param   s  Time s
 * @param   ms Time ms
 */
void LOG_GetTime(uint32_t *s, uint32_t *ms)
{
    uint32_t tick;

    if (R_FSP_CurrentIrqGet() >= 0) {
        tick = xTaskGetTickCountFromISR();
    }
    else {
        tick = xTaskGetTickCount();
    }
    *s = tick / 1000;
    *ms = tick % 1000;
}
#endif

static void Jlink_console_write (const char_t * buffer)
{
    fsp_err_t err = FSP_SUCCESS;

    g_transfer_complete = false;

    err = R_SCI_B_UART_Write(&g_jlink_console_ctrl, (uint8_t *) buffer, strlen(buffer));

    assert(FSP_SUCCESS == err);

    while (!g_transfer_complete)
    {
        vTaskDelay(1);
    }
}

void start_key_check_fifo (void)
{
    s_rx_buf           = 0;
    g_receive_complete = false;
}

void start_key_check (void)
{
    s_rx_buf           = 0;
    g_receive_complete = false;

    R_SCI_B_UART_Read(&g_jlink_console_ctrl, &s_rx_buf, 1);
}

uint8_t get_detected_key (void)
{
    return s_rx_buf;
}

bool_t key_pressed (void)
{
    return g_receive_complete;
}

void jlink_console_callback (uart_callback_args_t * p_args)
{
    /* Handle the UART event */
    switch (p_args->event)
    {
        /* Received a character */
        case UART_EVENT_RX_CHAR:
        {
            /* Only put the next character in the receive buffer if there is space for it */
            if ((sizeof(g_out_of_band_received)) > g_out_of_band_index)
            {
                /* Write either the next one or two bytes depending on the receive data size */
                if (UART_DATA_BITS_8 >= g_jlink_console_cfg.data_bits)
                {
                    g_out_of_band_received[g_out_of_band_index++] = (uint8_t) p_args->data;
                }
                else
                {
                    uint16_t * p_dest = (uint16_t *) &g_out_of_band_received[g_out_of_band_index];
                    *p_dest              = (uint16_t) p_args->data;
                    g_out_of_band_index += 2;
                }
            }

            break;
        }

        /* Receive complete */
        case UART_EVENT_RX_COMPLETE:
        {
            g_receive_complete = 1;
            break;
        }

        /* Transmit complete */
        case UART_EVENT_TX_COMPLETE:
        {
            g_transfer_complete = 1;
            break;
        }

        default:
        {
        }
    }
}

uint8_t get_new_chars (uint8_t * pBuf)
{
    uint8_t x   = 0U;
    uint8_t ret = 0U;

    /* Check if single character received. */
    if (g_receive_complete)
    {
        pBuf[0]            = s_rx_buf;
        g_receive_complete = 0;

        return 1;
    }
    else if (g_out_of_band_index == 0)
    {
        return 0;
    }
    else
    {
        /* Continue. */
    }

    for (x = 0; x < g_out_of_band_index; x++)
    {
        pBuf[x] = g_out_of_band_received[x];
    }

    memset(g_out_of_band_received, 0, g_out_of_band_index + 1);
    ret = (uint8_t)g_out_of_band_index;

    g_out_of_band_index = 0U;

    return ret;
}

static void Jlink_console_putchar(char ch)
{
    fsp_err_t err = FSP_SUCCESS;

    g_transfer_complete = false;

    err = R_SCI_B_UART_Write(&g_jlink_console_ctrl, (uint8_t *)&ch, 1);

    assert(FSP_SUCCESS == err);

    while (!g_transfer_complete)
    {
        vTaskDelay(1);
    }
}

void _exit(int code)
{
	(void)code;
        __asm("BKPT #0\n");
}

int segger_write(char ptr, FILE *file)
{
	(void)file;
	if (ptr == '\n')
		Jlink_console_putchar('\r');
	Jlink_console_putchar(ptr);

	return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(segger_write, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
