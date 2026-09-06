/*
 * perf_dac.c
 *
 *  Created on: Jan 28, 2026
 *      Author: Ran Qingling
 */
#include"hal_data.h"
#include"perf_dac.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

DATA_AREA_BSS volatile uint32_t convert_end;// __attribute__((section(".dtcm_noinit")));
//volatile uint32_t adc_count;//
//DATA_AREA_BSS volatile uint32_t timer_count;// __attribute__((section(".dtcm_noinit")));
//DATA_AREA_BSS volatile bool timer_count_end;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS uint16_t g_adc_data;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS float g_adc_volt;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS uint8_t g_volt_str[5];// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS uint16_t count_num;// __attribute__((section(".dtcm_noinit")));


CODE_AREA
void adc0_callback (adc_callback_args_t * p_args)
{
    if (ADC_EVENT_SCAN_COMPLETE == p_args->event)
    {
        convert_end = 1;
    }
}
CODE_AREA
fsp_err_t init_dac_driver(void)
{
    fsp_err_t err = FSP_SUCCESS;
    adc_status_t adc_status = {.state = ADC_STATE_CALIBRATION_IN_PROGRESS};
    printf("\r\nDAC example code to demonstrate the functionality.\r\n");
    printf("\r\nDAC1 P015 used as DAC output, ADC0 channel5 P005 used as ADC input to measure the DAC value  \r\n");
    printf("Connect P005 to P015 according to readme file. \r\n");
    printf("set DAC value is: 2000 \r\n");
    printf("DAC initialize......\r\n");
    count_num =0;
    /* Open the DAC channel */
    err = R_DAC_B_Open(&g_dac_b1_ctrl, &g_dac_b1_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("DAC_Open API FAILED\r\n");
        return err;
    }
    err = R_DAC_B_Write(&g_dac_b1_ctrl, 2000);
    if(FSP_SUCCESS != err)
    {
        printf("DAC write failed\r\n");
        return err;
    }
    err = R_DAC_B_Start(&g_dac_b1_ctrl);
    if(FSP_SUCCESS != err)
    {
        printf("DAC start failed\r\n");
        return err;
    }
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    printf("ADC initialize......\r\n");
    err = R_ADC_B_Open(&g_adc0_ctrl, &g_adc0_cfg);
    if(FSP_SUCCESS != err)
    {
        printf("ADC open failed\r\n");
        return err;
    }
    err = R_ADC_B_ScanCfg(&g_adc0_ctrl, &g_adc0_scan_cfg);
    if(FSP_SUCCESS != err)
    {
        printf("ADC scancfg failed\r\n");
        return err;
    }
    err = R_ADC_B_Calibrate(&g_adc0_ctrl, NULL);
    if (FSP_SUCCESS != err)
    {
         printf("ADC Calibrate failed\r\n");
         return err;
    }
     // Wait for calibration completion
    uint32_t timeout_count = 100;
    while ((ADC_STATE_IDLE != adc_status.state) &&(FSP_SUCCESS == err))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        err = R_ADC_B_StatusGet(&g_adc0_ctrl, &adc_status);
        if (FSP_SUCCESS != err)
        {
             printf("ADC StatusGet failed\r\n");
             return err;
        }
        if (--timeout_count == 0)
        {
            printf("\r\nADC Wait Timeout Error!");
            return FSP_ERR_TIMEOUT;
        }
    }

    return err;
}
CODE_AREA
fsp_err_t adc_read_data(void)
{
    fsp_err_t err = FSP_SUCCESS;     /* Error status */
    convert_end = 0;
    /* Read the result */
    printf("ADC convert start......\r\n");
    err = R_ADC_B_ScanGroupStart(&g_adc0_ctrl, ADC_GROUP_MASK_0);
    if (FSP_SUCCESS != err)
    {
         printf("ADC scan group start failed\r\n");
         return err;
    }
    uint16_t timecount =0;
    while(!convert_end)
    {
        timecount ++;
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        if(timecount>=100)
        {
             printf("\r\n[Error]: ADC conversion timeout!\r\n");
             err = FSP_ERR_TIMEOUT;
             break;
        }
    }
    err = R_ADC_B_Read(&g_adc0_ctrl, ADC_CHANNEL_5, &g_adc_data);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        /* ADC failure message */
        printf("** R_ADC__Read API failed **\r\n");
        return err;
    }
    {
        g_adc_volt = (float)((g_adc_data * 3.33)/4096);
    }

    snprintf((char *)g_volt_str, sizeof(g_volt_str), "%0.2f", g_adc_volt);

    printf("\r\nThe Voltage Reading from ADC: %d [%d]\r\n", g_adc_data,count_num);
    printf("\r\nThe ADC input voltage: %s\r\n", g_volt_str);
    count_num ++;
    return err;
}
CODE_AREA
fsp_err_t adc_scan_stop(void)
{
    fsp_err_t err = FSP_SUCCESS;     /* Error status */
    err = R_ADC_B_ScanStop(&g_adc0_ctrl);
    if (FSP_SUCCESS != err)
    {
         printf("ADC scan group stop failed\r\n");
         return err;
    }
    printf("ADC Scan stopped\r\n");
    err = R_ADC_B_Close(&g_adc0_ctrl);
    if (FSP_SUCCESS != err)
    {
         printf("ADC close failed\r\n");
         return err;
    }
    return err;
}
