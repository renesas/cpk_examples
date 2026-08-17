#ifndef __OV7725_H
#define __OV7725_H

#include <stdint.h>
#include "camera_def.h"

#define OV7725_IIC_ADDR		0x21

#ifdef __cplusplus
extern "C" {
#endif

uint32_t OV7725_GetID(uint16_t *id);
uint32_t OV7725_Init(CameraInterfaceEnum interface);
uint32_t OV7725_IsImageReady(CameraInterfaceEnum interface, uint8_t **p_image);
uint32_t OV7725_IsPowerDown(CameraInterfaceEnum interface);
uint32_t OV7725_PowerDown(CameraInterfaceEnum interface, uint8_t enable);
uint32_t OV7725_SetOutputSize(uint16_t length, uint16_t width);
uint32_t OV7725_StartCapture(const void *peri, CameraInterfaceEnum interface, uint8_t *image_buffer);
uint32_t OV7725_StopCapture(const void *peri, CameraInterfaceEnum interface);

#ifdef __cplusplus
}
#endif

#endif
