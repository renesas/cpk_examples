/*
 * perf_adc.c
 *
 *  Created on: Jan 15, 2026
 *      Author: Ran QingLing
 */
//#pragma clang attribute push (__attribute__((section(".itcm_from_flash"))), apply_to = function)
#include"hal_data.h"
#include"perf_adc.h"
#include "console.h"
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"

DATA_AREA_BSS volatile uint32_t convert_num;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS volatile uint32_t timer_count;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS uint16_t g_adc_data;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS float g_adc_volt;// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS uint8_t g_volt_str[5];// __attribute__((section(".dtcm_noinit")));
DATA_AREA_BSS volatile  bool g_ADC_scan_end;// __attribute__((section(".dtcm_noinit")));

CODE_AREA
void adc_callback (adc_callback_args_t * p_args)
{
    if (ADC_EVENT_SCAN_COMPLETE == p_args->event)
    {
        convert_num++;
        if(convert_num>=10000) g_ADC_scan_end =1;
    }
}
CODE_AREA
fsp_err_t adc_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    adc_status_t adc_status = {.state = ADC_STATE_CALIBRATION_IN_PROGRESS};
    printf("\r\nADC clock is set to 60MHZ\r\n");
    printf("ADC channel 5, SAR mode, 12bit continues scan mode \r\n");
    printf("ADC sampling time register[ADSSTR0.SST0] is set to 5 :the range is 2-1023\r\n");
    printf("ADC conversion time register[ADCNVSTR] is set to 5 :the range is 3-63\r\n");
    printf("record the time of 10000 ADC conversions\r\n");
    printf("ADC initialize......\r\n");
    printf("ADC convert start......\r\n");
    printf("ADC test running, please wait...\r\n");
    err = R_ADC_B_Open(&g_adc0_ctrl, &g_adc0_cfg);
    if(FSP_SUCCESS != err)
    {
        return err;
    }
    err = R_ADC_B_ScanCfg(&g_adc0_ctrl, &g_adc0_scan_cfg);
    if(FSP_SUCCESS != err)
    {
        return err;
    }
    err = R_ADC_B_Calibrate(&g_adc0_ctrl, NULL);
    if (FSP_SUCCESS != err)
    {
         return err;
    }
     // Wait for calibration completion
    while ((ADC_STATE_IDLE != adc_status.state) &&(FSP_SUCCESS == err))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        err = R_ADC_B_StatusGet(&g_adc0_ctrl, &adc_status);
        if (FSP_SUCCESS != err)
        {
             return err;
        }
    }

    err = R_ADC_B_ScanGroupStart(&g_adc0_ctrl, ADC_GROUP_MASK_0);
    if (FSP_SUCCESS != err)
    {
         return err;
    }
    //timer_count_end=0;
    return err;
}
CODE_AREA
void adc_ContinuesScan_test(void)
{
    convert_num =0;
    g_ADC_scan_end =0;
    while(!g_ADC_scan_end);
    adc_scan_stop();
}
CODE_AREA
fsp_err_t adc_scan_stop(void)
{
    fsp_err_t err = FSP_SUCCESS;     /* Error status */
    err = R_ADC_B_ScanStop(&g_adc0_ctrl);
    if (FSP_SUCCESS != err)
    {
         return err;
    }
    err = R_ADC_B_Close(&g_adc0_ctrl);
    if (FSP_SUCCESS != err)
    {
         return err;
    }
    return err;
}
//#pragma clang attribute pop

