/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#include <LVGL_thread.h>
#include "lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "lvgl/demos/lv_demos.h"


static uint32_t idle_time_sum;
static uint32_t non_idle_time_sum;
static uint32_t task_switch_timestamp;
static bool idle_task_running;

void lv_freertos_task_switch_in(const char * name)
{
    if(strcmp(name, "IDLE")) idle_task_running = false;
    else idle_task_running = true;

    task_switch_timestamp = lv_tick_get();
}

void lv_freertos_task_switch_out(void)
{
    uint32_t elaps = lv_tick_elaps(task_switch_timestamp);
    if(idle_task_running) idle_time_sum += elaps;
    else non_idle_time_sum += elaps;
}

uint32_t lv_os_get_idle_percent(void)
{
    if(non_idle_time_sum + idle_time_sum == 0) {
        LV_LOG_WARN("Not enough time elapsed to provide idle percentage");
        return 0;
    }

    uint32_t pct = (idle_time_sum * 100) / (idle_time_sum + non_idle_time_sum);

    non_idle_time_sum = 0;
    idle_time_sum = 0;

    return pct;
}


void timer_tick_callback(timer_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    lv_tick_inc(1);
}

void vApplicationMallocFailedHook( void )
{
    __BKPT(0);
}


#define RGB_565_REG    (0x1F << 11)
#define RGB_565_GREEN  (0x3F << 5)
#define RGB_565_BLUE   (0x1F << 0)

extern const unsigned char gImage_renesas_logo[];
/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void LVGL_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    fsp_err_t err;
    uint32_t count;
    uint32_t x , y;
    uint16_t temp_image, temp_imageH, temp_imageL;

    lv_init();

    lv_port_disp_init();

    lv_port_indev_init();

    uint16_t * p_fb = (uint16_t *)&fb_foreground[0][0];


    for( x=0;x<256;x++)
        {
            for( y=0;y<800;y++)
            {
                temp_imageH = (uint16_t)( (gImage_renesas_logo[2*y+(x)*800*2]<<8)&0xFF00 ); //取像素的高8bit
                temp_imageL = (uint16_t)(  gImage_renesas_logo[2*y+1+(x)*800*2]&0x00FF );   //取像素的低8bit
                temp_image = temp_imageH|temp_imageL;
                p_fb[y+x*(800)] = temp_image ; //(uint16_t)( ( (gImage_renesas_logo[2*y]<<8)&0xFF00 ) | (gImage_renesas_logo[2*y+1]&0x00FF)) ;
//               fb_background[0][y+x*(800)] = 0xF000+y;

            }

        }
    R_GLCDC_BufferChange (&g_display0_ctrl, (uint8_t*) p_fb, DISPLAY_FRAME_LAYER_1);

    R_BSP_SoftwareDelay(500,1000);

#if (1 == LV_USE_DEMO_BENCHMARK)
    lv_demo_benchmark();
#endif

#if (1 == LV_USE_DEMO_MUSIC)
    lv_demo_music();
#endif

#if (1 == LV_USE_DEMO_KEYPAD_AND_ENCODER)
    lv_demo_keypad_encoder();

#endif

#if (1 == LV_USE_DEMO_STRESS)
    lv_demo_stress();
#endif

#if (1 == LV_USE_DEMO_WIDGETS && 0 == LV_USE_DEMO_BENCHMARK)
    lv_demo_widgets();
//    lv_example_anim_1();
//    lv_example_anim_timeline_2();
#endif

    err = R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0);
    }

    err = R_GPT_Start(&g_timer0_ctrl);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0);
    }

    /* TODO: add your own code here */
    while (1)
    {
        lv_timer_handler();
        vTaskDelay (1);
    }
}
