/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 *
 */
#ifndef __CAMERA_I2C_API_H__
#define __CAMERA_I2C_API_H__

extern fsp_err_t camera_i2c_comm_write(uint32_t sub_address, uint32_t sub_address_length, const uint8_t *data, size_t data_length);
extern fsp_err_t camera_i2c_comm_read(uint32_t sub_address, uint32_t sub_address_length, uint8_t *data, size_t data_length);


#endif
