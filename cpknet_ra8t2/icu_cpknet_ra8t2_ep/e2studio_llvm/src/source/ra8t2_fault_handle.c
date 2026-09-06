#include <stdio.h>

#include "hal_data.h"

void __assert_func(const char * file, int line, const char * func, const char * expr)
{
    __disable_irq();
    puts("\r\n====== Assert Error ======\r\n");
    puts("File:\r\n");
    puts(file);
    printf(" at line: %d", line);
    puts("Function: ");
    puts(func);
    puts("\r\n");
    puts("Assert: ");
    puts(expr);
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
    puts("=============== In HardFault ===============\r\n");
    if (SCB->HFSR & 0x80000000) {
        puts("HardFault occurred by DEBUG\r\n");
    }
    else if (SCB->HFSR & 0x40000000) {
        puts("HardFault occurred by other exception forced\r\n");
    }

    msp = __get_MSP();
    p_stack = (uint32_t *)msp;
    puts("========== MSP Stack  ==========\r\n");
    printf("R0:  0x%08X\r\n", p_stack[0]);
    printf("R1:  0x%08X\r\n", p_stack[1]);
    printf("R2:  0x%08X\r\n", p_stack[2]);
    printf("R3:  0x%08X\r\n", p_stack[3]);
    printf("R12: 0x%08X\r\n", p_stack[4]);
    printf("LR:  0x%08X\r\n", p_stack[5]);
    printf("PC:  0x%08X\r\n", p_stack[6]);
    printf("PSR: 0x%08X\r\n", p_stack[7]);

    puts("========= SCB Register =========\r\n");
    printf("SCB->CSFR: 0x%08X\r\n", SCB->CFSR);
    printf("SCB->ICSR: 0x%08X\r\n", SCB->ICSR);
    printf("SCB->HFSR: 0x%08X\r\n", SCB->HFSR);

    puts("======== Other Register ========\r\n");
    printf("IPSR: 0x%08X\r\n", __get_IPSR());

    if (SCB->CFSR & 0x02000000) {
        puts("Checked Error: Divide By Zero\r\n");
    }
    if (SCB->CFSR & 0x01000000) {
        puts("Checked Error: Unaligned Access\r\n");
    }
    if (SCB->CFSR & 0x00100000) {
        puts("Checked Error: Stack Overflow\r\n");
        printf("MSP:    0x%08X\r\n", msp);
        printf("MSPLIM: 0x%08X\r\n", __get_MSPLIM());
        printf("PSP:    0x%08X\r\n", __get_PSP());
        printf("PSPLIM: 0x%08X\r\n", __get_PSPLIM());
    }
    if (SCB->CFSR & 0x00080000) {
        puts("Checked Error: No Coprocessor\r\n");
    }
    if (SCB->CFSR & 0x00040000) {
        puts("Checked Error: Invalid PC\r\n");
    }
    if (SCB->CFSR & 0x00020000) {
        puts("Checked Error: Invalid State\r\n");
    }
    if (SCB->CFSR & 0x00010000) {
        puts("Checked Error: Undefined Instruction\r\n");
    }
    if (SCB->CFSR & 0x00000100) {
        puts("Checked Error: Instruction Bus Error\r\n");
    }

    __BKPT(0);
}
