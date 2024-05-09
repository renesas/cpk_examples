/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "dwt.h"
#define    DWT_CR               *(uint32_t*)0xE0001000
#define    DWT_CYCCNT           *(uint32_t*)0xE0001004
#define    DEM_CR               *(uint32_t*)0xE000EDFC
//Enable bit
#define    DEM_CR_TRCENA        (1<<24)
#define    DWT_CR_CYCCNTENA     (1<<0)


//DWT init
void DWT_init(void)
{
    DEM_CR |= (uint32_t)DEM_CR_TRCENA;
    DWT_CYCCNT = (uint32_t)0u;
    DWT_CR |= (uint32_t)DWT_CR_CYCCNTENA;
}
//get DWT count
uint32_t DWT_TS_GET(void)
{
    return((uint32_t)DWT_CYCCNT);
}

void DWT_Reset(void)
{
    DWT_CYCCNT = (uint32_t)0u;
}
