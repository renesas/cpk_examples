#ifndef __MEMORY_AREA_H
#define __MEMORY_AREA_H

#include "memory_area_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CODE_MRAM
#undef CODE_MRAM
#endif

#ifdef CODE_ITCM
#undef CODE_ITCM
#endif

#ifdef CODE_SRAM
#undef CODE_SRAM
#endif

#ifdef CODE_SDRAM
#undef CODE_SDRAM
#endif

#ifdef CODE_OSPI0_CS0
#undef CODE_OSPI0_CS0
#endif

#ifdef CODE_OSPI0_CS1
#undef CODE_OSPI0_CS1
#endif

#ifdef CODE_OSPI1_CS0
#undef CODE_OSPI1_CS0
#endif

#ifdef CODE_OSPI1_CS1
#undef CODE_OSPI1_CS1
#endif

#ifdef DATA_SRAM
#undef DATA_SRAM
#endif

#ifdef DATA_DTCM
#undef DATA_DTCM
#endif

#ifdef DATA_SDRAM
#undef DATA_SDRAM
#endif

#ifdef DATA_OSPI0_CS0
#undef DATA_OSPI0_CS0
#endif

#ifdef DATA_OSPI0_CS1
#undef DATA_OSPI0_CS1
#endif

#ifdef DATA_OSPI1_CS0
#undef DATA_OSPI1_CS0
#endif

#ifdef DATA_OSPI1_CS1
#undef DATA_OSPI1_CS1
#endif

#define CODE_MRAM           0x00
#define CODE_SRAM           0x01
#define CODE_ITCM           0x02
#define CODE_OSPI1_CS1      0x04
#define CODE_OSPI1_CS0      0x05
#define CODE_OSPI0_CS1      0x06
#define CODE_OSPI0_CS0      0x07
#define CODE_SDRAM          0x08

#define DATA_SRAM           CODE_SRAM
#define DATA_DTCM           0x03
#define DATA_SDRAM          CODE_SDRAM
#define DATA_OSPI0_CS0      CODE_OSPI0_CS0
#define DATA_OSPI0_CS1      CODE_OSPI0_CS1
#define DATA_OSPI1_CS0      CODE_OSPI1_CS0
#define DATA_OSPI1_CS1      CODE_OSPI1_CS1

#if !defined(__CODE_AREA) || (__CODE_AREA == CODE_MRAM)
#define CODE_AREA
#elif __CODE_AREA == CODE_ITCM
#define CODE_AREA		__attribute__((section(".itcm_code_from_flash")))
#elif __CODE_AREA == CODE_SRAM
#define CODE_AREA		__attribute__((section(".ram_code_from_flash")))
#elif __CODE_AREA == CODE_SDRAM
#define CODE_AREA		__attribute__((section(".sdram_code_from_flash")))
#elif __CODE_AREA == CODE_OSPI0_CS0
#define CODE_AREA		__attribute__((section(".ospi0_cs0_code_from_flash")))
#elif __CODE_AREA == CODE_OSPI0_CS1
#define CODE_AREA       __attribute__((section(".ospi0_cs1_code")))
#elif __CODE_AREA == CODE_OSPI1_CS0
#define CODE_AREA		__attribute__((section(".ospi1_cs0_code_from_flash")))
#elif __CODE_AREA == CODE_OSPI1_CS1
#define CODE_AREA       __attribute__((section(".ospi1_cs1_code")))
#else
#error "Assign __CODE_AREA to an error area"
#define CODE_AREA
#endif

#if !defined(__DATA_AREA) || (__DATA_AREA == DATA_SRAM)
#define DATA_AREA_BSS
#define DATA_AREA_DATA
#define DATA_AREA_ZERO
#elif __DATA_AREA == DATA_DTCM
#define DATA_AREA_BSS	__attribute__((section(".dtcm_noinit")))
#define DATA_AREA_DATA	__attribute__((section(".dtcm_from_flash")))
#define DATA_AREA_ZERO	__attribute__((section(".dtcm")))
#elif __DATA_AREA == DATA_SDRAM
#define DATA_AREA_BSS	__attribute__((section(".sdram_noinit")))
#define DATA_AREA_DATA	__attribute__((section(".sdram_from_flash")))
#define DATA_AREA_ZERO	__attribute__((section(".sdram")))
#elif __DATA_AREA == DATA_OSPI0_CS0
#define DATA_AREA_BSS   __attrubute__((section(".ospi0_cs0_noinit")))
#define DATA_AREA_DATA  __attribute__((section(".ospi0_cs0_from_flash")))
#define DATA_AREA_ZERO  __attribute__((section(".ospi0_cs0")))
#elif __DATA_AREA == DATA_OSPI0_CS1
#define DATA_AREA_BSS   __attrubute__((section(".ospi0_cs1_noinit")))
#define DATA_AREA_DATA  __attribute__((section(".ospi0_cs1_from_flash")))
#define DATA_AREA_ZERO  __attribute__((section(".ospi0_cs1")))
#elif __DATA_AREA == CODE_OSPI1_CS0
#define DATA_AREA_BSS   __attrubute__((section(".ospi1_cs0_noinit")))
#define DATA_AREA_DATA  __attribute__((section(".ospi1_cs0_from_flash")))
#define DATA_AREA_ZERO  __attribute__((section(".ospi1_cs0")))
#elif __DATA_AREA == DATA_OSPI1_CS1
#define DATA_AREA_BSS   __attrubute__((section(".ospi1_cs1_noinit")))
#define DATA_AREA_DATA  __attribute__((section(".ospi1_cs1_from_flash")))
#define DATA_AREA_ZERO  __attribute__((section(".ospi1_cs1")))
#else
#error "Assign __DATA_AREA to an error area"
#define DATA_AREA_BSS
#define DATA_AREA_DATA
#define DATA_AREA_ZERO
#endif

#ifdef __cplusplus
}
#endif

#endif
