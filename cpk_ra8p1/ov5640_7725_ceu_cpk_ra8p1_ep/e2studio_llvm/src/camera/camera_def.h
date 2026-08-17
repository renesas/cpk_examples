#ifndef __CAMERA_DEF_H
#define __CAMERA_DEF_H

#ifndef CAMERA_USING_DVP
#define CAMERA_USING_DVP	1
#endif

#ifndef CAMERA_USING_MIPI
#define CAMERA_USING_MIPI	0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CameraModel {
	CAMERA_MODEL_OV5640,
	CAMERA_MODEL_OV7725
} CameraModelEnum;

typedef enum CameraInterface {
	CAMERA_INTERFACE_DVP,
	CAMERA_INTERFACE_MIPI_CSI
} CameraInterfaceEnum;

#ifdef __cplusplus
}
#endif

#endif
