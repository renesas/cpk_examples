/*
 * perf_agt.c
 *
 *  Created on: Feb 5, 2026
 *      Author: Ran Qingling
 */
#include <perf_crc.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

/* Flags to indicate UART TX, RX events */
DATA_AREA_BSS static volatile bool b_uart_rxflag = false;
DATA_AREA_BSS static volatile bool b_uart_txflag = false;

/* For on board LEDs */
extern bsp_leds_t g_bsp_leds;

/* Private functions */
static fsp_err_t crc_operation(void);
static void set_led(bsp_io_level_t b_value);
CODE_AREA
void crc_test(void)
{
    fsp_err_t   err                     = FSP_SUCCESS;

    /* Open UART module */
    err = R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    /* Handle error */
        if (FSP_SUCCESS != err)
        {
            /* Display failure message in RTT */
            printf("R_SCI_B_UART_Open API FAILED\r\n");
            printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
        }

    /* Open CRC module */
    err = R_CRC_Open(&g_crc_ctrl, &g_crc_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* Close opened UART module */
        deinit_uart();
        /* Display failure message in RTT */
        printf("R_CRC_Open API FAILED\r\n");
        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
    }
    printf("\r\nStart CRC Operation\r\n");
     /* Perform CRC operation in normal and snoop mode */
     err = crc_operation();
     /* Handle error */
     if (FSP_SUCCESS != err)
     {
         /* Turn on LED as sign of CRC operation failure */
         set_led(LED_ON);

         /* Print RTT message */
         printf ("\r\n** CRC operation failed **\r\n");
         /* Close all the opened modules before trap */
         cleanup();
         printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
     }
}

/*******************************************************************************************************************//**
 *  @brief      Performs CRC calculation in both normal and snoop modes.
 *              Toggles LED ON success; turns LED ON and displays failure messages via RTT on failure.
 *  @param[IN]  None
 *  @retval     FSP_SUCCESS    CRC calculation succeeded in both snoop and normal modes.
 *  @retval     err            Any error code other than FSP_SUCCESS indicating failure.
 **********************************************************************************************************************/
CODE_AREA
static fsp_err_t crc_operation (void)
{
    fsp_err_t err = FSP_SUCCESS;
    /* CRC inputs structure */
    crc_input_t input_data;
    uint32_t    normal_crc_value      = RESET_VALUE;           /* CRC value in normal mode */
    uint32_t    uart_time_out         = UINT32_MAX;            /* Timeout value to check RX, TX events */
    uint8_t     input_buffer[BUF_LEN] = {0x05,0x02,0x03,0x04}; /* Source data */
    uint8_t     dest_buffer[BUF_LEN]  = {RESET_VALUE};         /* Buffer to store UART read data */
    uint8_t     uart_data_len         = RESET_VALUE;           /* Data length for polynomial operation */

    /* Before beginning the operation turn OFF LED */
    set_led(LED_OFF);

    /* Clear callback event flag */
    b_uart_rxflag  = false;
    b_uart_txflag  = false;

    /* Update seed value and transfer uart_data_len as per the polynomial used */
    if(CRC_POLYNOMIAL_CRC_8 == g_crc_cfg.polynomial)
    {
        uart_data_len   = EIGHT_BIT_DATA_LEN;   /* Data length for 8 bit polynomial operation */
    }
    else if ((CRC_POLYNOMIAL_CRC_16 == g_crc_cfg.polynomial) || (CRC_POLYNOMIAL_CRC_CCITT == g_crc_cfg.polynomial))
    {
        uart_data_len   = SIXTEEN_BIT_DATA_LEN; /* Data length for 16 bit polynomial operation */
    }
    else
    {
        /* Display unsupported CRC Polynomial message in RTT */
        printf("\r\nThe provided example project demonstrates usage of only CRC-8, CRC-16, and CRC-CCITT."
                      "\r\nThe user may extend the project as needed to utilize other CRC Polynomial functionalities,"
                      "\r\nensuring that the CRC module supports the selected CRC Polynomial.\r\n");
        return FSP_ERR_UNSUPPORTED;
    }

    /* Update CRC input structure for normal mode */
    input_data.num_bytes      = NUM_BYTES;
    input_data.crc_seed       = SEED_VALUE;
    input_data.p_input_buffer = &input_buffer;

    /* Calculate CRC value for input data in normal mode */
    err = R_CRC_Calculate(&g_crc_ctrl, &input_data, &normal_crc_value);
    if (FSP_SUCCESS != err)
    {
        /* Display failure message in RTT */
        printf("R_CRC_Calculate API FAILED\r\n");
        return err;
    }

    /* Append calculated CRC value from normal mode to input buffer */
    if (CRC_POLYNOMIAL_CRC_8 == g_crc_cfg.polynomial)
    {
        /* Append 8 bit CRC value to input data */
        input_buffer[4] = (uint8_t) normal_crc_value;
    }
    else if ((CRC_POLYNOMIAL_CRC_16 == g_crc_cfg.polynomial) || (CRC_POLYNOMIAL_CRC_CCITT == g_crc_cfg.polynomial))
    {
        /* Extract the bytes from 16-bit CRC value and append to input buffer as per the selected byte order
         * in CRC configuration */
        if(CRC_BIT_ORDER_LMS_LSB == g_crc_cfg.bit_order)
        {
            input_buffer[4] = (uint8_t) (normal_crc_value & 0xFF);       /* Extract first byte */
            input_buffer[5] = (uint8_t) ((normal_crc_value >>8) & 0xFF); /* Extract second byte */
        }
        else
        {
            input_buffer[5] = (uint8_t) (normal_crc_value & 0xFF);       /* Extract first byte */
            input_buffer[4] = (uint8_t) ((normal_crc_value >>8) & 0xFF); /* Extract second byte */
        }
    }
    else
    {
        /* Do nothing */
    }



     uint32_t snoop_crc_value       = RESET_VALUE; /* CRC value in Snoop mode */

     /* Board support CRC snoop address */
     /* Enable snoop mode */
     err = R_CRC_SnoopEnable(&g_crc_ctrl, SEED_VALUE);
     if (FSP_SUCCESS != err)
     {
         /* Display failure message in RTT */
         printf("R_CRC_SnoopEnable API FAILED\r\n");
         return err;
     }

     /* Perform SCI UART loop-back transmission from TX to RX */
     /* Perform UART write operation */
     err =  R_SCI_B_UART_Write(&g_uart0_ctrl, input_buffer, uart_data_len);

     if (FSP_SUCCESS != err)
     {
         /* Display failure message in RTT */
         printf("R_SCI_B_UART_Write API FAILED\r\n");
         return err;
     }

     err =  R_SCI_B_UART_Read(&g_uart0_ctrl, dest_buffer, uart_data_len);

     if (FSP_SUCCESS != err)
     {
         /* Display failure message in RTT */
         printf("R_SCI_B_UART_Read API FAILED\r\n");
         return err;
     }
     /* Wait for TX and RX complete event */
     while((true != b_uart_txflag) || (true != b_uart_rxflag))
     {
         /* Start checking for timeout to avoid infinite loop */
         --uart_time_out;

         /* Check for time elapse */
         if (RESET_VALUE == uart_time_out)
         {
             /* We have reached to a scenario where UART TX and RX events not occurred */
             printf("** UART TX and RX events not received during UART write or read operation **\r\r");
             return FSP_ERR_TIMEOUT;
         }
      }

      /* Get CRC value in snoop mode for receive data */
      err = R_CRC_CalculatedValueGet(&g_crc_ctrl, &snoop_crc_value);
      if (FSP_SUCCESS != err)
      {
          /* Display failure message in RTT */
          printf("R_CRC_CalculatedValueGet API FAILED\r\n");
          return err;
      }

      /* Disable snoop operation */
      err = R_CRC_SnoopDisable(&g_crc_ctrl);
      if (FSP_SUCCESS != err)
      {
          /* Display failure message in RTT */
          printf("R_CRC_SnoopDisable API FAILED\r\n");
          return err;
      }

      /* Validate the CRC results */
      if (RESET_VALUE == snoop_crc_value)
      {
          printf("\r\nCRC Operation is successful\r\n");

          /* Compare UART write and read data buffer */
          if ( RESET_VALUE == memcmp(dest_buffer, input_buffer, uart_data_len))
          {
              printf("UART transmitted and received data successfully\r\n");
              /* Toggle LED as sign of successful operation */
              toggle_led();
           }
       }
       else
       {
           printf("UART loop-back transmission error\r\n");
           return FSP_ERR_ABORTED;
       }

    return err;
}
CODE_AREA
void uart0_callback(uart_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        switch(p_args->event)
        {
        case UART_EVENT_RX_COMPLETE:
            b_uart_rxflag = true;
            break;
        case UART_EVENT_TX_COMPLETE:
            b_uart_txflag = true;
            break;
        default:
            break;
        }
    }
}

/*******************************************************************************************************************//**
 * @brief       This function is called to close CRC module using its HAL level API.
 *              Errors are handled internally with appropriate messages; the application handles any further action.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void deinit_crc(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close opened CRC module */
    err = R_CRC_Close(&g_crc_ctrl);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        printf("** R_CRC_Close API FAILED **\r\n");
    }
}

/*******************************************************************************************************************//**
 * @brief       This function is called to close SCI UART/ SAU UART module using its HAL level API.
 *              Errors are handled internally with appropriate messages; the application handles any further action.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
CODE_AREA
void deinit_uart(void)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_SCI_B_UART_Close(&g_uart0_ctrl);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        printf("** R_SCI_B_UART_Close API FAILED **\r\n");
    }

}

/*******************************************************************************************************************//**
 * @brief     This function is used to close all opened module.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
CODE_AREA
void cleanup(void)
{
    /* Close all the opened modules */
    deinit_crc();
    deinit_uart();
}

/*******************************************************************************************************************//**
 * @brief     Toggles on-board LED, which is connected and supported by the BSP.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
CODE_AREA
void toggle_led(void)
{
    VALIDATE_IO_PORT_API(R_IOPORT_PinWrite(g_ioport.p_ctrl,(bsp_io_port_pin_t)g_bsp_leds.p_leds[RESET_VALUE], LED_ON));
    R_BSP_SoftwareDelay(TOGGLE_DELAY, BSP_DELAY_UNITS_MILLISECONDS);
    VALIDATE_IO_PORT_API(R_IOPORT_PinWrite(g_ioport.p_ctrl,(bsp_io_port_pin_t)g_bsp_leds.p_leds[RESET_VALUE], LED_OFF));
}

/*******************************************************************************************************************//**
 *  @brief       Turn the on-board LED ON or OFF.
 *  @param[IN]   b_value     LED_ON or LED_OFF
 *  @retval      None
 **********************************************************************************************************************/
CODE_AREA
static void set_led(bsp_io_level_t b_value)
{
    VALIDATE_IO_PORT_API(R_IOPORT_PinWrite(g_ioport.p_ctrl,(bsp_io_port_pin_t)g_bsp_leds.p_leds[RESET_VALUE], b_value));
}

