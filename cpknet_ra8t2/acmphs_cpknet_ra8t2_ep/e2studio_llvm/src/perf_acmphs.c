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
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#define DAC_MAX_VAL          (4095U)
#define DAC_REF_VAL          (2048U)        /* Mid count value for 12 bit DAC */
#define LOW_VAL              (2000U)        /* Lower value of fluctuating range */
#define HIGH_VAL             (2100U)        /* Higher value of fluctuating range */
static volatile bool b_comparator_state_flag = 0;
volatile bool start_comparator = 0U;

comparator_status_t acmphs_status;
volatile uint16_t current_dac_val;
CODE_AREA
void acmphs_user_callback(comparator_callback_args_t *p_args)
{
    /* Check for the channel 0 of comparator */
    if(0 == p_args->channel)
    {
        /* Toggle the flag */
        b_comparator_state_flag = true;
    }
}
CODE_AREA
static void deinit_dac(dac_b_instance_ctrl_t * p_ctrl)
{
    fsp_err_t err = FSP_SUCCESS;
    /* Close DAC */
    err = R_DAC_B_Close(p_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_DAC_B_Close API FAILED **\r\n");
    }
}
CODE_AREA
static void deinit_acmphs(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_ACMPHS_Close(&g_comparator0_ctrl);
    if (FSP_SUCCESS != err)
    {
        printf("\r\n** R_ACMPHS_Close API FAILED **\r\n");
    }
}
CODE_AREA
void acmphs_test(void)
{
    fsp_err_t err = FSP_SUCCESS;
    comparator_info_t stabilize_time={0};
    /* Open DAC0 module */
    err = R_DAC_B_Open(&g_dac_b0_ctrl, &g_dac_b0_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("DAC0_Open API FAILED\r\n");
    }
    //current_dac_val = 2000;
    /* Open DAC1 module */
    err = R_DAC_B_Open(&g_dac_b1_ctrl, &g_dac_b1_cfg);
    if (FSP_SUCCESS != err)
    {
        deinit_dac(&g_dac_b0_ctrl);
        printf("DAC1_Open API FAILED\r\n");
    }
    /* Open ACMPHS module */
    err = R_ACMPHS_Open(&g_comparator0_ctrl, &g_comparator0_cfg);
    if(FSP_SUCCESS != err)
    {
        deinit_dac(&g_dac_b0_ctrl);
        deinit_dac(&g_dac_b1_ctrl);
        //deinit_acmphs();
        printf("R_ACMPHS_Open failed\r\n");
    }
    /* Write 2048 (≈ 1.65V) on DAC0 module for reference voltage to comparator */
    err = R_DAC_B_Write(&g_dac_b0_ctrl, DAC_REF_VAL);
    if(FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("DAC0 write failed\r\n");
    }
    /* Write 0 on DAC1 module for providing analog input voltage to comparator
     * and avoid garbage to ACMPHS input */
    err = R_DAC_B_Write(&g_dac_b1_ctrl, 0);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("DAC1 write failed\r\n");
    }
    /* Start conversion on DAC0 module */
    err = R_DAC_B_Start(&g_dac_b0_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("DAC0 start failed\r\n");
    }

    /* Start conversion on DAC1 module */
    err = R_DAC_B_Start(&g_dac_b1_ctrl);;
    if (FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("DAC1 start failed\r\n");
    }
    /* Get the minimum stabilization wait time */
    err = R_ACMPHS_InfoGet(&g_comparator0_ctrl, &stabilize_time);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("R_ACMPHS_InfoGet failed\r\n");
    }
    err = R_ACMPHS_OutputEnable(&g_comparator0_ctrl);
    if (FSP_SUCCESS != err)
    {
        deinit_acmphs();
        printf("R_ACMPHS_OutputEnable failed\r\n");
    }
    R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    current_dac_val =0;
    while(1)
    {
        current_dac_val =current_dac_val + 300;
        if (DAC_MAX_VAL < current_dac_val) current_dac_val=0;
        else if (LOW_VAL < current_dac_val && current_dac_val < HIGH_VAL)
        {
            current_dac_val=current_dac_val + 100;
        }
        printf("DAC write value: %d.\r\n", current_dac_val);
        err = R_DAC_B_Write(&g_dac_b1_ctrl, current_dac_val);
        /* Handle error */
        if (FSP_SUCCESS != err)
        {
            deinit_acmphs();
            printf("R_DAC_B_Write1 failed\r\n");
        }
        /* Wait for the minimum stabilization wait time */
        R_BSP_SoftwareDelay(stabilize_time.min_stabilization_wait_us, BSP_DELAY_UNITS_MICROSECONDS);
        if (b_comparator_state_flag == true)
        {
            /* Check status of comparator */
            err = R_ACMPHS_StatusGet(&g_comparator0_ctrl, &acmphs_status);
            if (FSP_SUCCESS != err)
            {
                deinit_acmphs();
                printf ("\r\n** R_ACMPHS_StatusGet  FAILED **\r\n");
            }
            /* Clear flag */
            b_comparator_state_flag = false;
        }
        else
        {
            /* Do nothing */
        }
        if (COMPARATOR_STATE_OUTPUT_HIGH == acmphs_status.state)
        {
            printf("Comparator output high\r\n");
            err = R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
             /* Handle error */
            if (FSP_SUCCESS != err)
            {
                deinit_acmphs();
                printf ("\r\n** R_IOPORT_PinWrite API FAILED **\r\n");
            }
        }
        else if(COMPARATOR_STATE_OUTPUT_LOW == acmphs_status.state)
        {
            printf("Comparator output low\r\n");
            err = R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED, BSP_IO_LEVEL_LOW);
             /* Handle error */
            if (FSP_SUCCESS != err)
             {
                 deinit_acmphs();
                 printf ("\r\n** R_IOPORT_PinWrite API FAILED **\r\n");
             }

        }
        R_BSP_SoftwareDelay(3000, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
