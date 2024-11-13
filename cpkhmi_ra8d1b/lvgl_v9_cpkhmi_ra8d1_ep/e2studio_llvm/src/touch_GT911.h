/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#ifndef __TOUCH_GT911_H
#define __TOUCH_GT911_H
#include "hal_data.h"

typedef struct{
    uint16_t x;
    uint16_t y;
}TouchCordinate_t;


typedef enum
{
    TOUCH_EVENT_NONE,
    TOUCH_EVENT_DOWN,
    TOUCH_EVENT_HOLD,
    TOUCH_EVENT_MOVE,
    TOUCH_EVENT_UP
} touch_event_t;

#define GT911_REG_PRODUCT_ID       0x8140
#define GT911_REG_READ_COORD_ADDR  0x814E
#define GT911_REG_POINT1_X_ADDR    0x814F
#define GT911_REG_COMMAND          0x8040
#define GT911_REG_CONFIG_VERSION   0x8047
#define GT911_REG_CONFIG_CHECKSUM  0x80FF
#define GT911_REG_CONFIG_FRESH     0x8100
#define GT911_REG_FW_VER_HIGH      0x8145

//Reg 0x814E bit fields
#define BUFFER_READY           (1<< 7)
#define NUM_TOUCH_POINTS_MASK  0x0F

fsp_err_t enable_ts(i2c_master_ctrl_t * p_api_i2c_ctrl, external_irq_ctrl_t * p_api_irq_ctrl);
fsp_err_t init_ts(i2c_master_ctrl_t * p_api_ctrl);
extern volatile bool is_touch_pressed ;
#endif
