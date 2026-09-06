/*
 * perf_i,cu.c
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */
#include <perf_icu.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "common_utils.h"

volatile bool user_key;
/* Called from icu_irq_isr */
CODE_AREA
void irq0_callback (external_irq_callback_args_t * p_args)
{
    (void) p_args;
    user_key = 1;
    printf ("\r\n**Enter external interrupt **\r\n");
}
CODE_AREA
void icu_test (void)
{

    printf(EP_INFO);
    fsp_err_t err = R_ICU_ExternalIrqOpen(&g_external_irq0_ctrl, &g_external_irq0_cfg);
    if (FSP_SUCCESS != err)
    {
        // turn on led to indicate the err.
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
        printf ("\r\n** R_ICU_ExternalIrqOpen0 API failed **\r\n");
    }
    err = R_ICU_ExternalIrqEnable(&g_external_irq0_ctrl);
    if (FSP_SUCCESS != err)
    {
        // turn on led to indicate the err.
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
        printf ("\r\n** R_ICU_ExternalIrqEnable0 API failed **\r\n");
    }
    user_key=0;
    printf ("\r\n**Press 'USER KEY' button to trigger exteranl interrupt. **\r\n");
    while(1)
    {
        if(user_key==1)
        {
            user_key=0;
            R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
            R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
            R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_LOW);
        }
    }
}
