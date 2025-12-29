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
 * File Name    : sampleio.c
 * Description  : This module has information about the SWs and LEDs on the board.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "sampleios.h"
#if defined(BOARD_RA8T2_RTK)
/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/

#if defined(BOARD_RA8T2_RTK)
/** Array of DIP SW IOPORT pins. */
static const uint32_t g_sample_prv_dip_sws[] =
{
	(uint32_t) BSP_IO_PORT_11_PIN_07,  // JP1 1-2
	(uint32_t) BSP_IO_PORT_09_PIN_12,  // JP1 3-4
	(uint32_t) BSP_IO_PORT_09_PIN_11,  // JP1 5-6
};
/** Array of LED IOPORT pins. */
static const uint32_t g_sample_prv_leds[] =
{
    (uint32_t) BSP_IO_PORT_02_PIN_07,   ///< LED14
    (uint32_t) BSP_IO_PORT_09_PIN_04,   ///< LED15
    (uint32_t) BSP_IO_PORT_03_PIN_09,   ///< LED16
    (uint32_t) BSP_IO_PORT_09_PIN_13,   ///< LED17
};
#endif

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/

/** Structure with LED information for the board. */

const sample_leds_t g_sample_leds =
{
    .led_count = (uint16_t) ((sizeof(g_sample_prv_leds) / sizeof(g_sample_prv_leds[0]))),
    .p_leds    = g_sample_prv_leds
};

/** Structure with DIP SW information for the board. */
const sample_dip_sws_t g_sample_dip_sws =
{
    .sw_count = (uint16_t) ((sizeof(g_sample_prv_dip_sws) / sizeof(g_sample_prv_dip_sws[0]))),
    .p_sws    = g_sample_prv_dip_sws
};

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/

#endif

