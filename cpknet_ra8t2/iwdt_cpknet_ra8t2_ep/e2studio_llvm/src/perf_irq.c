/*
 * perf_irq.c
 *
 *  Created on: Feb 27, 2026
 *      Author: a5143926
 */
#include <perf_iwdt.h>
#include "console.h"
#include "hal_data.h"
#include "common_utils.h"
#include <string.h>
#include <perf_irq.h>
#include "coremark/coremark.h"
#include "perf_counter/perf_counter.h"
#include "common_utils.h"
bool user_key, md_key;
/* Called from icu_irq_isr */
CODE_AREA
void irq0_callback (external_irq_callback_args_t * p_args)
{
    (void) p_args;
    user_key = 1;
}
CODE_AREA
void irq1_callback (external_irq_callback_args_t * p_args)
{
    (void) p_args;
    md_key = 1;
}
CODE_AREA
void irq_initialize (void)
{
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
    err = R_ICU_ExternalIrqOpen(&g_external_irq1_ctrl, &g_external_irq1_cfg);
    if (FSP_SUCCESS != err)
    {
        // turn on led to indicate the err.
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
        printf ("\r\n** R_ICU_ExternalIrqOpen1 API failed **\r\n");
    }
    err = R_ICU_ExternalIrqEnable(&g_external_irq1_ctrl);
    if (FSP_SUCCESS != err)
    {
        // turn on led to indicate the err.
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
        printf ("\r\n** R_ICU_ExternalIrqEnable1 API failed **\r\n");
    }
}
