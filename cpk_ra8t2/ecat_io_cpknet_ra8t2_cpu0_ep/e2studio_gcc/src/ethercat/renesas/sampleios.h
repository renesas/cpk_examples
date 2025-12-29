/***********************************************************************************************************************
 * Copyright [2020-2022] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics Corporation and/or its affiliates and may only
 * be used with products of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.
 * Renesas products are sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for
 * the selection and use of Renesas products and Renesas assumes no liability.  No license, express or implied, to any
 * intellectual property right is granted by Renesas.  This software is protected under all applicable laws, including
 * copyright laws. Renesas reserves the right to change or discontinue this software and/or this documentation.
 * THE SOFTWARE AND DOCUMENTATION IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND
 * TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY,
 * INCLUDING WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE
 * SOFTWARE OR DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR
 * DOCUMENTATION (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER,
 * INCLUDING, WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY
 * LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * File Name    : sampleios.h
 * Description  : This module has information about the SWs and LEDs on the board.
 **********************************************************************************************************************/

#ifndef SAMPLEIOS_H
#define SAMPLEIOS_H

#include "bsp_api.h"
#if defined(BOARD_RA8T2_RTK)
/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/* Information on how many SWs and what pins they are on. */
typedef struct st_sample_dip_sws
{
    uint16_t         sw_count;        ///< The number of DIP SWs on this board
    uint32_t const (*p_sws);          ///< Pointer to an array of IOPORT pins for controlling DIP SWs
} sample_dip_sws_t;

/* Available user-controllable DIP SWs on this board. 
 * These enums can be can be used to index into the array of DIP SW pins
 * found in the appl_dip_sws_t structure. */
typedef enum e_sample_dipsw
{
    SAMPLE_DIPSW_0 = 0,
    SAMPLE_DIPSW_1 = 1,
    SAMPLE_DIPSW_2 = 2,
    SAMPLE_DIPSW_3 = 3,
    SAMPLE_DIPSW_4 = 4,
    SAMPLE_DIPSW_5 = 5,
    SAMPLE_DIPSW_6 = 6,
    SAMPLE_DIPSW_7 = 7,
} sample_dipsw_t;

/** Information on how many LEDs and what pins they are on. */
typedef struct st_sample_leds
{
    uint16_t         led_count;        ///< The number of LEDs on this board
    uint32_t const (*p_leds);          ///< Pointer to an array of IOPORT pins for controlling LEDs
} sample_leds_t;

/** Available user-controllable LEDs on this board. These enums can be can be used to index into the array of LED pins
 * found in the sample_leds_t structure. */
typedef enum e_sample_led
{
    SAMPLE_LED_RLED0 = 0,
    SAMPLE_LED_RLED1 = 1,
    SAMPLE_LED_RLED2 = 2,
    SAMPLE_LED_RLED3 = 3,
} sample_led_t;
/***********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
extern const sample_leds_t g_sample_leds;
extern const sample_dip_sws_t g_sample_dip_sws;
/***********************************************************************************************************************
 * Public Functions
 **********************************************************************************************************************/
#endif /* defined(BOARD_RA8x2_RTK) */
#endif /* SAMPLEIOS_H */
