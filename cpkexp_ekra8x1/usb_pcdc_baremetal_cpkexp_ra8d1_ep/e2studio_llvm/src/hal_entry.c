/***********************************************************************************************************************
 * File Name    : hal_entry.c
 * Description  : Entry function.
 **********************************************************************************************************************/
/*
 * Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "hal_entry.h"
#include "common_init.h"

#include "perf_counter/perf_counter.h"

/* Function declaration */
void R_BSP_WarmStart(bsp_warm_start_event_t event);

/* Global variables */
extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t *g_apl_string_table[];
extern int g_curr_led_freq;

const usb_descriptor_t usb_descriptor =
{ g_apl_device, /* Pointer to the device descriptor */
  g_apl_configuration, /* Pointer to the configuration descriptor for Full-speed */
  g_apl_hs_configuration, /* Pointer to the configuration descriptor for Hi-speed */
  g_apl_qualifier_descriptor, /* Pointer to the qualifier descriptor */
  g_apl_string_table, /* Pointer to the string descriptor table */
  NUM_STRING_DESCRIPTOR };

usb_status_t usb_event;

/* Banner Info */
char p_welcome[200] =
{ "\r\n Welcome to USB PCDC example project for "
KIT_NAME_MACRO
"!"
"\r\n Press 1 for Kit Information | 2 for Next Steps\r\n"
" 3 for PCDC read speed test | 4 for PCDC write speed test.\r\n" };

/* Next steps */
char nextsteps[USB_EP_PACKET_SIZE] =
{ "\r\n 2. NEXT STEPS \r\n"
  "\r\nVisit the following URLs to learn about the kit "
  "and the RA family of MCUs, download tools "
  "and documentation, and get support:\r\n"
  "\r\n a) "
KIT_NAME_MACRO
" resources: \t"
KIT_LANDING_URL
"\r\n b) RA product information:  \t"
PRODUCT_INFO_URL
"\r\n c) RA product support forum: \t"
PRODUCT_SUPPORT_URL
"\r\n d) Renesas support: \t\t"
RENESAS_SUPPORT_URL
"\r\n\r\n Press 1 for Kit Information or 2 for Next Steps.\r\n"
" 3 for PCDC read speed test | 4 for PCDC write speed test.\r\n" };

char kitinfo[USB_EP_PACKET_SIZE] =
{ '\0' };

const char *p_mcu_temp = "\r\n d) MCU Die temperature (F/C):  ";
const char *p_led_freq = "\r\n c) Current blinking frequency (Hz): ";
const char *p_kit_menu_ret = "\r\n Press 1 for Kit Information or 2 for Next Steps.\r\n"
        " 3 for PCDC read speed test | 4 for PCDC write speed test.\r\n";

static bool b_usb_attach = false;

/* Private functions */
static fsp_err_t check_for_write_complete(void);
static fsp_err_t print_to_console(char *p_data);
static void process_kit_info(void);

fsp_err_t g_err = FSP_SUCCESS;

static fsp_err_t pcdc_read_1m_data(void);
static fsp_err_t pcdc_write_1m_data(void);

static void pcdc_handle_read_complete(unsigned char *g_buf)
{
    fsp_err_t err = FSP_SUCCESS;

    switch (g_buf[0])
    {
        case KIT_INFO:
            process_kit_info ();
        break;
        case NEXT_STEPS:
            err = print_to_console (nextsteps);
            if (FSP_SUCCESS != err)
                APP_ERR_TRAP(err);
        break;
        case CARRIAGE_RETURN:
            /* Print banner info to console */
            err = print_to_console (p_welcome);
            if (FSP_SUCCESS != err)
                APP_ERR_TRAP(err);
        break;
        case WRITE_1M_DATA:
            pcdc_read_1m_data ();
        break;
        case READ_1M_DATA:
            pcdc_write_1m_data ();
        break;
        default:
        break;
    }
}

/*******************************************************************************************************************//**
 * The RA Configuration tool generates main() and uses it to generate threads if an RTOS is used.  This function is
 * called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    fsp_err_t err = FSP_SUCCESS;
    usb_event_info_t event_info =
    { 0 };
    uint8_t g_buf[READ_BUF_SIZE] =
    { 0 };
    static usb_pcdc_linecoding_t g_line_coding;

    init_cycle_counter (false);

    /* Open USB instance */
    err = R_USB_Open (&g_basic0_ctrl, &g_basic0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_TRAP(err);
    }

    while (true)
    {
        /* Obtain USB related events */
        err = R_USB_EventGet (&event_info, &usb_event);
        if (FSP_SUCCESS != err)
            APP_ERR_TRAP(err);

        /* USB event received by R_USB_EventGet */
        switch (usb_event)
        {
            case USB_STATUS_CONFIGURED:
                err = R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);
                if (FSP_SUCCESS != err)
                    APP_ERR_TRAP(err);
            break;
            case USB_STATUS_READ_COMPLETE:
                pcdc_handle_read_complete (g_buf);
                if (b_usb_attach)
                {
                    err = R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);
                    if (FSP_SUCCESS != err)
                        APP_ERR_TRAP(err);
                }
            break;
            case USB_STATUS_REQUEST: /* Receive Class Request */
                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (event_info.setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlDataGet (&g_basic0_ctrl, (uint8_t*) &g_line_coding, LINE_CODING_LENGTH);
                    /* Handle error */
                    if (FSP_SUCCESS != err)
                        APP_ERR_TRAP(err);
                }
                else if (USB_PCDC_GET_LINE_CODING == (event_info.setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlDataSet (&g_basic0_ctrl, (uint8_t*) &g_line_coding, LINE_CODING_LENGTH);
                    /* Handle error */
                    if (FSP_SUCCESS != err)
                        APP_ERR_TRAP(err);
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (event_info.setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet (&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    /* Handle error */
                    if (FSP_SUCCESS != err)
                        APP_ERR_TRAP(err);
                }
                else
                {
                    /* none */
                }
            break;
            case USB_STATUS_DETACH:
            case USB_STATUS_SUSPEND:
                b_usb_attach = false;
                memset (g_buf, 0, sizeof(g_buf));
            break;
            case USB_STATUS_RESUME:
                b_usb_attach = true;
            break;
            default:
            break;
        }
    }
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */
        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, &g_bsp_pin_cfg);
    }
}

/*****************************************************************************************************************
 *  @brief      Prints the message to console
 *  @param[in]  p_msg contains address of buffer to be printed
 *  @retval     FSP_SUCCESS     Upon success
 *  @retval     any other error code apart from FSP_SUCCESS, Write is unsuccessful
 ****************************************************************************************************************/
static fsp_err_t print_to_console(char *p_data)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t len = ((uint32_t) strlen (p_data));

    err = R_USB_Write (&g_basic0_ctrl, (uint8_t*) p_data, len, USB_CLASS_PCDC);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = check_for_write_complete ();
    if (FSP_SUCCESS != err)
    {
        /* Did not get the event hence returning error */
        return FSP_ERR_USB_FAILED;
    }
    return err;
}

/*****************************************************************************************************************
 *  @brief      Check for write completion
 *  @param[in]  None
 *  @retval     FSP_SUCCESS     Upon success
 *  @retval     any other error code apart from FSP_SUCCESS
 ****************************************************************************************************************/
static fsp_err_t check_for_write_complete(void)
{
    usb_status_t usb_write_event = USB_STATUS_NONE;
    int32_t timeout_count = UINT16_MAX;
    fsp_err_t err = FSP_SUCCESS;
    usb_event_info_t event_info =
    { 0 };

    do
    {
        err = R_USB_EventGet (&event_info, &usb_write_event);
        if (FSP_SUCCESS != err)
        {
            return err;
        }

        --timeout_count;

        if (0 > timeout_count)
        {
            timeout_count = 0;
            err = (fsp_err_t) USB_STATUS_NONE;
            break;
        }
    }
    while (USB_STATUS_WRITE_COMPLETE != usb_write_event);

    return err;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup hal_entry)
 **********************************************************************************************************************/

static void process_kit_info(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* clear kit info buffer before updating data */
    memset (kitinfo, '\0', 511);

    /* update  predefined text in the buffer */
    memcpy (kitinfo, (char*) KIT_INFO_PRIMARY_TEXT, strlen ((char*) KIT_INFO_PRIMARY_TEXT));

    /* Print kit menu to console */
    err = print_to_console (kitinfo);
    /* Handle error*/
    if (FSP_SUCCESS != err)
    {
        APP_ERR_TRAP(err);
    }
}

#define TEST_BUFSZ  512
#define PACKET_CNT  2048

#ifndef APP_PRINT
#define APP_PRINT(...)
#endif

static __attribute__((aligned(64)))   uint8_t test_buf[TEST_BUFSZ];

/*
 * The HCDC device will send 2048 packets to pcdc. then
 * PCDC device report the speed.
 */
static fsp_err_t pcdc_write_1m_data(void)
{
    fsp_err_t err;
    uint32_t ms;
    int i;

    memset (test_buf, 't', sizeof(test_buf));
    test_buf[TEST_BUFSZ - 2] = '\r';
    test_buf[TEST_BUFSZ - 1] = '\n';
    APP_PRINT("start PCDC write speed test ...\n");

    ms = (uint32_t) get_system_ms ();
    for (i = 0; i < PACKET_CNT; i++)
    {
        err = R_USB_Write (&g_basic0_ctrl, test_buf, sizeof(test_buf), USB_CLASS_PCDC);
        if (FSP_SUCCESS != err)
        {
            APP_PRINT("receive data from hcdc device fail.\n");
            return err;
        }

        err = check_for_write_complete ();
        if (FSP_SUCCESS != err)
        {
            APP_PRINT("error when waitting USB write complete.\n");
            return FSP_ERR_USB_FAILED;
        }
    }
    ms = (uint32_t) get_system_ms () - ms;

    sprintf ((char*) test_buf, "\r\nPCDC device write %d bytes use %d ms, "
             "PCDC write speed is %u KB/s\r\n",
             i * TEST_BUFSZ, ms, (((uint32_t) i * TEST_BUFSZ) >> 10) * 1000 / ms);

    return print_to_console ((char*) test_buf);
}

static fsp_err_t pcdc_read_1m_data(void)
{
    uint32_t ms, size = 0, asize = 0;
    usb_event_info_t event_info;
    fsp_err_t err;
    int first = 1;

    while (size < (TEST_BUFSZ * PACKET_CNT))
    {
        err = R_USB_Read (&g_basic0_ctrl, test_buf, TEST_BUFSZ, USB_CLASS_PCDC);
        if (err != FSP_SUCCESS)
        {
            sprintf ((char*) test_buf, "%s USB_Read fail. %d\n", __func__, err);
            goto out;
        }

        do
        {
            err = R_USB_EventGet (&event_info, &usb_event);
            if (err != FSP_SUCCESS)
            {
                sprintf ((char*) test_buf, "%s get usb event fail.\n", __func__);
                goto out;
            }
        }
        while (usb_event != USB_STATUS_READ_COMPLETE);

        if (first)
        {
            ms = (uint32_t) get_system_ms ();
            first = 0;
        }
        else
        {
            asize += event_info.data_size;
        }

        size += event_info.data_size;
    }

    ms = (uint32_t) get_system_ms () - ms;

    R_BSP_SoftwareDelay (3, BSP_DELAY_UNITS_SECONDS);

    sprintf ((char*) test_buf, "\r\nPCDC receive %u bytes use %u ms, "
             "PCDC read speed is %u KB/s\r\n",
             asize, ms, (asize >> 10) * 1000 / ms);

    out: return print_to_console ((char*) test_buf);
}
