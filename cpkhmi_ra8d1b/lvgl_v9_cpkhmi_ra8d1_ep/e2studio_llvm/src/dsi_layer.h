/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include "r_mipi_dsi.h"

#define REGFLAG_DELAY 0xFE
#define REGFLAG_END_OF_TABLE 0xFD

typedef struct {
    unsigned char size;
    unsigned char buffer[10];
    uint8_t msg_id;
    uint8_t flags;
} LCD_setting_table;


extern const mipi_dsi_cfg_t g_mipi_dsi1_cfg;
extern mipi_dsi_instance_ctrl_t  g_mipi_dsi0_ctrl;
extern LCD_setting_table lcd_init_focuslcd[];

fsp_err_t dsi_layer_configure_peripheral(void);



