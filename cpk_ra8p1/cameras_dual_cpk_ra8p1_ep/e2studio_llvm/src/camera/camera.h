#ifndef __CAMERA_H
#define __CAMERA_H

#include <stdint.h>
#include "camera_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CameraConfig {
	CameraModelEnum model;
	CameraInterfaceEnum interface;
	uint16_t length;
	uint16_t width;
	const void *peripheral_inst;
} CameraConfig;

typedef struct Camera {
	CameraModelEnum model;
	CameraInterfaceEnum interface;
	uint16_t length;
	uint16_t width;
	const void *peripheral_inst;

	uint8_t *image;
	uint8_t *image_ready;
	uint32_t (*init)(CameraInterfaceEnum interface);
	uint32_t (*isCaptureDone)(CameraInterfaceEnum interface, uint8_t **image);
	uint32_t (*isPowerDown)(CameraInterfaceEnum interface);
	uint32_t (*powerDown)(CameraInterfaceEnum interface, uint8_t enable);
	uint32_t (*setOutputSize)(uint16_t length, uint16_t width);
	uint32_t (*startCapture)(const void *peri, CameraInterfaceEnum interface, uint8_t *buffer);
	uint32_t (*stopCapture)(const void *peri, CameraInterfaceEnum interface);
} Camera;

uint32_t CameraInit(Camera *camera, const CameraConfig *config);
uint32_t CameraIsCaptureDone(Camera *camera);
uint32_t CameraIsPowerDown(Camera *camera, uint8_t *result);
uint32_t CameraPowerDown(Camera *camera, uint8_t enable);
uint32_t CameraSetOutputSize(Camera *camera, uint16_t length, uint16_t width);
uint32_t CameraStartCapture(Camera *camera, uint8_t *image_buffer);
uint32_t CameraStopCapture(Camera *camera);

#ifdef __cplusplus
}
#endif

#endif
