#ifndef __HYPER_RAM_H
#define __HYPER_RAM_H

#include <stdint.h>

#define HYPER_RAM_MAP_START_ADDR	0x70000000
#define HYPER_RAM_MAP_END_ADDR		0x78000000
#define HYPER_RAM_SIZE				(1024 * 1024 * 32)

#ifdef __cplusplus
extern "C" {
#endif

void HyperRAM_Init(void);

#ifdef __cplusplus
}
#endif

#endif
