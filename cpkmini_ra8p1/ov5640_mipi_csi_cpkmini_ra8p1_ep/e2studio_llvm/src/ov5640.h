#ifndef __OV5640_H
#define __OV5640_H

#include <stdint.h>

#define OV5640_INTERFACE_CSI	1
#define OV5640_INTERFACE_DVP	2

#ifndef OV5640_USING_INTERFACE
#define OV5640_USING_INTERFACE	OV5640_INTERFACE_CSI
#endif

#define OV5640_FIRMWARE_ADDR	0x8000
#define OV5640_IIC_ADDR			0x3C
#define OV5640_MAX_LENGTH		2592
#define OV5640_MAX_WIDTH		1944

#if OV5640_USING_INTERFACE == OV5640_INTERFACE_CSI
#define OV5640_MIPI_XCLK_HZ			24000000
#define OV5640_MAX_PCLK_HZ			96000000
/* REG 0x3035 config */
#define OV5640_SYS_CLK_DIV			0x01
#define OV5640_MIPI_SCALE_DIV		0x02
/* REG 0x3036 config */
#define OV5640_PLL_MULTIPLIER		0x7B
/* REG 0x3037 config */
#define OV5640_ROOT_DIVIDER_PLL		0x00
#define OV5640_PLL_PRE_DIVIDER		0x08
/* REG 0x3108 config */
#define OV5640_ROOT_DIVIDER_PCLK	0x01 /* PLL_CLKI / 2 */
#define OV5640_ROOT_DIVIDER_SCLK2X	0x00 /* PLL_CLKI */
#define OV5640_ROOT_DIVIDER_SCLK	0x02 /* PLL_CLKI / 4 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	EXPOSURE_LEVEL_0,
	EXPOSURE_LEVEL_1,
	EXPOSURE_LEVEL_2,
	EXPOSURE_LEVEL_3,
	EXPOSURE_LEVEL_4,
	EXPOSURE_LEVEL_5,
	EXPOSURE_LEVEL_6,
	EXPOSURE_LEVEL_7,
	EXPOSURE_LEVEL_8,
	EXPOSURE_LEVEL_9,
	EXPOSURE_LEVEL_10
} OV5640_ExposureLevelEnum;

typedef enum {
	OUTPUT_FORMAT_RGB565,
	OUTPUT_FORMAT_JPEG,
	OUTPUT_FORMAT_YUV422,
	OUTPUT_FORMAT_YUV420
} OV5640_OutputFormatEnum;

uint32_t OV5640_GetID(uint16_t *id);
uint32_t OV5640_GetOutputSize(uint16_t *length, uint16_t *width);
uint32_t OV5640_Init(uint16_t length, uint16_t width);
#if OV5640_USING_INTERFACE == OV5640_INTERFACE_CSI
uint32_t OV5640_IsImageReady(uint8_t **p_image);
#else
uint32_t OV5640_IsImageReady(void);
#endif
uint32_t OV5640_SetAutoFocus(uint8_t enable);
uint32_t OV5640_SetExposureLevel(OV5640_ExposureLevelEnum level);
uint32_t OV5640_SetFPS(uint16_t target, uint16_t *real);
uint32_t OV5640_SetMIPIChannel(uint8_t channel);
uint32_t OV5640_SetOutputFormat(OV5640_OutputFormatEnum format);
uint32_t OV5640_SetOutputSize(uint16_t length, uint16_t width);
uint32_t OV5640_StartCapture(uint8_t *image_buffer);

#ifdef __cplusplus
}
#endif

#endif
