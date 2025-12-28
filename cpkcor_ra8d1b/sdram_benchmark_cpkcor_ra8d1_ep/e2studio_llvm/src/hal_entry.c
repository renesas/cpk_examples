/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include "hal_data.h"
#include "board_sdram.h"
#include <stdio.h>
#include "common_utils.h"
FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

#define DCACHE_Enable   0//0
#define DTCM_Used       0


//extern void mpu_direct_config(void);
void DWT_init();
uint32_t DWT_get_count();
void DWT_clean_count();
uint32_t DWT_count_to_us(uint32_t delta_count);

#define DWT_DEM *(uint32_t*)0xE000EDFC

void DWT_init()
{
    DWT->CTRL = 0;
    DWT_DEM |= 1<<24;
    DWT->CYCCNT = 0;
    DWT->CTRL |= 1<<0;
}


uint32_t DWT_get_count()
{
    return DWT->CYCCNT;
}

void DWT_clean_count()
{
    DWT->CYCCNT = 0;
}

uint32_t DWT_count_to_us(uint32_t delta_count)
{
    return delta_count/480;
}


volatile uint32_t DWT_pre_count=0, DWT_post_count=0, time_sdram_access=0;
volatile uint32_t DWT_delta=0;

#define SDRAM_EXAMPLE_DATALEN    4*1024

volatile uint32_t SRAM_write_buff_Cache[SDRAM_EXAMPLE_DATALEN];
volatile uint32_t SRAM_read_buff_Cache[SDRAM_EXAMPLE_DATALEN];
volatile uint32_t SRAM_write_buff_Nocache[SDRAM_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".ram_nocache");
volatile uint32_t SRAM_read_buff_Nocache[SDRAM_EXAMPLE_DATALEN]  BSP_PLACE_IN_SECTION(".ram_nocache");

volatile uint32_t dtcm_write_buffer[SDRAM_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".dtcm_from_flash");
volatile uint32_t dtcm_read_buffer[SDRAM_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".dtcm_from_flash");


volatile uint32_t sdram_cache[SDRAM_EXAMPLE_DATALEN]  BSP_PLACE_IN_SECTION(".sdram");
volatile uint32_t sdram_nocache[SDRAM_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".sdram_nocache");



/*******************************************************************************
 *
 ******************************************************************************/
#define EXAMPLE_SDRAM_START_ADDRESS (0x68000000U)
#define EXAMPLE_DTCM_START_ADDRESS  (0x20000000U)
#define EXAMPLE_SRAM_START_ADDRESS  (0x220B0000U)


uint8_t timer1s_flag = 0;
/* Callback function */




uint32_t sdram_write_count = 0;
/* SRAM<--->SDRAM 读写测试，SRAM cache<-->SDRAM cache, SRAM cache<-->SDRAM Non cache, SRAM Non cache<-->SDRAM Non cache  */
void SDRAMReadWrite32Bit_test(void)
{
       uint32_t index;
       uint32_t datalen = SDRAM_EXAMPLE_DATALEN ;
       uint32_t i = 0;

//       uint32_t *sdram = EXAMPLE_SDRAM_START_ADDRESS;

       APP_PRINT("##############################################################\r\n");
       APP_PRINT("##############################################################\r\n");
       APP_PRINT("##############################################################\r\n");
       APP_PRINT("**********Write to SDRAM! **********\r\n");
       APP_PRINT("**********SRAM cacheable SDRAM cacheable Start! **********\r\n");

       memset((uint32_t *)SRAM_write_buff_Cache, 0 ,datalen);
       memset((uint32_t *)sdram_cache, 0 ,datalen);
       memset((uint32_t *)SRAM_write_buff_Nocache, 0 ,datalen);
       memset((uint32_t *)sdram_nocache, 0 ,datalen);
       for( i=0; i<datalen; i++)
       {
           SRAM_write_buff_Cache[i] = i;
           SRAM_write_buff_Nocache[i] = i;
           dtcm_write_buffer[i] = i;
       }

       DWT_init();
       DWT_clean_count();
       DWT_pre_count = DWT_get_count();

       for (index = 0; index < datalen; index++)
       {

           sdram_cache[index]  = SRAM_write_buff_Cache[index];  //source 为 SRAM
       }

       DWT_post_count = DWT_get_count();
       DWT_delta = DWT_post_count - DWT_pre_count;
       if(DWT_delta==0)
       {
           APP_PRINT("DWT count error! \r\n");
       }
       time_sdram_access = DWT_count_to_us(DWT_delta);

//       APP_PRINT("sdram write DWT count:%d \r\n", DWT_delta);
//       APP_PRINT("sdram write time:%dus \r\n", time_sdram_access);
       APP_PRINT("sram cache sdram cache write speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
       if(time_sdram_access==0)
       {
           while(1){;}
       }

       APP_PRINT("**********SRAM cacheable SDRAM cacheable End! **********\r\n");

       R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);


       APP_PRINT("**********SRAM cacheable SDRAM Non cacheable Start! **********\r\n");

       DWT_init();
       DWT_clean_count();
       DWT_pre_count = DWT_get_count();

       for (index = 0; index < datalen; index++)
       {
           sdram_nocache[index]  = SRAM_write_buff_Cache[index];  //source 为 SRAM
       }


       DWT_post_count = DWT_get_count();
       DWT_delta = DWT_post_count - DWT_pre_count;
       if(DWT_delta==0)
       {
           APP_PRINT("DWT count error! \r\n");
       }
       time_sdram_access = DWT_count_to_us(DWT_delta);

//       APP_PRINT("sdram write DWT count:%d \r\n", DWT_delta);
//       APP_PRINT("sdram write time:%dus \r\n", time_sdram_access);
       APP_PRINT("sram cache sdram non cache write speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
       if(time_sdram_access==0)
       {
           while(1){;}
       }
//       APP_PRINT("SDRAM write %d bytes data finished! \r\n" , SDRAM_EXAMPLE_DATALEN*4);
       APP_PRINT("**********SRAM cacheable SDRAM Non cacheable End!   **********\r\n");

       R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

#if 0
       APP_PRINT("**********SRAM Non acheable SDRAM Non cacheable Start! **********\r\n");
       DWT_init();
       DWT_clean_count();
       DWT_pre_count = DWT_get_count();

       for (index = 0; index < datalen; index++)
       {

           sdram_nocache[index]  = SRAM_write_buff_Nocache[index];  //source 为 SRAM

       }

       DWT_post_count = DWT_get_count();
       DWT_delta = DWT_post_count - DWT_pre_count;
       if(DWT_delta==0)
       {
           APP_PRINT("DWT count error! \r\n");
       }
       time_sdram_access = DWT_count_to_us(DWT_delta);

//       APP_PRINT("sdram write DWT count:%d \r\n", DWT_delta);
//       APP_PRINT("sdram write time:%dus \r\n", time_sdram_access);
       APP_PRINT("sdram non cache write speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
       if(time_sdram_access==0)
       {
           while(1){;}
       }
#endif

       R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);


       APP_PRINT("##############################################################\r\n");
       APP_PRINT("##############################################################\r\n");
       APP_PRINT("##############################################################\r\n");
       APP_PRINT("**********Read from SDRAM! **********\r\n");
       APP_PRINT("**********SRAM cacheable SDRAM cacheable read Start! **********\r\n");
        DWT_init();
        DWT_clean_count();
        DWT_pre_count = DWT_get_count();

        for (index = 0; index < datalen; index++)
        {
            SRAM_read_buff_Cache[index] = sdram_cache[index];  //读SDRAM
        }

        DWT_post_count = DWT_get_count();
        DWT_delta = DWT_post_count - DWT_pre_count;
        if(DWT_delta==0)
        {
         APP_PRINT("DWT count error! \r\n");
        }
        time_sdram_access = DWT_count_to_us(DWT_delta);

//        APP_PRINT("sdram read DWT count:%d \r\n", DWT_delta);
//        APP_PRINT("sdram read time:%dus \r\n", time_sdram_access);
        APP_PRINT("sram cache sdram cache read speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
        if(time_sdram_access==0)
        {
         while(1){;}
        }
        //       APP_PRINT("SDRAM write %d bytes data finished! \r\n" , SDRAM_EXAMPLE_DATALEN*4);
        APP_PRINT("**********SRAM cacheable SDRAM cacheable read End! **********\r\n");

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);


        APP_PRINT("**********SRAM cacheable SDRAM non cacheable read Start! **********\r\n");
        DWT_init();
        DWT_clean_count();
        DWT_pre_count = DWT_get_count();

        for (index = 0; index < datalen; index++)
        {
            SRAM_read_buff_Cache[index] = sdram_nocache[index];  //读SDRAM, SDRAM 放在noncacheable段
        }

        DWT_post_count = DWT_get_count();
        DWT_delta = DWT_post_count - DWT_pre_count;
        if(DWT_delta==0)
        {
         APP_PRINT("DWT count error! \r\n");
        }
        time_sdram_access = DWT_count_to_us(DWT_delta);

//        APP_PRINT("sdram read DWT count:%d \r\n", DWT_delta);
//        APP_PRINT("sdram read time:%dus \r\n", time_sdram_access);
        APP_PRINT("sram cache sdram non cache read speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
        if(time_sdram_access==0)
        {
         while(1){;}
        }
        //       APP_PRINT("SDRAM write %d bytes data finished! \r\n" , SDRAM_EXAMPLE_DATALEN*4);
        APP_PRINT("**********SRAM cacheable SDRAM non cacheable read End!   **********\r\n");

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

#if 0
        APP_PRINT("**********SRAM non cacheable SDRAM non cacheable read Start! **********\r\n");
        DWT_init();
        DWT_clean_count();
        DWT_pre_count = DWT_get_count();

        for (index = 0; index < datalen; index++)
        {
            SRAM_write_buff_Nocache[index] = sdram_nocache[index];  //读SDRAM
        }

        DWT_post_count = DWT_get_count();
        DWT_delta = DWT_post_count - DWT_pre_count;
        if(DWT_delta==0)
        {
         APP_PRINT("DWT count error! \r\n");
        }
        time_sdram_access = DWT_count_to_us(DWT_delta);

//        APP_PRINT("sdram read DWT count:%d \r\n", DWT_delta);
//        APP_PRINT("sdram read time:%dus \r\n", time_sdram_access);
        APP_PRINT("sram non cache sdram non read write speed:%dMB/s \r\n", SDRAM_EXAMPLE_DATALEN*4 / time_sdram_access);
        if(time_sdram_access==0)
        {
         while(1){;}
        }
#endif

}

timer_info_t g_gpt_info;

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    APP_PRINT("SDRAM read/write test start!\r\n");


    R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_GPT_Enable(&g_timer0_ctrl);
    R_GPT_CounterSet(&g_timer0_ctrl, 100);

    R_GPT_Start(&g_timer0_ctrl);
    R_GPT_InfoGet(&g_timer0_ctrl, &g_gpt_info);
    APP_PRINT("gpt info, direction %d\r\n" , g_gpt_info.count_direction);
    APP_PRINT("gpt info, clk source %d\r\n" , g_gpt_info.clock_frequency);
    APP_PRINT("gpt info, count %d\r\n" , g_gpt_info.period_counts);


    /* Initialize SDRAM. */
    bsp_sdram_init();          //See BSP_PRV_SDRAM_BUS_WIDTH to define bus width of SDRAM
    APP_PRINT("gpt info, count %d\r\n" , g_gpt_info.period_counts);
    SDRAMReadWrite32Bit_test();

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        IOPORT_CFG_OPEN (&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
