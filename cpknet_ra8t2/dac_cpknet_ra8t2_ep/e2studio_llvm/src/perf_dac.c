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

DATA_AREA_BSS volatile uint32_t convert_end;
DATA_AREA_BSS uint16_t g_adc_data;
DATA_AREA_BSS float g_adc_volt;
DATA_AREA_BSS uint8_t g_volt_str[5];
DATA_AREA_BSS uint16_t count_num;


CODE_AREA
void dac_test(void)
{
    fsp_err_t err = FSP_SUCCESS;
    printf("\r\nDAC example code to demonstrate the functionality.\r\n");
    printf("\r\nDAC1 P015 used as DAC output. \r\n");
    printf("\r\nSet DAC value is: 1500, is about 1.2085V.\r\n");
    printf("DAC open......\r\n");
    count_num =0;
    /* Open the DAC channel */
    err = R_DAC_B_Open(&g_dac_b1_ctrl, &g_dac_b1_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("DAC_Open API FAILED\r\n");
    }
    printf("DAC write value: 1500.\r\n");
    err = R_DAC_B_Write(&g_dac_b1_ctrl, 1500);
    if(FSP_SUCCESS != err)
    {
        printf("DAC write failed\r\n");
    }
    printf("DAC start to output value.\r\n");
    err = R_DAC_B_Start(&g_dac_b1_ctrl);
    if(FSP_SUCCESS != err)
    {
        printf("DAC start failed\r\n");
    }
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
}
