/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include "board_sdram.h"
#include <stdio.h>
FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

#define DCACHE_Enable   0
#define DTCM_Used       1


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


uint32_t DWT_pre_count=0, DWT_post_count=0, time_sdram_access=0;
uint32_t DWT_delta=0;

#define SEMC_EXAMPLE_DATALEN    4096
#define SEMC_EXAMPLE_WRITETIMES (1000U)

uint32_t write_buff[SEMC_EXAMPLE_DATALEN];// BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(".itcm_data");
uint32_t read_buff[SEMC_EXAMPLE_DATALEN];
uint32_t dtcm_read_buff[SEMC_EXAMPLE_DATALEN] BSP_PLACE_IN_SECTION(".dtcm_data");


void SDRAMReadWrite32Bit(void) ;

/*******************************************************************************
 * Variables
 ******************************************************************************/
#define EXAMPLE_SDRAM_START_ADDRESS (0x68000000U)
#define EXAMPLE_DTCM_START_ADDRESS  (0x20000000U)

void SDRAMReadWrite32Bit(void)
{
       uint32_t index;
       uint32_t datalen = SEMC_EXAMPLE_DATALEN ;
       uint32_t *sdram  = (uint32_t *)EXAMPLE_SDRAM_START_ADDRESS; /* SDRAM start address. */
       uint32_t *dtcm_buffer  = (uint32_t *)EXAMPLE_DTCM_START_ADDRESS;

       memset((uint32_t *)dtcm_buffer, 0 ,datalen);
       memset((uint32_t *)write_buff, 0 ,datalen);
       memset((uint32_t *)sdram, 0 ,datalen);
       for(uint16_t i=0; i<datalen; i++)
       {
           write_buff[i] = i;
       }
       for (index = 0; index < datalen; index++)
       {
#if DTCM_Used
        dtcm_buffer[index]  = write_buff[index];  //index;
#else

#endif
       }
#if DCACHE_Enable
       SCB_EnableDCache();
#endif
       DWT_init();
       DWT_clean_count();
       DWT_pre_count = DWT_get_count();

       for (index = 0; index < datalen; index++)
       {
#if DTCM_Used
        sdram[index]  = dtcm_buffer[index];  //source 为 DTCM
#else
        sdram[index]  = write_buff[index];  //source 为 SRAM
#endif
       }
       DWT_post_count = DWT_get_count();
       DWT_delta = DWT_post_count - DWT_pre_count;
       if(DWT_delta==0)
       {
           printf("DWT count error! \r\n");
       }
       time_sdram_access = DWT_count_to_us(DWT_delta);
#if DCACHE_Enable
       SCB_DisableDCache();
#endif

       if(time_sdram_access==0)
       {
           while(1){;}
       }
       printf("SDRAM write %d bytes data finished! \r\n" , SEMC_EXAMPLE_DATALEN);

       R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

       /* Read data from the SDRAM. */
#if DCACHE_Enable
       SCB_EnableDCache();
#endif
       memset((uint32_t *)dtcm_buffer, 0 ,datalen);
       time_sdram_access = 0;
       DWT_init();
       DWT_clean_count();
       DWT_pre_count = DWT_get_count();

       for (index = 0; index < datalen; index++)
       {
#if DTCM_Used
        dtcm_read_buff[index] = sdram[index];
#else
        read_buff[index] = sdram[index];
#endif
       }
       DWT_post_count = DWT_get_count();
       DWT_delta = DWT_post_count - DWT_pre_count;
       while(DWT_delta==0)
       {
       }
       time_sdram_access = DWT_count_to_us(DWT_delta);
       if(time_sdram_access==0)
       {
           while(1){;}
       }

#if DCACHE_Enable
       SCB_DisableDCache();
#endif
       printf("SDRAM read %d bytes data finished! \r\n" , SEMC_EXAMPLE_DATALEN);
       printf("SDRAM read back data: \r\n");
       /* Compare the two buffers. */
       while (datalen--)
       {
           printf("0x%3x  ", sdram[datalen]);
           if(datalen%16==0)
           {
               printf("\r\n");
           }
#if DTCM_Used
        if (dtcm_buffer[datalen] != sdram[datalen])
        {
            printf("SDRAM test error! \r\n");
            while(1){;}
        }
#else
        if (read_buff[datalen] != sdram[datalen])
        {
            printf("SDRAM test error! \r\n");
            while(1){;}
        }
#endif
       }

       printf("SDRAM test pass! \r\n");

}


void SDRAMReadWrite16Bit(void)
{
    uint32_t index;
    uint32_t datalen = SEMC_EXAMPLE_DATALEN ;
    uint16_t *sdram  = (uint16_t *)EXAMPLE_SDRAM_START_ADDRESS; /* SDRAM start address. */
    uint16_t *dtcm_buffer  = (uint16_t *)EXAMPLE_DTCM_START_ADDRESS;


    memset((uint16_t *)dtcm_buffer, 0 ,datalen);
    memset((uint16_t *)write_buff, 0 ,datalen);
    for(uint16_t i=0; i<datalen; i++)
    {
        write_buff[i] = i;
    }

    for (index = 0; index < datalen; index++)
    {
        dtcm_buffer[index]  = (uint16_t)write_buff[index];  //index;
    }

    SCB_EnableDCache();
    DWT_init();
    DWT_pre_count = DWT_get_count();

    for (index = 0; index < datalen; index++)
    {
        sdram[index]  = dtcm_buffer[index];  //index;
    }
    DWT_post_count = DWT_get_count();
    DWT_delta = DWT_post_count - DWT_pre_count;
    while(DWT_delta==0)
    {
    }
    time_sdram_access = DWT_count_to_us(DWT_delta);
    if(time_sdram_access==0)
    {
        while(1){;}
    }

    SCB_DisableDCache();

    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    /* Read data from the SDRAM. */
    SCB_EnableDCache();
    time_sdram_access = 0;
    memset((uint16_t *)dtcm_buffer, 0 ,datalen);
    DWT_init();
    DWT_clean_count();
    DWT_pre_count = DWT_get_count();

    for (index = 0; index < datalen; index++)
    {
        dtcm_buffer[index] = sdram[index];
    }
    DWT_post_count = DWT_get_count();
    DWT_delta = DWT_post_count - DWT_pre_count;
    while(DWT_delta==0)
    {
    }
    time_sdram_access = DWT_count_to_us(DWT_delta);
    if(time_sdram_access==0)
    {
        while(1){;}
    }


    SCB_DisableDCache();

    /* Compare the two buffers. */
    while (datalen--)
    {
        if (dtcm_buffer[datalen] != sdram[datalen])
        {
            while(1){;}
        }
    }


}


/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    fsp_err_t err;
    err = R_SCI_B_UART_Open(&g_uart3_ctrl, &g_uart3_cfg);
    if(FSP_SUCCESS != err)
    {
        while(1);
    }
    printf("SDRAM read/write test start!\r\n");
    /* Initialize SDRAM. */
    bsp_sdram_init();          //See BSP_PRV_SDRAM_BUS_WIDTH to define bus width of SDRAM
    SDRAMReadWrite32Bit();
//    SDRAMReadWrite16Bit();

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
