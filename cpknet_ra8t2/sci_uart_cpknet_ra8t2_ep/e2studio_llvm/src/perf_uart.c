/*
 * perf_uart.c
 *
 *  Created on: Jan 26, 2026
 *      Author: Ran QingLing
 */
#include"hal_data.h"
#include"perf_uart.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

DATA_AREA_BSS uint8_t  g_dest[16];
DATA_AREA_BSS uint8_t  g_src[16];
DATA_AREA_BSS uint8_t  g_out_of_band_received[16];
DATA_AREA_DATA volatile uint32_t g_transfer_complete = 0;
DATA_AREA_DATA volatile uint32_t g_receive_complete  = 0;
DATA_AREA_DATA volatile uint32_t g_out_of_band_index = 0;

void uart_initialize(void)
{
    fsp_err_t err = FSP_SUCCESS;
    printf("\r\nSCI-UART example code to demonstrate the functionality.\r\n");
    printf("\r\nSCI1 used as UART0,RXD0=P401[J201:4], TXD0 =P400[J201:4]\r\n");
    printf("connect P400 to P401\r\n");
    printf("UART initialize......\r\n");

    err = R_SCI_B_UART_Open (&g_uart0_ctrl, &g_uart0_cfg);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_"UART_TYPE"_Open API failed **\r\n");
    }

}
CODE_AREA
void uart0_callback (uart_callback_args_t * p_args)
{
    /* Handle the UART event */
    switch (p_args->event)
    {
        /* Received a character */
        case UART_EVENT_RX_CHAR:
        {
            /* Only put the next character in the receive buffer if there is space for it */
            if (sizeof(g_out_of_band_received) > g_out_of_band_index)
            {
                /* Write either the next one or two bytes depending on the receive data size */
                if (UART_DATA_BITS_8 >= g_uart0_cfg.data_bits)
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
CODE_AREA
void uart1_callback (uart_callback_args_t * p_args)
{
    /* Handle the UART event */
    switch (p_args->event)
    {
        /* Received a character */
        case UART_EVENT_RX_CHAR:
        {
            /* Only put the next character in the receive buffer if there is space for it */
            if (sizeof(g_out_of_band_received) > g_out_of_band_index)
            {
                /* Write either the next one or two bytes depending on the receive data size */
                if (UART_DATA_BITS_8 >= g_uart1_cfg.data_bits)
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
CODE_AREA
void uart2_callback (uart_callback_args_t * p_args)
{
    /* Handle the UART event */
    switch (p_args->event)
    {
        /* Received a character */
        case UART_EVENT_RX_CHAR:
        {
            /* Only put the next character in the receive buffer if there is space for it */
            if (sizeof(g_out_of_band_received) > g_out_of_band_index)
            {
                /* Write either the next one or two bytes depending on the receive data size */
                if (UART_DATA_BITS_8 >= g_uart2_cfg.data_bits)
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
CODE_AREA
void uart_read_write(uart_ctrl_t * const p_api_ctrl)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_SCI_B_UART_Read(p_api_ctrl, g_dest, 16);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_SCI_B_UART_Read API failed **\r\n");
    }

    err = R_SCI_B_UART_Write(p_api_ctrl, g_src, 16);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_SCI_B_UART_Write API failed **\r\n");
    }

    uint32_t wait_count = 0;
    while (!g_transfer_complete && (wait_count++ < 9000))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    if (wait_count >= 9000) {
        printf("\r\nError UART Transfer Timeout\r\n");
    }

    wait_count = 0;
    while (!g_receive_complete && (wait_count++ < 500))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    if (wait_count >= 9000) {
        printf("\r\nError UART Receive Timeout\r\n");
    }

    if (0 == memcmp(g_src, g_dest, 16))
    {
        printf("UART transmit SUCCESS! g_src and g_dest match\r\n");
    }
    else
    {
        printf("Data comparison FAILED! Data mismatch detected\r\n");
        for(uint8_t i = 0; i < 16; i++)
        {
            if(g_src[i] != g_dest[i])
            {
                printf("Mismatch at index %d: Sent 0x%02X, Received 0x%02X\r\n",
                       i, g_src[i], g_dest[i]);
            }
        }
    }

    memset(g_dest, 0, sizeof(g_dest));
    g_transfer_complete = 0;
    g_receive_complete = 0;
}
CODE_AREA
void uart_perf_test(void)
{
    fsp_err_t err = FSP_SUCCESS;
    for(uint8_t i=0;i<16;i++)
    {
        g_src[i]=i+1;
    }
    printf("UART baud rate is 115200......\r\n");
    uart_read_write(&g_uart0_ctrl);
    R_SCI_B_UART_Close(&g_uart0_ctrl);
    printf("UART baud rate is 2M......\r\n");
    err = R_SCI_B_UART_Open (&g_uart1_ctrl, &g_uart1_cfg);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_"UART_TYPE"_Open API failed **\r\n");
    }
    uart_read_write(&g_uart1_ctrl);
    R_SCI_B_UART_Close(&g_uart1_ctrl);
    printf("UART baud rate is 5M......\r\n");
    err = R_SCI_B_UART_Open (&g_uart2_ctrl, &g_uart2_cfg);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_"UART_TYPE"_Open API failed **\r\n");
    }
    uart_read_write(&g_uart2_ctrl);
    R_SCI_B_UART_Close(&g_uart2_ctrl);
}
CODE_AREA
void r_sci_b_uart_baud_set (uint32_t baudrate_value)
{
    sci_b_baud_setting_t baud_setting;
    uint32_t             baud_rate                 = baudrate_value;
    bool                 enable_bitrate_modulation = false;
    uint32_t             error_rate_x_1000         = 3000;
    printf("Set UART Baudrate: %u bps\r\n", baudrate_value);
    fsp_err_t err = R_SCI_B_UART_BaudCalculate(baud_rate, enable_bitrate_modulation, error_rate_x_1000, &baud_setting);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_SCI_B_UART_BaudCalculate API failed **\r\n");
    }
    err = R_SCI_B_UART_BaudSet(&g_uart0_ctrl, (void *) &baud_setting);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_SCI_B_UART_BaudSet API failed **\r\n");
    }
}

void r_sci_b_uart_baud_set9 (uint32_t baudrate_value)
{
    sci_b_baud_setting_t baud_setting;
    uint32_t             baud_rate                 = baudrate_value;
    bool                 enable_bitrate_modulation = false;
    uint32_t             error_rate_x_1000         = 3000;
   // printf("Set UART Baudrate: %u bps\r\n", baudrate_value);
    fsp_err_t err = R_SCI_B_UART_BaudCalculate(baud_rate, enable_bitrate_modulation, error_rate_x_1000, &baud_setting);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_SCI_B_UART_BaudCalculate API failed **\r\n");
    }
    err = R_SCI_B_UART_BaudSet(&g_uart9_ctrl, (void *) &baud_setting);
    if (FSP_SUCCESS != err)
    {
        printf ("\r\n** R_SCI_B_UART_BaudSet API failed **\r\n");
    }
}

#define BSP_PRV_PERIPHERAL_CLK_REQ_BIT_POS1      (6U)
#define BSP_PRV_PERIPHERAL_CLK_REQ_BIT_MASK1     (1U << BSP_PRV_PERIPHERAL_CLK_REQ_BIT_POS1)
#define BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS1      (7U)
#define BSP_PRV_PERIPHERAL_CLK_RDY_BIT_MASK1     (1U << BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS1)
void bsp_peripheral_clock_set1 (volatile uint8_t * p_clk_ctrl_reg,
                                      volatile uint8_t * p_clk_div_reg,
                                      uint8_t            peripheral_clk_div,
                                      uint8_t            peripheral_clk_source)
{
    /* Request to stop the peripheral clock. */
    *p_clk_ctrl_reg |= (uint8_t) BSP_PRV_PERIPHERAL_CLK_REQ_BIT_MASK1;

    /* Wait for the peripheral clock to stop. */
    FSP_HARDWARE_REGISTER_WAIT((uint8_t) ((*p_clk_ctrl_reg & BSP_PRV_PERIPHERAL_CLK_RDY_BIT_MASK1) >>
                                          BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS1),
                               1U);

    /* Select the peripheral clock divisor and source. */
    *p_clk_div_reg  = peripheral_clk_div;
    *p_clk_ctrl_reg = peripheral_clk_source | BSP_PRV_PERIPHERAL_CLK_REQ_BIT_MASK1 |
                      BSP_PRV_PERIPHERAL_CLK_RDY_BIT_MASK1;

    /* Request to start the peripheral clock. */
    *p_clk_ctrl_reg &= (uint8_t) ~BSP_PRV_PERIPHERAL_CLK_REQ_BIT_MASK1;

    /* Wait for the peripheral clock to start. */
    FSP_HARDWARE_REGISTER_WAIT((uint8_t) ((*p_clk_ctrl_reg & BSP_PRV_PERIPHERAL_CLK_RDY_BIT_MASK1) >>
                                          BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS1),
                               0U);
}
