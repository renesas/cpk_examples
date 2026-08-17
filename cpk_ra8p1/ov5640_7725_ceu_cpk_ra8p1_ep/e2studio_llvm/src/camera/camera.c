#include "camera.h"
#include "hal_data.h"
#include "ov5640.h"
#include "ov7725.h"

#ifndef __CAMERA_DEBUG
#define __CAMERA_DEBUG	1
#endif

#if __CAMERA_DEBUG
#include "utils/log.h"
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }
#else
#define LIKE_RETURN(v, t, msg, ...)		if (v == t) { return v; }
#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { return v; }
#endif

uint32_t CameraInit(Camera *camera, const CameraConfig *config)
{
	uint32_t err = FSP_ERR_UNSUPPORTED;

	if ((camera == NULL) || (config == NULL)) {
		return FSP_ERR_INVALID_POINTER;
	}
#if CAMERA_USING_DVP == 0
	if (config->interface == CAMERA_INTERFACE_DVP) {
		return FSP_ERR_UNSUPPORTED;
	}
#endif
#if CAMERA_USING_MIPI == 0
	if (config->interface == CAMERA_INTERFACE_MIPI_CSI) {
		return FSP_ERR_UNSUPPORTED;
	}
#endif

	camera->model = config->model;
	camera->interface = config->interface;
	camera->length = config->length;
	camera->width = config->width;
	camera->peripheral_inst = config->peripheral_inst;

	if (camera->model == CAMERA_MODEL_OV7725) {
		if (camera->interface == CAMERA_INTERFACE_MIPI_CSI) {
			return err;
		}
		camera->init = OV7725_Init;
		camera->isCaptureDone = OV7725_IsImageReady;
		camera->isPowerDown = OV7725_IsPowerDown;
		camera->powerDown = OV7725_PowerDown;
		camera->setOutputSize = OV7725_SetOutputSize;
		camera->startCapture = OV7725_StartCapture;
		camera->stopCapture = OV7725_StopCapture;
	}
	else if (camera->model == CAMERA_MODEL_OV5640) {
		camera->init = OV5640_Init;
		camera->isCaptureDone = OV5640_IsImageReady;
		camera->isPowerDown = OV5640_IsPowerDown;
		camera->powerDown = OV5640_PowerDown;
		camera->setOutputSize = OV5640_SetOutputSize;
		camera->startCapture = OV5640_StartCapture;
		camera->stopCapture = OV5640_StopCapture;
	}
	else {
		return err;
	}

	__DSB();
	__ISB();
	camera->image = NULL;
	camera->image_ready = NULL;
	err = camera->init(camera->interface);
	UNLIKE_RETURN(err, 0, "init failed: %u", err);
	err = camera->setOutputSize(camera->length, camera->width);
	UNLIKE_RETURN(err, 0, "setOutputSize failed: %u", err);

	return err;
}

uint32_t CameraIsCaptureDone(Camera *camera)
{
	uint32_t ret;

	if (camera == NULL) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->isCaptureDone == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	camera->image_ready = NULL;
	ret = camera->isCaptureDone(camera->interface, &camera->image_ready);
	/* 外设本身没有接收缓冲，使用的是 CameraStartCapture() 传递的接收缓冲 */
	if (ret && (camera->image_ready == NULL)) {
		camera->image_ready = camera->image;
	}

	return ret;
}

uint32_t CameraIsPowerDown(Camera *camera, uint8_t *result)
{
	if ((camera == NULL) || (result == NULL)) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->isPowerDown == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	if (camera->isPowerDown(camera->interface)) {
		*result = 1;
	}
	else {
		*result = 0;
	}

	return 0;
}

uint32_t CameraPowerDown(Camera *camera, uint8_t enable)
{
	if (camera == NULL) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->powerDown == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	if (camera->isPowerDown != NULL) {
		if (camera->isPowerDown(camera->interface) != enable) {
			camera->powerDown(camera->interface, enable);
		}
	}
	else {
		camera->powerDown(camera->interface, enable);
	}

	return 0;
}

uint32_t CameraSetOutputSize(Camera *camera, uint16_t length, uint16_t width)
{
	uint32_t err = FSP_ERR_UNSUPPORTED;

	if (camera == NULL) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->setOutputSize == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	err = camera->setOutputSize(length, width);
	UNLIKE_RETURN(err, 0, "setOutputSize failed: %u", err);
	camera->length = length;
	camera->width = width;

	return err;
}

uint32_t CameraStartCapture(Camera *camera, uint8_t *image_buffer)
{
	uint32_t err = FSP_ERR_UNSUPPORTED;

	if (camera == NULL) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->startCapture == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	err = camera->startCapture(camera->peripheral_inst, camera->interface, image_buffer);
	UNLIKE_RETURN(err, 0, "startCapture failed: %u", err);
	camera->image = image_buffer;

	return err;
}

uint32_t CameraStopCapture(Camera *camera)
{
	uint32_t err;

	if (camera == NULL) {
		return FSP_ERR_INVALID_POINTER;
	}
	if (camera->stopCapture == NULL) {
		return FSP_ERR_UNSUPPORTED;
	}

	err = camera->stopCapture(camera->peripheral_inst, camera->interface);

	return err;
}
