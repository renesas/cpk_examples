#ifndef __ES8311_H
#define __ES8311_H

#include <stdbool.h>
#include <stdint.h>

#define ES8311_ADDRESS	0x18

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ES8311_ON_IDLE = 0,
	ES8311_ON_RECEIVE_DONE,
	ES8311_ON_TRANSMIT_DONE
} ES8311_CallbackEnum;

typedef void (* ES8311_CallbackFunc)(void);

void ES8311_Adc2DacLoopback(bool enable);
void ES8311_DumpAdcRegs(void);
uint32_t ES8311_Init(void);
ES8311_CallbackFunc ES8311_RegisterCb(ES8311_CallbackFunc cb, ES8311_CallbackEnum cbe);
uint32_t ES8311_Receive(void *data, uint32_t length);
void ES8311_SetVolume(uint8_t percent);
uint32_t ES8311_Transmit(void *data, uint32_t length);
uint32_t ES8311_WriteRead(void *tx_data, void *rx_data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
