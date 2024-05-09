/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef DWT_H_
#define DWT_H_

#include "hal_data.h"

//DWT init
void DWT_init(void);
//get DWT count
uint32_t DWT_TS_GET(void);

void DWT_Reset(void);

#endif //DWT_H_

