/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include "ecat_def.h"
#include "ecatappl.h"
#include "ecatslv.h"
#include "applInterface.h"
#if (CiA402_SAMPLE_APPLICATION == 1)
#include "cia402appl.h"
#else
#include "sampleappl.h"
#endif
#include "renesashw.h"

void R_BSP_WarmStart(bsp_warm_start_event_t event);

/* Locations of bitfields used to configure Peripheral Clocks. */
#define BSP_PRV_PERIPHERAL_CLK_REQ_BIT_POS      (6U)
#define BSP_PRV_PERIPHERAL_CLK_REQ_BIT_MASK     (1U << BSP_PRV_PERIPHERAL_CLK_REQ_BIT_POS)
#define BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS      (7U)
#define BSP_PRV_PERIPHERAL_CLK_RDY_BIT_MASK     (1U << BSP_PRV_PERIPHERAL_CLK_RDY_BIT_POS)

#define BSP_PRV_PRCR_KEY                        (0xA500U)
#define BSP_PRV_PRCR_UNLOCK                     ((BSP_PRV_PRCR_KEY) | 0x3U)
#define BSP_PRV_PRCR_LOCK                       ((BSP_PRV_PRCR_KEY) | 0x0U)

void bsp_test_peripheral_clock_set (volatile uint8_t * p_clk_ctrl_reg,
                                      volatile uint8_t * p_clk_div_reg,
                                      uint8_t            peripheral_clk_div,
                                      uint8_t            peripheral_clk_source);

/*******************************************************************************************************************//**
 * @brief  Blinky example application
 *
 * Blinks all leds at a rate of 1 second using the software delay function provided by the BSP.
 *
 **********************************************************************************************************************/
void hal_entry (void)
{
#if BSP_TZ_SECURE_BUILD

    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
    fsp_err_t err;

//    __disable_irq();

    ethercat_ssc_port_extend_cfg_t * p_ethercat_ssc_port_ext_cfg;
    ether_phy_instance_t * p_ethercat_phy0;
    ether_phy_instance_t * p_ethercat_phy1;
    p_ethercat_ssc_port_ext_cfg = (ethercat_ssc_port_extend_cfg_t *)gp_ethercat_ssc_port->p_cfg->p_extend;
    p_ethercat_phy0 = (ether_phy_instance_t *)p_ethercat_ssc_port_ext_cfg->p_ether_phy_instance[0];
    p_ethercat_phy1= (ether_phy_instance_t *)p_ethercat_ssc_port_ext_cfg->p_ether_phy_instance[1];


    err = RM_ETHERCAT_SSC_PORT_Open(gp_ethercat_ssc_port->p_ctrl, gp_ethercat_ssc_port->p_cfg);
    if(FSP_SUCCESS != err)
    {
        __BKPT(0);
    }

    R_ETHERCAT_PHY_StartAutoNegotiate(p_ethercat_phy0->p_ctrl);
    R_ETHERCAT_PHY_StartAutoNegotiate(p_ethercat_phy1->p_ctrl);

    MainInit();

#if (CiA402_SAMPLE_APPLICATION == 1)
    /* Initialize axis structures */
    CiA402_Init();
#endif

    __enable_irq();
    /* Create basic mapping */
    APPL_GenerateMapping(&nPdInputSize,&nPdOutputSize);
    /* Set stack run flag */
    bRunApplication = TRUE;

    /* Execute the stack */
    while(bRunApplication == TRUE)
    {
        MainLoop();
    }

#if (CiA402_SAMPLE_APPLICATION == 1)
    /* Remove all allocated axes resources */
    CiA402_DeallocateAxis();
#endif

    RM_ETHERCAT_SSC_PORT_Close(gp_ethercat_ssc_port->p_ctrl);

}





