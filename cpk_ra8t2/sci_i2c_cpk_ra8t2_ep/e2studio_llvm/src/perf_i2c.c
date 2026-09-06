/*
 * perf_i2c.c
 *
 *  Created on: Jan 22, 2026
 *      Author: Ran Qingling
 */
#include"hal_data.h"
#include"perf_i2c.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

/* Event flag to identify the Master event */
DATA_AREA_DATA static volatile master_transfer_mode_t g_master_RW = MASTER_WRITE;
/* Slave transmit buffer */
DATA_AREA_BSS static uint8_t g_slave_tx_buf[BUF_LEN];
/* Slave receive buffer */
DATA_AREA_BSS static uint8_t g_slave_rx_buf[BUF_LEN];
/* Private global variables */
/* Capture callback event for IIC Slave and SCI_I2C Master modules */
DATA_AREA_DATA static volatile i2c_master_event_t g_master_event = (i2c_master_event_t)RESET_VALUE;
DATA_AREA_DATA static volatile i2c_slave_event_t g_slave_event  = (i2c_slave_event_t)RESET_VALUE;
/* Capture return value from Slave read and write API */
DATA_AREA_DATA static volatile fsp_err_t g_slave_api_ret_err = FSP_SUCCESS;

/* Private functions */
static fsp_err_t sci_i2c_master_read(void);
static fsp_err_t sci_i2c_master_write(void);

fsp_err_t init_i2c_driver(void)
{
    fsp_err_t err = FSP_SUCCESS;
    printf("\r\n==SCI-I2C test project==\r\n");
    printf("\r\nSCI0 used as i2c master, while I2C2 used as i2c slave\r\n");
    printf("according to readme file,connect SCI0 to I2C2\r\n");
    printf("rate: fast mode\r\n");
    printf("I2C initialize......\r\n");
    /* Open SCI_I2C Master channel */
    err = R_SCI_B_I2C_Open(&g_sci_i2c_master_ctrl, &g_sci_i2c_master_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("I2C Master_Open API FAILED\r\n");
        return err;
    }
    /* Open I2C Slave channel */
    err = R_IIC_SLAVE_Open(&g_i2c_slave_ctrl, &g_i2c_slave_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("R_IIC SLAVE_Open API FAILED\r\n");
        /* I2C Slave open unsuccessful, closing the SCI_I2C Master module */
        if (FSP_SUCCESS != R_SCI_B_I2C_Close(&g_sci_i2c_master_ctrl))
        {
            printf("R_SCI_slave_Close API FAILED\r\n");
        }
    }
    return err;
}
CODE_AREA
fsp_err_t process_master_WriteRead(void)
{
    fsp_err_t      error = FSP_SUCCESS;

    switch(g_master_RW)
    {
        case MASTER_WRITE:
        {
            /* Change Master event so that relevant operation can be performed following the current one */
            g_master_RW = MASTER_READ;
            /* Before beginning the operation turn OFF LED */
            R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_LOW);/* User led off */
            /* Master starts writing the data to Slave */
            error = sci_i2c_master_write();
            if (FSP_SUCCESS != error)
            {
                printf("** "SCI_TYPE"_I2C Master Write operation failed ! **\r\n");
            }
            else
            {
                /* User led on */
                R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
                printf("** "SCI_TYPE"_I2C Master Write operation is successful **\r\n");
            }
            break;
        }
        case MASTER_READ:
        {
            /* Change Master event so that relevant operation can be performed following the current one */
            g_master_RW = MASTER_WRITE;
            /* Before beginning the operation turn OFF LED */
            R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_LOW);/* User led off */
            /* Master starts receiving data transmitted by Slave */
            error = sci_i2c_master_read();
            if (FSP_SUCCESS != error)
            {
                printf("** "SCI_TYPE"_I2C Master Read operation failed ! **\r\n");
            }
            else
            {
                /* User led on */
                R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
                printf("** "SCI_TYPE"_I2C Master Read operation is successful **\r\n");
            }

            break;
        }
        default:
            break;
    }

    return error;
}

/*******************************************************************************************************************//**
 *  @brief       Performs Master read operation
 *  @param[IN]   None
 *  @retval      FSP_SUCCESS               Master successfully read all data written by Slave device.
 *  @retval      FSP_ERR_TRANSFER_ABORTED  Callback event failure.
 *  @retval      FSP_ERR_ABORTED           Data mismatch occurred.
 *  @retval      FSP_ERR_TIMEOUT           In case of no callback event occurrence.
 *  @retval      err                       API returned error if any.
 **********************************************************************************************************************/
CODE_AREA
static fsp_err_t sci_i2c_master_read(void)
{
    fsp_err_t read_err           = FSP_SUCCESS;
    uint16_t read_time_out       = UINT16_MAX;
    uint8_t read_buffer[BUF_LEN] = {RESET_VALUE};

    /* Reset and update Slave transmit buffer with data received by Slave */
    memset(g_slave_tx_buf, RESET_VALUE, BUF_LEN);
    memcpy(g_slave_tx_buf, g_slave_rx_buf, BUF_LEN);

    /* Resetting callback event */
    g_master_event = (i2c_master_event_t)RESET_VALUE;
    g_slave_event  = (i2c_slave_event_t)RESET_VALUE;

    /* Start master read. Master has to initiate the transfer. */
    read_err = R_SCI_B_I2C_Read(&g_sci_i2c_master_ctrl, read_buffer, BUF_LEN, false);
    if (FSP_SUCCESS != read_err)
    {
        printf("\r\n** R_"SCI_TYPE"_I2C_Read API FAILED **\r\n");
        return read_err;
    }
    /* Wait until Slave write and Master read process gets completed */
    while ((I2C_MASTER_EVENT_RX_COMPLETE != g_master_event) || (I2C_SLAVE_EVENT_TX_COMPLETE != g_slave_event))
    {
        /* Check for aborted event */
        if ((I2C_SLAVE_EVENT_ABORTED == g_slave_event) || (I2C_MASTER_EVENT_ABORTED == g_master_event))
        {
            printf("** EVENT_ABORTED received during Master read operation **\r\n");
            /* I2C transaction failure */
            return FSP_ERR_TRANSFER_ABORTED;
        }
        /* Handle error for Slave write API return value g_slave_api_ret_err gets
         * updated only in case of error return from slave write API */
        else if (FSP_SUCCESS != g_slave_api_ret_err)
        {
            read_err = g_slave_api_ret_err;
            /* Reset this with success again for further usage to capture in case of API failure */
            g_slave_api_ret_err = FSP_SUCCESS;

            return read_err;
        }

        else
        {
            /* Start checking for time out to avoid infinite loop */
            --read_time_out;

            /* Check for time elapse */
            if (RESET_VALUE == read_time_out)
            {
                /* We have reached to a scenario where i2c event not occurred */
                printf ("** No event received during Master read and Slave write operation **\r\n");

                /* No event received */
                return FSP_ERR_TIMEOUT;
            }
        }
    }

    /* Compare data received by Master device with Slave write buffer */
    if ( BUFF_EQUAL == memcmp(read_buffer, g_slave_tx_buf, BUF_LEN) )
    {
        read_err = FSP_SUCCESS;
    }
    else
    {
        read_err = FSP_ERR_ABORTED;
    }

    return read_err;
}

/*******************************************************************************************************************//**
 *  @brief      Performs Master write operation.
 *  @param[IN]  None
 *  @retval     FSP_SUCCESS               Master writes and slave receives data successfully.
 *  @retval     FSP_ERR_TRANSFER_ABORTED  Callback event failure.
 *  @retval     FSP_ERR_ABORTED           Data mismatch occurred.
 *  @retval     FSP_ERR_TIMEOUT           In case of no callback event occurrence.
 *  @retval     read_err                  API returned error if any.
 **********************************************************************************************************************/
CODE_AREA
static fsp_err_t sci_i2c_master_write(void)
{
    fsp_err_t write_err           = FSP_SUCCESS;
    uint16_t write_time_out       = 1000;

    /* Update Master buffer and slave is recipient so clear slave RX buffer */
    uint8_t write_buffer[BUF_LEN] = {0x10, 0x20, 0x30, 0x40, 0x50};
    memset(g_slave_rx_buf, RESET_VALUE, BUF_LEN);
    /* Resetting callback event */
    g_master_event = (i2c_master_event_t)RESET_VALUE;
    g_slave_event  = (i2c_slave_event_t)RESET_VALUE;

    /* Start Master Write operation */

    write_err = R_SCI_B_I2C_Write(&g_sci_i2c_master_ctrl, write_buffer, BUF_LEN, false);

    /* Handle error */
    if (FSP_SUCCESS != write_err)
    {
        printf("\r\n** R_"SCI_TYPE"_I2C_Write API FAILED **\r\n");
        return write_err;
    }

    /* Wait until Master write and slave read process gets completed */
    while ((I2C_MASTER_EVENT_TX_COMPLETE != g_master_event) || (I2C_SLAVE_EVENT_RX_COMPLETE != g_slave_event))
    {
        /* Check for aborted event */
        if ((I2C_SLAVE_EVENT_ABORTED == g_slave_event) || (I2C_MASTER_EVENT_ABORTED == g_master_event))
        {
            printf ("** Error EVENT_ABORTED received during Master write operation **\r\n");

            /* I2C transaction failure */
            return FSP_ERR_TRANSFER_ABORTED;

        }

        /* Handle error for Slave read API return value g_slave_api_ret_err gets
         * updated only in case of error return from Slave read API */
        else if (FSP_SUCCESS != g_slave_api_ret_err)
        {
            write_err = g_slave_api_ret_err;

            /* Reset this with success again for further usage to capture in case of API failure */
            g_slave_api_ret_err = FSP_SUCCESS;

            return write_err;
        }

        else
        {
            /* Start checking for time out to avoid infinite loop */
            R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
            --write_time_out;
            /* Check for time elapse */
            if (RESET_VALUE == write_time_out)
            {
                /* We have reached to a scenario where i2c event not occurred */
                printf ("** No event received during Master Write and Slave read operation **\r\n");

                /* No event received */
                return FSP_ERR_TIMEOUT;
            }
        }
    }

    /* Compare Master written data with Slave read buffer */
    if ( BUFF_EQUAL == memcmp(g_slave_rx_buf, write_buffer, BUF_LEN) )
    {
        write_err = FSP_SUCCESS;
    }
    else
    {
        write_err = FSP_ERR_ABORTED;
    }

    return write_err;
}

/*******************************************************************************************************************//**
 *  @brief        The user defined Master callback function
 *  @param[IN]    p_args
 *  @retval       None
 **********************************************************************************************************************/
CODE_AREA
void sci_i2c_master_callback(i2c_master_callback_args_t * p_args)
{
    if (NULL != p_args)
    {
        g_master_event = p_args->event;
    }
}

/*******************************************************************************************************************//**
 *  @brief        The user defined Slave callback function.
 *  @param[IN]    p_args
 *  @retval       None
 **********************************************************************************************************************/
CODE_AREA
void i2c_slave_callback(i2c_slave_callback_args_t * p_args)
{
    fsp_err_t err = FSP_SUCCESS;
    /* Log the event to global variable for any other Slave event */
    g_slave_event = p_args->event;
    if (NULL != p_args)
    {
        switch(p_args->event)
        {
            case I2C_SLAVE_EVENT_TX_COMPLETE:
                break;
            case I2C_SLAVE_EVENT_RX_COMPLETE:
                break;
            case I2C_SLAVE_EVENT_RX_REQUEST:
            {
                /* Perform Slave read operation */
                err = R_IIC_SLAVE_Read(&g_i2c_slave_ctrl, g_slave_rx_buf, BUF_LEN);
                if(FSP_SUCCESS != err)
                {
                    /* Update return error here */
                    g_slave_api_ret_err = err;
                }

                break;
            }
            case I2C_SLAVE_EVENT_TX_REQUEST:
            {
                /* Perform Slave write operation */
                err = R_IIC_SLAVE_Write(&g_i2c_slave_ctrl, g_slave_tx_buf, BUF_LEN);
                if(FSP_SUCCESS != err)
                {
                    /* Update return error here */
                    g_slave_api_ret_err = err;
                }

                break;
            }
            default:

                break;
        }
    }
}

/*******************************************************************************************************************//**
 * @brief     Closes the SCI_I2C Master and IIC Slave modules using HAL-level APIs.
 *            Handles errors internally and prints appropriate RTT messages.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
void deinit_i2c_driver(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close opened SCI_I2C Master module */
    err = R_SCI_B_I2C_Close(&g_sci_i2c_master_ctrl);

    if (FSP_SUCCESS != err)
    {
        printf("** R_"SCI_TYPE"_I2C_Close API FAILED **\r\n");
    }
    /* Close opened IIC Slave module */

    err = R_IIC_SLAVE_Close(&g_i2c_slave_ctrl);

    if (FSP_SUCCESS != err)
    {
        printf("** R_IIC"SLAVE_TYPE"_Close API FAILED **\r\n");
    }
}
