/*
 * r_rmac_phy_target_yt8512h.c
 *
 *  Created on: 2026年3月11日
 *      Author: Eli.Kang
 */


/* Access to peripherals and board defines. */
#include "bsp_api.h"
#include "r_ethercat_phy.h"

#if (ETHER_PHY_CFG_USE_CUSTOM_PHY_LSI_ENABLE)

void ethercat_phy_target_yt8512_initialize(ethercat_phy_instance_ctrl_t * p_instance_ctrl); 
bool ethercat_phy_target_yt8512_is_support_link_partner_ability(ethercat_phy_instance_ctrl_t * p_instance_ctrl,
                                                                 uint32_t                       line_speed_duplex);

void ethercat_phy_target_yt8512_initialize (ethercat_phy_instance_ctrl_t * p_instance_ctrl)  
{
    R_ETHERCAT_PHY_Write(p_instance_ctrl, 0x1e, 0x40C0); // EXT 40C0  LED0 Control
    R_ETHERCAT_PHY_Write(p_instance_ctrl, 0x1f, 0x0030); //LED0 link or Re、TR LED1 ON，link down OFF


    R_ETHERCAT_PHY_Write(p_instance_ctrl, 0x1e, 0x40C3); // EXT 40C3  LED1 Control
    R_ETHERCAT_PHY_Write(p_instance_ctrl, 0x1f, 0x0320); //LED1,light when 100M recieve and transmit,on when link
}                                      /* End of function ethercat_phy_targets_initialize() */

bool ethercat_phy_target_yt8512_is_support_link_partner_ability (ethercat_phy_instance_ctrl_t * p_instance_ctrl,
                                                                  uint32_t                       line_speed_duplex)
{
    FSP_PARAMETER_NOT_USED(p_instance_ctrl);

    return true;
}
#endif


