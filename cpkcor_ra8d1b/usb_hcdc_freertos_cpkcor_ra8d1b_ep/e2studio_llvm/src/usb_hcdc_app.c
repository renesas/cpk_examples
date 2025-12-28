/***********************************************************************************************************************
 * File Name    : usb_hcdc_app.c
 * Description  : Contains data structures and functions used in usb_hcdc_app.c.
 **********************************************************************************************************************/
 
 /*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <usb_hcdc_app.h>
#include <r_usb_cstd_rtos.h>
#include <common_utils.h>
#include "usb_hcdc_thread.h"

#include "perf_counter/perf_counter.h"

/*******************************************************************************************************************//**
 * @addtogroup usb_hcdc_ep
 * @{
 **********************************************************************************************************************/
/** Private variables **/
static usb_hcdc_linecoding_t g_serial_state;
static usb_hcdc_linecoding_t g_com_parm;
static uint8_t  g_snd_buf[8] BSP_ALIGN_VARIABLE(4) = {RESET_VALUE};         //Send buffer
static uint8_t  g_rcv_buf[512] BSP_ALIGN_VARIABLE(4) = {RESET_VALUE};       //Receive buffer
static uint8_t  g_usb_dummy = RESET_VALUE;                         //dummy variable to send
static volatile bool g_err_flag = false;                    //flag bit

/** Private functions**/
static void set_line_coding (usb_instance_ctrl_t * p_ctrl, uint8_t device_address);
static void set_control_line_state (usb_instance_ctrl_t * p_ctrl, uint8_t device_address);
static void get_line_coding (usb_instance_ctrl_t * p_ctrl, uint8_t device_address);
static void usb_data_process(usb_event_info_t *event_info);
static void handle_error(fsp_err_t err, char *err_str);

/******************************************************************************************************//**
* @brief This function process the data from the host to device and prints the received data on RTT terminal
* @param[IN]   None
* @retval      None
*******************************************************************************************************/
void usb_hcdc_task(void)
{
    fsp_err_t err = FSP_SUCCESS;
    static usb_event_info_t *event_info = NULL;
    BaseType_t err_queue = pdFALSE;
    memset(&g_serial_state, RESET_VALUE, sizeof(g_serial_state));
    memset(&g_com_parm, RESET_VALUE, sizeof(g_com_parm));
    g_snd_buf[ZERO_INDEX]= KIT_INFO;

    fsp_pack_version_t version = {RESET_VALUE};

    /* version get API for FLEX pack information */
    R_FSP_VersionGet(&version);


    /* Example Project information printed on the Console */
    APP_PRINT(BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT(EP_INFO);


    err = R_USB_Open (&g_basic_ctrl, &g_basic_cfg);
    /* Error Handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT ("\r\nError in initializing USB Host CDC\r\n");
        APP_ERR_TRAP (err);
    }


    while (true)
    {
        /* Handle error if queue send fails*/
        if (true == g_err_flag)
        {
            handle_error(g_err_flag, "Error in sending usb event through queue");
        }

        /* Receive message from queue and analyzing the received message*/
         err_queue= xQueueReceive(g_usb_queue, &event_info,(portMAX_DELAY));
         /* Handle error */
         if(pdTRUE != err_queue)
         {
             handle_error ((fsp_err_t)err_queue, "Error in receiving USB event message through queue");
         }

        switch (event_info->event)
        {
            case USB_STATUS_CONFIGURED:
            {
                /* CDC Class request "SetLineCoding" */
                set_line_coding (&g_basic_ctrl, event_info->device_address);
            }
            break;

            case USB_STATUS_READ_COMPLETE:
            {
                /* CDC class communication data process */
                usb_data_process(event_info);
            }
            break;

            case USB_STATUS_WRITE_COMPLETE:
            {
                /* Report receive start */
                err = R_USB_Read (&g_basic_ctrl, g_rcv_buf, CDC_READ_DATA_LEN,
                                  event_info->device_address);
                /* Handle Error */
                if (FSP_SUCCESS != err)
                {
                    handle_error (err,"**R_USB_Read API FAILED**");
                }
            }
            break;

            case USB_STATUS_REQUEST_COMPLETE:
            {
                /* Check Complete request "SetLineCoding" */
                if (USB_CDC_SET_LINE_CODING == (event_info->setup.request_type & USB_BREQUEST))
                {
                    /* Class notification "SerialState" receive start */
                    set_control_line_state (&g_basic_ctrl, event_info->device_address);
                }
                /* Check Complete request "SetControlLineState" */
                else if (USB_CDC_SET_CONTROL_LINE_STATE == (event_info->setup.request_type & USB_BREQUEST))
                {
                    /* CDC Class request "SetLineCoding" */
                    get_line_coding (&g_basic_ctrl, event_info->device_address);
                }
                else if (USB_CDC_GET_LINE_CODING == (event_info->setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_Write (&g_basic_ctrl, g_snd_buf, CDC_WRITE_DATA_LEN,
                                 event_info->device_address);
                    if (FSP_SUCCESS != err)
                    {
                        handle_error (err, "**R_USB_Write API FAILED**");
                    }
                }
                else
                {
                    /* Not support request */
                }
            }
            break;

            default:
            {
                /* No operation to do*/
            }
            break;
        }
    }
}
 /* End of function main_task() */

/*******************************************************************************************************************//**
 * @brief In this function initializes to set control line state information for host control transfer.
 * @param[IN]   p_ctrl         Pointer to control structure
 * @param[IN]   device_address         USB device address
 * @retval      None.
 ***********************************************************************************************************************/
static void set_control_line_state (usb_instance_ctrl_t * p_ctrl, uint8_t device_address)
{
    usb_setup_t setup;
    fsp_err_t err = FSP_SUCCESS;
    setup.request_type      = SET_CONTROL_LINE_STATE;   /* bRequestCode:SET_CONTROL_LINE_STATE, bmRequestType */
    setup.request_value     = VALUE_ZERO;                   /* wValue:Zero */
    setup.request_index     = VALUE_ZERO;                   /* wIndex:Interface */
    setup.request_length    = VALUE_ZERO;                   /* wLength:Zero */

    err = R_USB_HostControlTransfer(p_ctrl, &setup, &g_usb_dummy, device_address);
    if (FSP_SUCCESS != err)
    {
        handle_error (err, "**R_USB_HOSTControlTransfer API FAILED**");
    }

} /* End of function cdc_set_control_line_state */
/*******************************************************************************************************************//**
 * @brief In this function initializes to set line coding information for host control transfer.
 * @param[IN]   p_ctrl         Pointer to control structure
 * @param[IN]   device_address  USB device address
 * @retval      None.
 ***********************************************************************************************************************/
 static void set_line_coding (usb_instance_ctrl_t * p_ctrl, uint8_t device_address)
{
     usb_setup_t setup;
     fsp_err_t err = FSP_SUCCESS;
     g_com_parm.dwdte_rate   = (uint32_t)USB_HCDC_SPEED_9600;
     g_com_parm.bdata_bits   = USB_HCDC_DATA_BIT_8;
     g_com_parm.bchar_format = USB_HCDC_STOP_BIT_1;
     g_com_parm.bparity_type = USB_HCDC_PARITY_BIT_NONE;


     setup.request_type      = SET_LINE_CODING;    /* bRequestCode:SET_LINE_CODING, bmRequestType */
     setup.request_value     = VALUE_ZERO;             /* wValue:Zero */
     setup.request_index     = VALUE_ZERO;             /* wIndex:Interface */
     setup.request_length    = LINE_CODING_LENGTH; /* Data:Line Coding Structure */

     /* Request Control transfer */
     err = R_USB_HostControlTransfer(p_ctrl, &setup, (uint8_t *)&g_com_parm, device_address);
     if (FSP_SUCCESS != err)
     {
         handle_error (err, "**R_USB_HostControlTransfer API FAILED**");
     }
} /* End of function cdc_set_line_coding */

 /*******************************************************************************************************************//**
  * @brief In this function initializes to get line coding information for host control transfer.
  * @param[IN]   p_ctrl                pointer to control structure
  * @param[IN]   device_address         USB device address
  * @retval      None.
  ***********************************************************************************************************************/
static void get_line_coding (usb_instance_ctrl_t * p_ctrl, uint8_t device_address)
{
    usb_setup_t setup;
    fsp_err_t err = FSP_SUCCESS;
    setup.request_type      = GET_LINE_CODING;      /* bRequestCode:GET_LINE_CODING, bmRequestType */
    setup.request_value     = VALUE_ZERO;               /* wValue:Zero */
    setup.request_index     = VALUE_ZERO;               /* wIndex:Interface */
    setup.request_length    = LINE_CODING_LENGTH;   /* Data:Line Coding Structure */

    /* Request Control transfer */
    err = R_USB_HostControlTransfer(p_ctrl, &setup, (uint8_t *)&g_com_parm, device_address);
    if (FSP_SUCCESS != err)
    {
        handle_error (err, "**R_USB_HostControlTransfer API FAILED**");
    }

} /* End of function cdc_get_line_coding */

/*******************************************************************************************************************//**
 * @brief  This function is called to do closing of usb module using its HAL level API and handles the error trap.
 *  Handle the Error internally with Proper Message. Application handles the rest.
 * @param[IN] err       Return values of APIs
 * @param[IN] err_str   Print message from the failed API call
 * @retval    None
 **********************************************************************************************************************/
static void handle_error(fsp_err_t err, char *err_str)
{
    fsp_err_t error = FSP_SUCCESS;

    /* close opened USB module */
    error = R_USB_Close(&g_basic_ctrl);
    /* Handle error */
    if(FSP_SUCCESS != error)
    {
        APP_ERR_PRINT ("**\r\n R_USB_Close API FAILED **\r\n");
    }
    APP_ERR_PRINT(err_str);
    APP_ERR_TRAP(err);
} /* End of function handle_error() */

/*******************************************************************************************************************//**
 * @brief     This function is callback for FreeRTOS+HCDC and triggers when USB event occurs from the device.
 * @param[IN]   p_event_info    Triggered event
 * @param[IN]   cur_task        current task handle
 * @param[IN]   usb_state       State of USB
 * @retval      None.
 ***********************************************************************************************************************/
void usb_rtos_callback (usb_event_info_t *p_event_info, usb_hdl_t cur_task, usb_onoff_t usb_state)
{
    FSP_PARAMETER_NOT_USED (cur_task);
    FSP_PARAMETER_NOT_USED (usb_state);

    /* Send event received to queue */
    if (pdTRUE != (xQueueSend(g_usb_queue, (const void *)&p_event_info, (TickType_t)(NO_WAIT_TIME))))
    {
        g_err_flag = true;
    }
} /* End of function usb_rtos_callback */

#define TEST_BUFSZ	512
#define PACKET_CNT	2048
#define TEST_SIZE	(TEST_BUFSZ * PACKET_CNT)

static fsp_err_t hcdc_write_1m_data(void)
{
	usb_event_info_t *ev = NULL;
	BaseType_t err_queue = pdFALSE;
	uint32_t size = TEST_SIZE;
	fsp_err_t err;

	memset(g_rcv_buf, 't', sizeof(g_rcv_buf));
	APP_PRINT("start CDC write speed test ...\n");

	/*
	 * Send 1M data and left one write complete msg for
	 * main loop.
	 */
	do {
		err_queue = xQueueReceive(g_usb_queue, &ev, portMAX_DELAY);
		if (pdTRUE != err_queue) {
			APP_PRINT("get queue msg from USB failed.\n");
			break;
		}

		switch (ev->event) {
		case USB_STATUS_WRITE_COMPLETE:
			break;
		default:
			APP_PRINT("receive unknown event in %s\n", __func__);
			break;
		}

		err = R_USB_Write(&g_basic_ctrl, g_rcv_buf,
				sizeof(g_rcv_buf), ev->device_address);
		if (FSP_SUCCESS != err) {
			APP_PRINT("write data to cdc device fail. %d\n", err);
			return err;
		}

		size -= sizeof(g_rcv_buf);

	} while (size > 0);

	return 0;
}

static fsp_err_t hcdc_read_1m_data(void)
{
	usb_event_info_t *ev = NULL;
	BaseType_t err_queue = pdFALSE;
	uint32_t size = 0;
	fsp_err_t err;

	APP_PRINT("start CDC read speed test ...\n");

	while (size < TEST_SIZE) {
		err_queue = xQueueReceive(g_usb_queue, &ev, portMAX_DELAY);
		if (pdTRUE != err_queue) {
			APP_PRINT("get queue msg from USB failed.\n");
			break;
		}

		switch (ev->event) {
		case USB_STATUS_WRITE_COMPLETE:
			break;
		case USB_STATUS_READ_COMPLETE:
			size += ev->data_size;
			break;
		default:
			APP_PRINT("receive unknown event in %s\n", __func__);
			break;
		}

		err = R_USB_Read(&g_basic_ctrl, g_rcv_buf,
				sizeof(g_rcv_buf), ev->device_address);
		if (err != FSP_SUCCESS) {
			APP_PRINT("case USB_Read fail.\n");
			return err;
		}
	}

	return 0;
}

/*******************************************************************************************************************//**
  * @brief This function is to do data process with peripheral device
  * @param[IN]   event_info             data process in HCDC event type
  * @retval      None.
  ***********************************************************************************************************************/
void usb_data_process(usb_event_info_t *event_info)
{
    fsp_err_t err = FSP_SUCCESS;
    if (USB_CLASS_HCDC == event_info->type) {
        if (RESET_VALUE < event_info->data_size) {
            /* Send the received data to USB Host */
            switch(g_snd_buf[ZERO_INDEX]) {
		case KIT_INFO:
                    g_snd_buf[ZERO_INDEX]= NEXT_STEPS;
		    break;
                case NEXT_STEPS:
                    g_snd_buf[ZERO_INDEX]= CARRIAGE_RETURN;
		    break;
                case CARRIAGE_RETURN:
                    g_snd_buf[ZERO_INDEX] = WRITE_1M_DATA;
		    break;
		case WRITE_1M_DATA:
		    g_snd_buf[ZERO_INDEX] = READ_1M_DATA;
		    break;
		case READ_1M_DATA:
		    g_snd_buf[ZERO_INDEX] = KIT_INFO;
		    break;
                default:
		    break;
            }

	    g_rcv_buf[event_info->data_size] = 0;
            APP_PRINT("\r\n Received data :%s", g_rcv_buf);

	    /* Write next command to the PCDC device */
            err = R_USB_Write (&g_basic_ctrl, g_snd_buf, CDC_WRITE_DATA_LEN,
                         event_info->device_address);
            if (FSP_SUCCESS != err)
		    handle_error (err, "**R_USB_Write API FAILED**");

	    if (g_snd_buf[ZERO_INDEX] == WRITE_1M_DATA)
		    hcdc_write_1m_data();
	    else if (g_snd_buf[ZERO_INDEX] == READ_1M_DATA)
		    hcdc_read_1m_data();
        } else {
            /* Send the data reception request when the zero-length packet is received. */
            err = R_USB_Read (&g_basic_ctrl,g_rcv_buf, event_info->data_size,
                        event_info->device_address);
            if (FSP_SUCCESS != err)
                handle_error (err, "**R_USB_Read API FAILED**");
        }
    } else {
        /* Class notification "SerialState" receive start */
        err = R_USB_Read(&g_basic_ctrl, (uint8_t *) &g_serial_state,
        USB_HCDC_SERIAL_STATE_MSG_LEN,event_info->device_address);
        /* Error Handle */
        if (FSP_SUCCESS != err)
            handle_error (err, "**R_USB_Read API FAILED**");
    }
} /* End of function usb_data_process */


/*******************************************************************************************************************//**
 * @} (end addtogroup usb_hcdc_ep)
 **********************************************************************************************************************/
