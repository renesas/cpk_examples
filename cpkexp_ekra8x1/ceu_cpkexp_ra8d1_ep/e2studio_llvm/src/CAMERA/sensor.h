/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Sensor abstraction layer.
 */
#ifndef __SENSOR_H__
#define __SENSOR_H__
#include <stdarg.h>


#define OV3640_SLV_ADDR         (0x14)


#define OV2640_SLV_ADDR         (0x60)
#define OV5640_SLV_ADDR         (0x78)
#define OV7725_SLV_ADDR         (0x42)
#define MT9V0XX_SLV_ADDR        (0x90)
#define MT9M114_SLV_ADDR        (0x90)
#define LEPTON_SLV_ADDR         (0x54)
#define HM0XX0_SLV_ADDR         (0x48)
#define GC2145_SLV_ADDR         (0x78)
#define FROGEYE2020_SLV_ADDR    (0x6E)
#define GC0328_SLV_ADDR         (0x42)

// Chip ID Registers
#define OV5640_CHIP_ID          (0x300A)
#define OV_CHIP_ID              (0x0A)
#define ON_CHIP_ID              (0x00)
#define HIMAX_CHIP_ID           (0x0001)
#define GC_CHIP_ID              (0xF0)

// Chip ID Values
#define OV2640_ID               (0x26)
#define OV5640_ID               (0x56)
#define OV7670_ID               (0x76)
#define OV7690_ID               (0x76)
#define OV7725_ID               (0x77)
#define OV9650_ID               (0x96)
#define MT9V0X2_ID_V_1          (0x1311)
#define MT9V0X2_ID_V_2          (0x1312)
#define MT9V0X2_ID              (0x1313)
#define MT9V0X2_C_ID            (0x1413)
#define MT9V0X4_ID              (0x1324)
#define MT9V0X4_C_ID            (0x1424)
#define MT9M114_ID              (0x2481)
#define LEPTON_ID               (0x54)
#define LEPTON_1_5              (0x5415)
#define LEPTON_1_6              (0x5416)
#define LEPTON_2_0              (0x5420)
#define LEPTON_2_5              (0x5425)
#define LEPTON_3_0              (0x5430)
#define LEPTON_3_5              (0x5435)
#define HM01B0_ID               (0xB0)
#define HM0360_ID               (0x60)
#define GC2145_ID               (0x21)
#define PAJ6100_ID              (0x6100)
#define FROGEYE2020_ID          (0x2020)
#define GC0328_ID               (0x9d)

typedef enum {
    FRAMESIZE_INVALID = 0,
    // C/SIF Resolutions
    FRAMESIZE_QQCIF,    // 88x72
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_CIF,      // 352x288
    FRAMESIZE_QQSIF,    // 88x60
    FRAMESIZE_QSIF,     // 176x120
    FRAMESIZE_SIF,      // 352x240
    // VGA Resolutions
    FRAMESIZE_QQQQVGA,  // 40x30
    FRAMESIZE_QQQVGA,   // 80x60
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_HQQQQVGA, // 30x20
    FRAMESIZE_HQQQVGA,  // 60x40
    FRAMESIZE_HQQVGA,   // 120x80
    FRAMESIZE_HQVGA,    // 240x160
    FRAMESIZE_HVGA,     // 480x320
    // FFT Resolutions
    FRAMESIZE_64X32,    // 64x32
    FRAMESIZE_64X64,    // 64x64
    FRAMESIZE_128X64,   // 128x64
    FRAMESIZE_128X128,  // 128x128
    // Himax Resolutions
    FRAMESIZE_160X160,  // 160x160
    FRAMESIZE_320X320,  // 320x320
    // Other
    FRAMESIZE_LCD,      // 128x160
    FRAMESIZE_QQVGA2,   // 128x160
    FRAMESIZE_WVGA,     // 720x480
    FRAMESIZE_WVGA2,    // 752x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_WXGA,     // 1280x768
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_SXGAM,    // 1280x960
    FRAMESIZE_UXGA,     // 1600x1200
    FRAMESIZE_HD,       // 1280x720
    FRAMESIZE_FHD,      // 1920x1080
    FRAMESIZE_QHD,      // 2560x1440
    FRAMESIZE_QXGA,     // 2048x1536
    FRAMESIZE_WQXGA,    // 2560x1600
    FRAMESIZE_WQXGA2,   // 2592x1944
} framesize_t;

typedef enum {
    GAINCEILING_2X,
    GAINCEILING_4X,
    GAINCEILING_8X,
    GAINCEILING_16X,
    GAINCEILING_32X,
    GAINCEILING_64X,
    GAINCEILING_128X,
} gainceiling_t;

typedef enum {
    SDE_NORMAL,
    SDE_NEGATIVE,
} sde_t;

typedef enum {
    ATTR_CONTRAST=0,
    ATTR_BRIGHTNESS,
    ATTR_SATURATION,
    ATTR_GAINCEILING,
} sensor_attr_t;

typedef enum {
    ACTIVE_LOW,
    ACTIVE_HIGH
} polarity_t;

typedef enum {
    IOCTL_SET_READOUT_WINDOW,
    IOCTL_GET_READOUT_WINDOW,
    IOCTL_SET_TRIGGERED_MODE,
    IOCTL_GET_TRIGGERED_MODE,
    IOCTL_TRIGGER_AUTO_FOCUS,
    IOCTL_PAUSE_AUTO_FOCUS,
    IOCTL_RESET_AUTO_FOCUS,
    IOCTL_WAIT_ON_AUTO_FOCUS,
    IOCTL_SET_NIGHT_MODE,
    IOCTL_GET_NIGHT_MODE,
    IOCTL_LEPTON_GET_WIDTH,
    IOCTL_LEPTON_GET_HEIGHT,
    IOCTL_LEPTON_GET_RADIOMETRY,
    IOCTL_LEPTON_GET_REFRESH,
    IOCTL_LEPTON_GET_RESOLUTION,
    IOCTL_LEPTON_RUN_COMMAND,
    IOCTL_LEPTON_SET_ATTRIBUTE,
    IOCTL_LEPTON_GET_ATTRIBUTE,
    IOCTL_LEPTON_GET_FPA_TEMPERATURE,
    IOCTL_LEPTON_GET_AUX_TEMPERATURE,
    IOCTL_LEPTON_SET_MEASUREMENT_MODE,
    IOCTL_LEPTON_GET_MEASUREMENT_MODE,
    IOCTL_LEPTON_SET_MEASUREMENT_RANGE,
    IOCTL_LEPTON_GET_MEASUREMENT_RANGE,
    IOCTL_HIMAX_MD_ENABLE,
    IOCTL_HIMAX_MD_CLEAR,
    IOCTL_HIMAX_MD_WINDOW,
    IOCTL_HIMAX_MD_THRESHOLD,
    IOCTL_HIMAX_OSC_ENABLE,
} ioctl_t;

typedef enum {
    SENSOR_ERROR_NO_ERROR              =  0,
    SENSOR_ERROR_CTL_FAILED            = -1,
    SENSOR_ERROR_CTL_UNSUPPORTED       = -2,
    SENSOR_ERROR_ISC_UNDETECTED        = -3,
    SENSOR_ERROR_ISC_UNSUPPORTED       = -4,
    SENSOR_ERROR_ISC_INIT_FAILED       = -5,
    SENSOR_ERROR_TIM_INIT_FAILED       = -6,
    SENSOR_ERROR_DMA_INIT_FAILED       = -7,
    SENSOR_ERROR_DCMI_INIT_FAILED      = -8,
    SENSOR_ERROR_IO_ERROR              = -9,
    SENSOR_ERROR_CAPTURE_FAILED        = -10,
    SENSOR_ERROR_CAPTURE_TIMEOUT       = -11,
    SENSOR_ERROR_INVALID_FRAMESIZE     = -12,
    SENSOR_ERROR_INVALID_PIXFORMAT     = -13,
    SENSOR_ERROR_INVALID_WINDOW        = -14,
    SENSOR_ERROR_INVALID_FRAMERATE     = -15,
    SENSOR_ERROR_INVALID_ARGUMENT      = -16,
    SENSOR_ERROR_PIXFORMAT_UNSUPPORTED = -17,
    SENSOR_ERROR_FRAMEBUFFER_ERROR     = -18,
    SENSOR_ERROR_FRAMEBUFFER_OVERFLOW  = -19,
    SENSOR_ERROR_JPEG_OVERFLOW         = -20,
} sensor_error_t;

// Bayer patterns.
// NOTE: These must match the Bayer subformats in imlib.h

#endif /* __SENSOR_H__ */
