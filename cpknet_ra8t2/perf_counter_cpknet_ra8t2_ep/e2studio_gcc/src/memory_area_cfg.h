#ifndef __MEMORY_AREA_CFG_H
#define __MEMORY_AREA_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __CODE_AREA
#define __CODE_AREA     CODE_MRAM
#endif

#ifndef __DATA_AREA
#define __DATA_AREA     DATA_SRAM
#endif

#ifdef __cplusplus
}
#endif

#endif
