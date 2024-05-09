/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include "arm_mve.h"
#include "stdio.h"
#include "stdlib.h"
#include "dwt.h"
#include "pic.h"
FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

#define HELIUM_BASIC_DEMO
#define HELIUM_RGB_DEINTERLEAVE_DEMO

#ifdef HELIUM_BASIC_DEMO
#undef HELIUM_RGB_DEINTERLEAVE_DEMO
#endif




volatile uint32_t t_start=0;
volatile uint32_t t_end=0;
volatile uint32_t t_diff=0;

volatile uint32_t t_start_mve=0;
volatile uint32_t t_end_mve=0;
volatile uint32_t t_diff_mve=0;


#ifdef HELIUM_BASIC_DEMO
void helium_basic_example(void)
{
    uint8_t *a_ptr = malloc(sizeof(uint8_t) * 256);
    uint8_t *b_ptr = malloc(sizeof(uint8_t) * 256);
    uint8_t *c_ptr = malloc(sizeof(uint8_t) * 256);
    uint8_t *d_ptr = malloc(sizeof(uint8_t) * 256);


    for(uint16_t i = 0 ; i < 256 ; i++)
    {
        a_ptr[i] = (uint8_t)i;
        b_ptr[i] = (uint8_t)(i + 1);
    }

    uint8x16_t rega_8, regb_8, regc_8, regd_8;
    for(uint16_t i = 0 ; i < 256 ; i+=16)
    {
        rega_8 = vld1q_u8(&a_ptr[i]);
        regb_8 = vld1q_u8(&b_ptr[i]);
        regc_8 = vaddq_u8(rega_8, regb_8);
        vst1q_u8(&c_ptr[i], regc_8);
    }



    for(uint16_t i = 0 ; i < 256 ; i += 16)
    {
        rega_8 = vld1q_u8(&a_ptr[i]);
        regb_8 = vld1q_u8(&b_ptr[i]);
        regd_8 = vsubq_u8(regb_8, rega_8);
        vst1q_u8(&d_ptr[i], regd_8);
    }


    uint32_t *a_32_ptr = malloc(sizeof(uint32_t) * 256);
    uint32_t *b_32_ptr = malloc(sizeof(uint32_t) * 256);
    for(uint16_t i = 0 ; i < 256 ; i++)
    {
        a_32_ptr[i] = i;
    }

    uint32x4_t rega_32, regb_32;
    for(int i = 0 ; i < 256 ; i += 4)
    {
        rega_32 = vld1q_u32(&a_32_ptr[i]);
        regb_32 = vmulq_n_u32(rega_32, 3);
        vst1q_u32(&b_32_ptr[i], regb_32);
    }

    printf("a_ptr =\r\n");
    for(uint16_t i = 0 ; i < 16 ; i++)
    {
        for(uint16_t j = 0 ; j < 16 ; j++)
        {
            printf("%d  ",a_ptr[j + 16 * i] );
        }
        printf("\r\n");
    }
    printf("\n");

    printf("b_ptr =\r\n");
    for(uint16_t i = 0 ; i < 16 ; i++)
    {
        for(uint16_t j = 0 ; j < 16 ; j++)
        {
            printf("%d  ",b_ptr[j + 16 * i] );
        }
        printf("\r\n");
    }
    printf("\n");

    printf("vadd  c_ptr = a_ptr + b_ptr result:\r\n");
    for(uint16_t i = 0 ; i < 16 ; i++)
    {
        for(uint16_t j = 0 ; j < 16 ; j++)
        {
            printf("%d  ",c_ptr[j + 16 * i] );
        }
        printf("\r\n");
    }
    printf("\n");

    printf("vsub  d_ptr = b_ptr- a_ptr result:\r\n");
    for(uint16_t i = 0 ; i < 16 ; i++)
    {
        for(uint16_t j = 0 ; j < 16 ; j++)
        {
            printf("%d  ",d_ptr[j + 16 * i] );
        }
        printf("\r\n");
    }
    printf("\n");


    printf("vmulq_n b_32_ptr = a_32_ptr * 3 result:\r\n");
    for(uint16_t i = 0 ; i < 16 ; i++)
    {
        for(uint16_t j = 0 ; j < 16 ; j++)
        {
            printf("%d  ",b_32_ptr[j + 16 * i] );
        }
        printf("\r\n");
    }
    printf("\n");

}

#endif


#ifdef HELIUM_RGB_DEINTERLEAVE_DEMO
uint8_t   rgba_r[240*160];
uint8_t   rgba_g[240*160];
uint8_t   rgba_b[240*160];
uint8_t   rgba_a[240*160];

uint8_t   rgba_r_mve[240*160];
uint8_t   rgba_g_mve[240*160];
uint8_t   rgba_b_mve[240*160];
uint8_t   rgba_a_mve[240*160];

void rgba_deinterleave(uint8_t *r,uint8_t *g,uint8_t *b, uint8_t *rgb,uint32_t height, uint32_t width)
{
    uint32_t index=0;
    uint8_t* pixelPtr;
    for(uint32_t i=0;i<height;i++)
    {
        for(uint32_t j=0;j<width;j++)
        {
            index = j + i * width;
            pixelPtr = rgb + index*4;

            b[index] = *(pixelPtr + 2);
            g[index] = *(pixelPtr + 1);
            r[index] = *(pixelPtr);
        }
    }
}


void rgba_deinterleave_mve(uint8_t *r,uint8_t *g,uint8_t *b, uint8_t *rgb,uint32_t PixelCnt)
{
    int num8x16 = PixelCnt / 64;

    for (int i=0; i < num8x16; i++)
    {
        uint8x16x4_t intlv_rgba = vld4q_u8(rgb+4*16*i);
        vst1q_u8(r+16*i, intlv_rgba.val[0]);
        vst1q_u8(g+16*i, intlv_rgba.val[1]);
        vst1q_u8(b+16*i, intlv_rgba.val[2]);
    }
}

uint8_t result_cmp(uint8_t *p0,uint8_t *p1,uint32_t len)
{
    for(uint32_t i = 0; i<len;i++)
    {
        if(*(p0+i) != *(p1+i))
        {
            return 0;
        }
    }
    return 1;
}



#endif

void helium_test(void)
{
#ifdef HELIUM_BASIC_DEMO
    helium_basic_example();
#endif

#ifdef HELIUM_RGB_DEINTERLEAVE_DEMO

    DWT_init();
    DWT_Reset();
    t_start = DWT_TS_GET();

    rgba_deinterleave(&rgba_r[0],&rgba_g[0],&rgba_b[0], &gImage_dog_240_160[0],160,240);

    t_end = DWT_TS_GET();
    t_diff = t_end - t_start;

    DWT_Reset();
    t_start_mve = DWT_TS_GET();


    rgba_deinterleave_mve(&rgba_r_mve[0],&rgba_g_mve[0],&rgba_b_mve[0], &gImage_dog_240_160[0],240*160*4);

    t_end_mve = DWT_TS_GET();
    t_diff_mve = t_end_mve - t_start_mve;

    if(result_cmp(&rgba_r[0],&rgba_r_mve[0],sizeof(rgba_r)/sizeof(rgba_r[0])))
    {
        printf("\r\nred color match.");
    }
    if(result_cmp(&rgba_g[0],&rgba_g_mve[0],sizeof(rgba_g)/sizeof(rgba_g[0])))
    {
        printf("\r\ngreen color match.");
    }
    if(result_cmp(&rgba_b[0],&rgba_b_mve[0],sizeof(rgba_b)/sizeof(rgba_b[0])))
    {
        printf("\r\nblue color match.");
    }
    printf("\r\nwithout helium running cycles:%d,with helium running cycles:%d.\r\n",t_diff,t_diff_mve);
    if(t_diff_mve<t_diff)
    {
        printf("code with helium win!\r\n");
    }
    else
    {
        printf("code without helium win!\r\n");
    }
#endif
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    fsp_err_t   err;

    err = R_SCI_B_UART_Open(&g_uart3_ctrl,&g_uart3_cfg);
    if(FSP_SUCCESS != err)
    {
        while(1);
    }

    helium_test();
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
        R_IOPORT_Open (&g_ioport_ctrl, &IOPORT_CFG_NAME);
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
