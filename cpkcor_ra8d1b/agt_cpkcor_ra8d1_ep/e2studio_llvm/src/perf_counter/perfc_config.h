#ifndef __PERFC_CONFIG_H__
#define __PERFC_CONFIG_H__

#if 1
#define __PERFC_CFG_DISABLE_DEFAULT_SYSTICK_PORTING__	0
#define __PERFC_USE_PMU_PORTING__			0
#define __PERFC_CFG_PORTING_INCLUDE__			"perfc_port_default.h"
#else
#define __PERFC_CFG_DISABLE_DEFAULT_SYSTICK_PORTING__	1
#define __PERFC_USE_PMU_PORTING__			1
#define __PERFC_CFG_PORTING_INCLUDE__			"perfc_port_pmu.h"
#endif

#endif
