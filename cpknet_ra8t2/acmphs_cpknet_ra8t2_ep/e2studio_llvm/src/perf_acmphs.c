/*
 * perf_i,cu.c
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */
#include <perf_acmphs.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>

static volatile bool b_comparator_state_flag = 0;
volatile bool start_comparator = 0U;

comparator_status_t acmphs_status;
volatile uint16_t dac_value = 2500;
void acmphs_user_callback(comparator_callback_args_t *p_args)
{
    /* Check for the channel 0 of comparator */
    if(0 == p_args->channel)
    {
        /* Toggle the flag */
        b_comparator_state_flag = true;
        printf("Enter acmphs interrupt.\r\n");
    }
}

static void deinit_dac(void)
{
    fsp_err_t err = FSP_SUCCESS;
    /* Close DAC */
    err = R_DAC_B_Close(&g_dac_b1_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_DAC_B_Close API FAILED **\r\n");
    }
}
static void deinit_acmphs(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_ACMPHS_Close(&g_comparator0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_ACMPHS_Close API FAILED **\r\n");
    }
}

void acmphs_test(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_DAC_B_Open(&g_dac_b1_ctrl, &g_dac_b1_cfg);
    if (FSP_SUCCESS != err)
    {
        deinit_dac();
        printf("DAC_Open API FAILED\r\n");
    }
    printf("DAC start to output value.\r\n");
    err = R_DAC_B_Start(&g_dac_b1_ctrl);
    if(FSP_SUCCESS != err)
    {
        printf("DAC start failed\r\n");
    }
    printf("DAC write value: 2500.\r\n");
    err = R_DAC_B_Write(&g_dac_b1_ctrl, 2500);
    if(FSP_SUCCESS != err)
    {
        printf("DAC write failed\r\n");
    }

    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    start_comparator = 1;
    err = R_ACMPHS_Open(&g_comparator0_ctrl, &g_comparator0_cfg);
    if(FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("DAC write failed\r\n");
    }
    R_ACMPHS_OutputEnable (&g_comparator0_ctrl);
    /*---wait comparator to stabilize---*/
    comparator_info_t info;
    R_ACMPHS_InfoGet(&g_comparator0_ctrl, &info);
    R_BSP_SoftwareDelay(info.min_stabilization_wait_us, BSP_DELAY_UNITS_MICROSECONDS);



    while(1)
    {
        if(start_comparator == 1)
        {
            if (b_comparator_state_flag == true)
            {
                start_comparator = 0;
                err = R_ACMPHS_StatusGet(&g_comparator0_ctrl, &acmphs_status);
                if (FSP_SUCCESS != err)
                {
                    printf("\r\n** R_ACMPHS_StatusGet API FAILED **\r\n");
                    deinit_acmphs();
                    printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
                }
                /* Clear flag */
                b_comparator_state_flag = false;
                if (COMPARATOR_STATE_OUTPUT_HIGH == acmphs_status.state)
                {
                    printf("Comparator output high\r\n");
                    err = R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
                     /* Handle error */
                    if (FSP_SUCCESS != err)
                    {
                        APP_ERR_PRINT ("\r\n** R_IOPORT_PinWrite API FAILED **\r\n");
                        deinit_acmphs();
                        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
                    }
                }
                else if(COMPARATOR_STATE_OUTPUT_LOW == acmphs_status.state)
                {
                    printf("Comparator output low\r\n");
                    err = R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED, BSP_IO_LEVEL_LOW);
                     /* Handle error */
                    if (FSP_SUCCESS != err)
                    {
                        APP_ERR_PRINT ("\r\n** R_IOPORT_PinWrite API FAILED **\r\n");
                        deinit_acmphs();
                        printf("\r\nReturned Error Code: 0x%x  \r\n", (err));
                    }

                }
                R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
                printf("\r\nDAC write value: 100.\r\n");
                err = R_DAC_B_Write(&g_dac_b1_ctrl, 100);
                if (FSP_SUCCESS != err)
                {
                    printf("DAC write failed\r\n");
                }
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
                start_comparator = 1;

            }
        }
    }
}
