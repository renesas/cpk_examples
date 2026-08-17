#ifndef __PDM_H
#define __PDM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PDM_ON_LEFT_DATA = 0,
	PDM_ON_RIGHT_DATA,
	PDM_ON_ERROR
} PDM_CallbackEnum;

typedef void (* PDM_CallbackFunc)(void);

uint32_t PDM_Init(void);
PDM_CallbackFunc PDM_RegisterCb(PDM_CallbackFunc cb, PDM_CallbackEnum cbe);
uint32_t PDM_Start(void *ldata, void *rdata, uint32_t length, uint32_t num_to_callback);
uint32_t PDM_Stop(void);

#ifdef __cplusplus
}
#endif

#endif
