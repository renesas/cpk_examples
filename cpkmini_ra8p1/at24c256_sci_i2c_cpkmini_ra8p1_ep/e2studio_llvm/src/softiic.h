#ifndef __SOFTIIC_H
#define __SOFTIIC_H

#include <stdint.h>
#include "hal_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SoftiicSetPinAsInput(group, pin)	{ R_BSP_PinAccessEnable(); R_BSP_PinCfg(pin, IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE); R_BSP_PinAccessDisable(); }
#define SoftiicSetPinAsOD(group, pin)       { R_BSP_PinAccessEnable(); R_BSP_PinCfg(pin, IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_NMOS_ENABLE | IOPORT_CFG_PULLUP_ENABLE); R_BSP_PinAccessDisable(); }
#define SoftiicDelayUs(us)                  R_BSP_SoftwareDelay(us, BSP_DELAY_UNITS_MICROSECONDS)

#define SoftiicSCLHigh(group, pin)			{ R_BSP_PinAccessEnable(); R_BSP_PinWrite(pin, BSP_IO_LEVEL_HIGH); R_BSP_PinAccessDisable(); }
#define SoftiicSCLLow(group, pin)			{ R_BSP_PinAccessEnable(); R_BSP_PinWrite(pin, BSP_IO_LEVEL_LOW); R_BSP_PinAccessDisable(); }
#define SoftiicSDAHigh(group, pin)			{ R_BSP_PinAccessEnable(); R_BSP_PinWrite(pin, BSP_IO_LEVEL_HIGH); R_BSP_PinAccessDisable(); }
#define SoftiicSDALow(group, pin)			{ R_BSP_PinAccessEnable(); R_BSP_PinWrite(pin, BSP_IO_LEVEL_LOW); R_BSP_PinAccessDisable(); }
#define SoftiicGetSDALevel(group, pin)		R_BSP_PinRead(pin)

typedef uint32_t pin_group_t;
typedef bsp_io_port_pin_t pin_num_t;

typedef struct SoftiicType {
    pin_group_t *group_scl;
    pin_num_t pin_scl;
    pin_group_t *group_sda;
    pin_num_t pin_sda;
    uint32_t time;
} SoftiicType;

uint8_t SoftiicWriteArray(SoftiicType *siic, uint8_t slave, uint8_t *array, uint16_t length);

/* SoftiicWriteMem/SoftiicReadMem return values:
 * 0: success
 * 1: device write-address NACK
 * 2: memory-address NACK
 * 3: write-data NACK or repeated-start read-address NACK
 * 4: unsupported memory-address size */
uint8_t SoftiicWriteMem(SoftiicType *siic, uint8_t slave, uint16_t mem_addr, uint8_t mem_size, uint8_t *data, uint16_t length);
uint8_t SoftiicWaitDevice(SoftiicType *siic, uint8_t slave, uint32_t timeout_ms);
uint8_t SoftiicReadArray(SoftiicType *siic, uint8_t slave, uint8_t *array, uint16_t length);
uint8_t SoftiicReadMem(SoftiicType *siic, uint8_t slave, uint16_t mem_addr, uint8_t mem_size, uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
