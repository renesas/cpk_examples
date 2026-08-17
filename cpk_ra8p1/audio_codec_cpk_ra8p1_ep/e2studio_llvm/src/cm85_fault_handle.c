#include <stdio.h>

#include "hal_data.h"
#include "utils/log.h"

void __assert_func(const char * file, int line, const char * func, const char * expr)
{
    __disable_irq();
    LOG_PutsEndl("\r\n====== Assert Error ======");
    LOG_PutsEndl("File:");
    LOG_Puts(file);
    LOG_PrintfEndl(" at line: %d", line);
    LOG_Puts("Function: ");
    LOG_PutsEndl(func);
    LOG_Puts("Assert: ");
    LOG_PutsEndl(expr);
    while (1) {
        R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_HIGH);
		R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
		R_IOPORT_PinWrite(g_ioport.p_ctrl, USER_LED, BSP_IO_LEVEL_LOW);
		R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

#if !defined(__clang_version__) && !defined(__ARMCOMPILER_VERSION)
#pragma GCC diagnostic ignored "-Wformat="
void HardFault_Handler(void);
#endif
void HardFault_Handler(void)
{
    uint32_t msp;

    uint32_t *p_stack = NULL;

    __disable_irq();
    LOG_PutsEndl("\r\n=============== In HardFault ===============");
    if (SCB->HFSR & 0x80000000) {
        LOG_PutsEndl("HardFault occurred by DEBUG");
    }
    else if (SCB->HFSR & 0x40000000) {
        LOG_PutsEndl("HardFault occurred by other exception forced");
    }

    msp = __get_MSP();
    p_stack = (uint32_t *)msp;
    LOG_PutsEndl("========== MSP Stack  ==========");
    LOG_PrintfEndl("R0:  0x%08X", p_stack[0]);
    LOG_PrintfEndl("R1:  0x%08X", p_stack[1]);
    LOG_PrintfEndl("R2:  0x%08X", p_stack[2]);
    LOG_PrintfEndl("R3:  0x%08X", p_stack[3]);
    LOG_PrintfEndl("R12: 0x%08X", p_stack[4]);
    LOG_PrintfEndl("LR:  0x%08X", p_stack[5]);
    LOG_PrintfEndl("PC:  0x%08X", p_stack[6]);
    LOG_PrintfEndl("PSR: 0x%08X", p_stack[7]);

    LOG_PutsEndl("\r\n========= SCB Register =========");
    LOG_PrintfEndl("SCB->CSFR: 0x%08X", SCB->CFSR);
    LOG_PrintfEndl("SCB->ICSR: 0x%08X", SCB->ICSR);
    LOG_PrintfEndl("SCB->HFSR: 0x%08X", SCB->HFSR);

    LOG_PutsEndl("\r\n======== Other Register ========");
    LOG_PrintfEndl("IPSR: 0x%08X", __get_IPSR());

    if (SCB->CFSR & 0x02000000) {
        LOG_PutsEndl("Checked Error: Divide By Zero");
    }
    if (SCB->CFSR & 0x01000000) {
        LOG_PutsEndl("Checked Error: Unaligned Access");
    }
    if (SCB->CFSR & 0x00100000) {
        LOG_PutsEndl("Checked Error: Stack Overflow");
        LOG_PrintfEndl("MSP:    0x%08X", msp);
        LOG_PrintfEndl("MSPLIM: 0x%08X", __get_MSPLIM());
        LOG_PrintfEndl("PSP:    0x%08X", __get_PSP());
        LOG_PrintfEndl("PSPLIM: 0x%08X", __get_PSPLIM());
    }
    if (SCB->CFSR & 0x00080000) {
        LOG_PutsEndl("Checked Error: No Coprocessor");
    }
    if (SCB->CFSR & 0x00040000) {
        LOG_PutsEndl("Checked Error: Invalid PC");
    }
    if (SCB->CFSR & 0x00020000) {
        LOG_PutsEndl("Checked Error: Invalid State");
    }
    if (SCB->CFSR & 0x00010000) {
        LOG_PutsEndl("Checked Error: Undefined Instruction");
    }
    if (SCB->CFSR & 0x00000200) {
    	LOG_PutsEndl("Checked Error: Precise data bus error");
    	if (SCB->CFSR & 0x00008000) {
    		LOG_PrintfEndl("BFAR is valid: 0x%08X\r\n", SCB->BFAR);
    	}
    }
    if (SCB->CFSR & 0x00000100) {
        LOG_PutsEndl("Checked Error: Instruction Bus Error");
    }

    __BKPT(0);
}
