/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#ifndef USER_FREERTOSCONFIG_H_
#define USER_FREERTOSCONFIG_H_

#define configUSE_TRACE_FACILITY 1

void lv_freertos_task_switch_in(const char * name);
void lv_freertos_task_switch_out(void);

#define traceTASK_SWITCHED_IN()   lv_freertos_task_switch_in(pxCurrentTCB->pcTaskName);
#define traceTASK_SWITCHED_OUT()  lv_freertos_task_switch_out();

#endif /* USER_FREERTOSCONFIG_H_ */
