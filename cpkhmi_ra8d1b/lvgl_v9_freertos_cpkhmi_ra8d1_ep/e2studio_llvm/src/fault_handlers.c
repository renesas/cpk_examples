/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "bsp_api.h"

void HardFault_Handler(void) __attribute ( ( naked ) );
void MemManage_Handler(void) ;
void BusFault_Handler(void) ;
void SecureFault_Handler(void) ;
void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress );

/*
 * Need to enable individual fault handlers :-
 *
 *     SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | \
            SCB_SHCSR_BUSFAULTENA_Msk | \
            SCB_SHCSR_MEMFAULTENA_Msk );
 */

volatile uint32_t g_ccr = 0;

void HardFault_Handler(void)
{
    __asm volatile
    (
        ".align 8                                                   \n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void MemManage_Handler(void)
{
    g_ccr = __get_CONTROL();
    __BKPT(1);
}

void BusFault_Handler(void)
{
    g_ccr = __get_CONTROL();
    __BKPT(1);
}

void SecureFault_Handler(void)
{
    g_ccr = __get_CONTROL();
    __BKPT(1);
}

void UsageFault_Handler( void ) __attribute__( ( naked ) );

/* The fault handler implementation calls a function called
prvGetRegistersFromStack(). */
void UsageFault_Handler(void)
{

}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr  = pulFaultStackAddress[ 5 ];
    pc  = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    FSP_PARAMETER_NOT_USED(r0);
    FSP_PARAMETER_NOT_USED(r1);
    FSP_PARAMETER_NOT_USED(r2);
    FSP_PARAMETER_NOT_USED(r3);
    FSP_PARAMETER_NOT_USED(r12);
    FSP_PARAMETER_NOT_USED(lr);
    FSP_PARAMETER_NOT_USED(pc);
    FSP_PARAMETER_NOT_USED(psr);

    g_ccr = __get_CONTROL();

    __BKPT(1);
}
