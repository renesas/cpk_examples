#include "hal_data.h"
//#include "LVGL_thread.h"
#include "stdio.h"
#include <sys/stat.h>
//#include <errno.h>
//#undef errno
//extern int errno;

int _write(int file, char *ptr, int len);
int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _read(int file, char *ptr, int len);
int _lseek(int file, int ptr, int dir);

#define DEBUG_SERIAL_TIMEOUT 2000

static bool uart_tx_completed = false;

void uart_callback(uart_callback_args_t *p_args)
{
//    BaseType_t xHigherPriorityTaskWoken;
//    BaseType_t xResult = pdFAIL;
//
//    if (UART_EVENT_TX_COMPLETE == p_args->event)
//    {
//        xResult = xSemaphoreGiveFromISR( g_serial_binary_semaphore, &xHigherPriorityTaskWoken );
//    }
//
//    if( pdFAIL != xResult)
//    {
//        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//    }
    if( p_args->event == UART_EVENT_TX_COMPLETE ){
        uart_tx_completed = true;
    }

}

int _write(int file, char *ptr, int len)
{
    fsp_err_t err = FSP_SUCCESS;
    FSP_PARAMETER_NOT_USED(file);

    static bool uart_open = false;

    if (false == uart_open)
    {
        err = R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
      if (FSP_SUCCESS == err)
      {
          uart_open = true;
//          xSemaphoreGive( g_serial_binary_semaphore); //allow the first write to work.
      }

    }

    if (FSP_SUCCESS == err)
    {
#if defined(RENESAS_CORTEX_M85)
#if (BSP_CFG_DCACHE_ENABLED)
        SCB_CleanInvalidateDCache_by_Addr(ptr, len); //DTC is configured for UART TX
#endif
#endif

          err = R_SCI_B_UART_Write(&g_uart0_ctrl, (uint8_t *)ptr, (uint32_t)len);
          if (FSP_SUCCESS == err)
          {
              /* Wait for the previous USB Write to complete */
//              if( xSemaphoreTake( g_serial_binary_semaphore, DEBUG_SERIAL_TIMEOUT ) != pdTRUE )
//              {
//                  __BKPT(0);
//              }
              while(uart_tx_completed == false)
              {
                  ;
              }
              uart_tx_completed = false;
          }
          else
          {
              __BKPT(0);
          }

    }

    if (FSP_SUCCESS != err)
    {
        len = -1;
    }

    return len;
}

int _close(int file)
{
  FSP_PARAMETER_NOT_USED(file);
  return -1;
}
int _fstat(int file, struct stat *st)
{
    FSP_PARAMETER_NOT_USED(file);
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
    FSP_PARAMETER_NOT_USED(file);
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
    FSP_PARAMETER_NOT_USED(file);
    FSP_PARAMETER_NOT_USED(ptr);
    FSP_PARAMETER_NOT_USED(dir);
  return 0;
}

int _read(int file, char *ptr, int len)
{
    FSP_PARAMETER_NOT_USED(file);
    FSP_PARAMETER_NOT_USED(ptr);
    FSP_PARAMETER_NOT_USED(len);
    return 0;
}
