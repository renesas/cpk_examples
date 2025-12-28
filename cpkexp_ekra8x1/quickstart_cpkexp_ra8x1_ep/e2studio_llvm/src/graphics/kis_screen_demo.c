/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : led_screen_demo.c
 * Version      : .
 * Description  : The led demo screen display.
 *********************************************************************************************************************/

#include <math.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "common_utils.h"
#include "common_init.h"
#include "menu_camview.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "camera_layer.h"
#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "gimp.h"

#include "font_ai_face_digit.h"
#include "bg_font_18_full.h"
#include "lcd.h"

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t   in_transition(void);

void do_kis_screen(void);

#define USE_ALIGNMENT

#ifdef USE_ALIGNMENT

/*COMMON all text aligned */
 #define ALIGNED_Y_OFFSET          (380)
#endif

/* PART_NUMBER */
#define PART_NUMBER_X_OFFSET       (230)

#ifdef USE_ALIGNMENT
 #define PART_NUMBER_Y_OFFSET      (ALIGNED_Y_OFFSET)
#else
 #define PART_NUMBER_Y_OFFSET      (265)
#endif

/* DEVICE_NUMBER */
#define DEVICE_NUMBER_X_OFFSET     (202)
#ifdef USE_ALIGNMENT
 #define DEVICE_NUMBER_Y_OFFSET    (ALIGNED_Y_OFFSET)
#else
 #define DEVICE_NUMBER_Y_OFFSET    (240)
#endif

/* UID */
#define UID_X_OFFSET               (40)
#ifdef USE_ALIGNMENT
 #define UID_Y_OFFSET              (150)
#else
 #define UID_Y_OFFSET              (330)
#endif

/* Temperature */
#define TEMPERATURE_X_OFFSET       (40)
#ifdef USE_ALIGNMENT
 #define TEMPERATURE_Y_OFFSET      (210)
#else
 #define TEMPERATURE_Y_OFFSET      (380)
#endif

/* URL */
#define URL_X_OFFSET               (117)
#ifdef USE_ALIGNMENT
 #define URL_Y_OFFSET              (ALIGNED_Y_OFFSET)
#else
 #define URL_Y_OFFSET              (330)
#endif

static char_t s_temp_buffer[32] = "";
static char_t s_url_buffer[]    = "renesas.com/ra/ek-ra8d1";

// static char_t s_url_buffer[]    = "abcdefghijklmnopqrstuvwxyz0123456789";

/**********************************************************************************************************************
 * Function Name: check_for_temp
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
static void check_for_temp (void)
{
    uint16_t wn_mcu_temp_f = g_board_status.temperature_f.whole_number;
    uint16_t fr_mcu_temp_f = g_board_status.temperature_f.mantissa;
    uint16_t wn_mcu_temp_c = g_board_status.temperature_c.whole_number;
    uint16_t fr_mcu_temp_c = g_board_status.temperature_c.mantissa;
    char_t   fs[16]        = "";
    char_t   cs[16]        = "";

    /* Temprature as degrees F */
    sprintf(fs, "%d.%02d", wn_mcu_temp_f, fr_mcu_temp_f);

    /* Temprature as degrees C */
    sprintf(cs, "%d.%02d", wn_mcu_temp_c, fr_mcu_temp_c);

    /* Fix output as two decimal places */
    size_t len_f = (strcspn(fs, ".") + 3);
    size_t len_c = (strcspn(cs, ".") + 3);

    /* Update temperature to display */
    memset(s_temp_buffer, ' ', 30);
    s_temp_buffer[30] = '\0';
    s_temp_buffer[31] = '\0';

    sprintf(s_temp_buffer, "%.*s'F / %.*s'C       ", len_f, fs, len_c, cs);
}

/**********************************************************************************************************************
 * End of function check_for_temp
 *********************************************************************************************************************/

#define CURRENCY_HB_HEIGHT_SIZE    (26)
#define CURRENCY_HB_WIDTH_SIZE     (70)

#define BLANK_POS_X                (144)
#define BLANK_POS_Y                (300)

/**********************************************************************************************************************
 * Function Name: do_kis_screen
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void do_kis_screen (void)
{
    uint16_t * img = NULL;
    static char_t uid_str[256] = "";

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    check_for_temp();

    if (in_transition()) {
        img = lcd_get_pic_from_flash(PICTURE_INDEX_KIT_INFO);

        d2_setblitsrc(d2_handle, img, 480, 480, LCD_VPIX, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                    480,
                    LCD_VPIX,                     // Source width/height
                    (d2_blitpos) 0,
                    0,                            // Source position
                    (d2_width) ((480) << 4),
                    (d2_width) ((LCD_VPIX) << 4), // Destination width/height
                    0,
                    0,                            // Destination position
                    d2_tm_filter);

        bsp_unique_id_t const * p_uid = R_BSP_UniqueIdGet();

        sprintf(uid_str, "%08x-%08x", p_uid->unique_id_words[0], p_uid->unique_id_words[1]);

        print_bg_font_18(d2_handle, 150, 107, PART_NUMBER);
        print_bg_font_18(d2_handle, 150, 135, DEVICE_NUMBER);
        print_bg_font_18(d2_handle, 150, 163, (char *)uid_str);
        print_bg_font_18(d2_handle, 150, 193, (char *)s_temp_buffer);
        print_bg_font_18(d2_handle, 150, 220, (char *)s_url_buffer);
    }
    else {
        print_bg_font_18(d2_handle, 150, 107, PART_NUMBER);
        print_bg_font_18(d2_handle, 150, 135, DEVICE_NUMBER);
        print_bg_font_18(d2_handle, 150, 163, (char *)uid_str);
        print_bg_font_18(d2_handle, 150, 193, (char *)s_temp_buffer);
        print_bg_font_18(d2_handle, 150, 220, (char *)s_url_buffer);
    }

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}
