/****************************************************************************
*  Copyright 2024 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*     http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/

/*============================ INCLUDES ======================================*/

#if __PERFC_USE_PMU_PORTING__

#include "cmsis_compiler.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
#ifndef __perfc_sync_barrier__
#   define __perfc_sync_barrier__(...)         do {__DSB();__ISB();} while(0)
#endif


#define __cpu_perf__(__str, ...)                                                \
    using(                                                                      \
        struct {                                                                \
            uint64_t dwNoInstr;                                                 \
            uint64_t dwNoMemAccess;                                             \
            uint64_t dwNoL1DCacheRefill;                                        \
            int64_t lCycles;                                                    \
            uint32_t wInstrCalib;                                               \
            uint32_t wMemAccessCalib;                                           \
            float fCPI;                                                         \
            float fDCacheMissRate;                                              \
        } __PERF_INFO__ = {0},                                                  \
        ({                                                                      \
            __PERF_INFO__.dwNoInstr = perfc_pmu_get_instruction_count();        \
            __PERF_INFO__.dwNoMemAccess = perfc_pmu_get_memory_access_count();  \
            __PERF_INFO__.wInstrCalib = perfc_pmu_get_instruction_count()       \
                                - __PERF_INFO__.dwNoInstr;                      \
            __PERF_INFO__.wMemAccessCalib = perfc_pmu_get_memory_access_count() \
                                          - __PERF_INFO__.dwNoMemAccess;        \
            __PERF_INFO__.dwNoL1DCacheRefill                                    \
                = perfc_pmu_get_L1_dcache_refill_count();                       \
            __PERF_INFO__.dwNoInstr = perfc_pmu_get_instruction_count();        \
            __PERF_INFO__.dwNoMemAccess = perfc_pmu_get_memory_access_count();  \
        }),                                                                     \
        ({                                                                      \
            __PERF_INFO__.dwNoInstr = perfc_pmu_get_instruction_count()         \
                                    - __PERF_INFO__.dwNoInstr                   \
                                    - __PERF_INFO__.wInstrCalib;                \
            __PERF_INFO__.dwNoMemAccess = perfc_pmu_get_memory_access_count()   \
                                        - __PERF_INFO__.dwNoMemAccess           \
                                        - __PERF_INFO__.wMemAccessCalib;        \
            __PERF_INFO__.dwNoL1DCacheRefill                                    \
                = perfc_pmu_get_L1_dcache_refill_count()                        \
                - __PERF_INFO__.dwNoL1DCacheRefill;                             \
                                                                                \
            __PERF_INFO__.fDCacheMissRate                                       \
                        = (float)( (double)__PERF_INFO__.dwNoL1DCacheRefill     \
                                 / (double)__PERF_INFO__.dwNoMemAccess)         \
                        * 100.0f;                                               \
                                                                                \
            __PERF_INFO__.fCPI = (float)(    (double)__PERF_INFO__.lCycles      \
                                       /    (double)__PERF_INFO__.dwNoInstr);   \
            if (__PLOOC_VA_NUM_ARGS(__VA_ARGS__) == 0) {                        \
                __perf_counter_printf__( "\r\n"                                 \
                        "[Report for " __str "]\r\n"                            \
                        "-----------------------------------------\r\n"         \
                        "Instruction executed: %lld\r\n"                        \
                        "Cycle Used: %lld\r\n"                                  \
                        "Cycles per Instructions: %3.3f \r\n\r\n"               \
                        "Memory Access Count: %lld\r\n"                         \
                        "L1 DCache Refill Count: %lld\r\n"                      \
                        "L1 DCache Miss Rate: %3.4f %% \r\n"                    \
                        ,                                                       \
                        __PERF_INFO__.dwNoInstr,                                \
                        __PERF_INFO__.lCycles,                                  \
                        (double)__PERF_INFO__.fCPI,                             \
                        __PERF_INFO__.dwNoMemAccess,                            \
                        __PERF_INFO__.dwNoL1DCacheRefill,                       \
                        (double)__PERF_INFO__.fDCacheMissRate                   \
                        );                                                      \
             } else {                                                           \
                __VA_ARGS__                                                     \
             }                                                                  \
        }))                                                                     \
        __cycleof__("", { __PERF_INFO__.lCycles = __cycle_count__; })


/*============================ TYPES =========================================*/
typedef uint32_t perfc_global_interrupt_status_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
extern
void perfc_port_pmu_insert_to_debug_monitor_handler(void);

extern
uint64_t perfc_pmu_get_instruction_count(void);

extern
uint64_t perfc_pmu_get_memory_access_count(void);

extern
uint64_t perfc_pmu_get_L1_dcache_refill_count(void);

/*============================ PROTOTYPES ====================================*/
/* low level interface for pmu porting */
extern uint32_t perfc_port_get_system_timer_freq(void);
extern int64_t perfc_port_get_system_timer_top(void);
extern bool perfc_port_is_system_timer_ovf_pending(void);
extern bool perfc_port_init_system_timer(bool bTimerOccupied);
extern int64_t perfc_port_get_system_timer_elapsed(void);
extern void perfc_port_clear_system_timer_ovf_pending(void);
extern void perfc_port_stop_system_timer_counting(void);
extern void perfc_port_clear_system_timer_counter(void);

#endif
