#include "hal_data.h"
#include "common_utils.h"
#include "lwprintf.h"


static uint8_t  g_out_of_band_received[TRANSFER_LENGTH];
volatile uint32_t g_transfer_complete = 0;
volatile uint32_t g_receive_complete  = 0;
uint32_t g_out_of_band_index = 0;

#define TX_BUF_SIZE 256
static uint8_t tx_ringbuf[TX_BUF_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile bool uart_tx_busy = false;

int lwprintf_output_func(int ch, lwprintf_t *lw) {
    (void)lw;  // 如果你不需要用到 lw 参数
    uint16_t next = (tx_head + 1) % TX_BUF_SIZE;
    if (next == tx_tail) {
        return 0; // buffer full, drop
    }
    tx_ringbuf[tx_head] = (uint8_t)ch;
    tx_head = next;

    if (!uart_tx_busy) {
        uart_tx_busy = true;
        uint8_t c = tx_ringbuf[tx_tail];
        R_SCI_B_UART_Write(&g_uart0_ctrl, &c, 1);
    }
    return 1;  // 表示成功处理
}

void Debug_UART0_Init(void)
{
   fsp_err_t err = FSP_SUCCESS;

   err = R_SCI_B_UART_Open (&g_uart0_ctrl, &g_uart0_cfg);
   assert(FSP_SUCCESS == err);
}

////void sci9_uart_callback (uart_callback_args_t * p_args)
////{
    /* Handle the UART event */
////    switch (p_args->event)
////    {
        /* Received a character */
////        case UART_EVENT_RX_CHAR:
////        {
            /* Only put the next character in the receive buffer if there is space for it */
////            if (sizeof(g_out_of_band_received) > g_out_of_band_index)
////            {
                /* Write either the next one or two bytes depending on the receive data size */
////                if ((UART_DATA_BITS_7 == g_uart0_cfg.data_bits) || (UART_DATA_BITS_8 == g_uart0_cfg.data_bits))
////                {
////                    g_out_of_band_received[g_out_of_band_index++] = (uint8_t) p_args->data;
////                }
////                else
////                {
////                    uint16_t * p_dest = (uint16_t *) &g_out_of_band_received[g_out_of_band_index];
////                    *p_dest              = (uint16_t) p_args->data;
////                    g_out_of_band_index += 2;
////                }
////            }
////            break;
////        }
        /* Receive complete */
////        case UART_EVENT_RX_COMPLETE:
////        {
////            g_receive_complete = 1;
////            break;
////        }
        /* Transmit complete */
////        case UART_EVENT_TX_COMPLETE:
////        {
////            g_transfer_complete = 1;
////            break;
////        }
////        default:
////        {
////        }
////    }
////}

void sci0_uart_callback(uart_callback_args_t *p_args)
{
    /* TODO: add your own code here */

    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            /* 把串口接收到的数据发送回去 */
//            R_SCI_B_UART_Write(&g_uart8_ctrl, (uint8_t *)&(p_args->data), 1);
            break;
        }
        case UART_EVENT_TX_COMPLETE:
        {
//            uart_tx_busy = true;
//            test++;
//            R_SCI_B_UART_Write(&g_uart9_ctrl, &test, 1);

                    tx_tail = (tx_tail + 1) % TX_BUF_SIZE;
                    if (tx_tail != tx_head) {
                        uint8_t c = tx_ringbuf[tx_tail];
                        R_SCI_B_UART_Write(&g_uart0_ctrl, &c, 1);
                    } else {
                        uart_tx_busy = false;
                    }
            break;
        }
        default:
            break;
    }
}
