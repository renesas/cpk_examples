#include "app_thread.h"
#include "console.h"

#include "FreeRTOS_ARP.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"

#include "perf_counter/perf_counter.h"
#include "SEGGER_RTT/SEGGER_RTT.h"
#include "utils/log.h"

#define ARP_RESOLUTION_RETRY_MS			(1000U)
#define ARP_RESOLUTION_TIMEOUT_MS		(1500U)
#define DHCP_HOSTNAME                	"cpkcor-ra8p1"
#define DHCP_POLL_INTERVAL_MS        	(100U)
#define DHCP_STATUS_LOG_INTERVAL_MS  	(5000U)
#define ETHERNET_RESET_ASSERT_MS   		(10U)
#define ETHERNET_RESET_PIN         		BSP_IO_PORT_10_PIN_07
#define ETHERNET_RESET_RELEASE_MS  		(100U)
#define LOG_TAG                    		__FUNCTION__
#define PHY_AUTONEG_POLL_INTERVAL_MS 	(200U)
#define PHY_AUTONEG_TIMEOUT_MS       	(25000U)
#define PHY_AUTONEG_TRACE_ENABLE     	(0)
#define PHY_AUTONEG_TRACE_HEARTBEAT_MS 	(1000U)
#define PHY_ANER_LOCAL_NP_ABLE_MASK     (1U << 2)
#define PHY_ANER_PAGE_RECEIVED_MASK     (1U << 1)
#define PHY_ANER_PARALLEL_FAULT_MASK    (1U << 4)
#define PHY_ANER_PARTNER_AN_ABLE_MASK   (1U << 0)
#define PHY_ANER_PARTNER_NP_ABLE_MASK   (1U << 3)
#define PHY_BMCR_AN_ENABLE_MASK     	(1U << 12)
#define PHY_BMCR_AN_RESTART_MASK    	(1U << 9)
#define PHY_BMCR_RESET_MASK         	(1U << 15)
#define PHY_BMSR_AN_COMPLETE_MASK   	(1U << 5)
#define PHY_BMSR_LINK_STATUS_MASK   	(1U << 2)
#define PHY_DESTRUCTIVE_RESET_TEST_ENABLE (0)
#define PHY_DRIVER_READY_TIMEOUT_MS (5000U)
#define PHY_GBCR_1000_FULL_MASK     (1U << 9)
#define PHY_GBCR_1000_HALF_MASK     (1U << 8)
#define PHY_GBESR_1000_FULL_MASK    (1U << 13)
#define PHY_GBSR_1000_FULL_MASK     (1U << 11)
#define PHY_GBSR_1000_HALF_MASK     (1U << 10)
#define PHY_GBSR_IDLE_ERROR_MASK    (0xFFU)
#define PHY_GBSR_LOCAL_RX_OK_MASK   (1U << 13)
#define PHY_GBSR_MASTER_MASK        (1U << 14)
#define PHY_GBSR_MS_FAULT_MASK      (1U << 15)
#define PHY_GBSR_REMOTE_RX_OK_MASK  (1U << 12)
#define PHY_GREEN_ETHERNET_ADDRESS_VALUE (0x8011U)
#define PHY_GREEN_ETHERNET_DISABLE_ENABLE (0)
#define PHY_GREEN_ETHERNET_DISABLE_VALUE (0x573FU)
#define PHY_INVALID_REGISTER_VALUE  (0xFFFFU)
#define PHY_PAGE_SELECT_MASK        (0x0FFFU)
#define PHY_PAGE_SELECT_RETRY_COUNT (3U)
#define PHY_PHYSR_DUPLEX_MASK       (1U << 3)
#define PHY_PHYSR_LINK_MASK         (1U << 2)
#define PHY_PHYSR_MASTER_MASK       (1U << 11)
#define PHY_PHYSR_SPEED_MASK        (3U << 4)
#define PHY_PHYSR_SPEED_POS         (4U)
#define PHY_REG_AN_ADVERTISEMENT    (0x04U)
#define PHY_REG_AN_EXPANSION        (0x06U)
#define PHY_REG_AN_LINK_PARTNER     (0x05U)
#define PHY_REG_BASIC_CONTROL       (0x00U)
#define PHY_REG_BASIC_STATUS        (0x01U)
#define PHY_REG_GIGABIT_CONTROL     (0x09U)
#define PHY_REG_GIGABIT_EXT_STATUS  (0x0FU)
#define PHY_REG_GIGABIT_STATUS      (0x0AU)
#define PHY_REG_GREEN_ETHERNET_ADDRESS (0x1BU)
#define PHY_REG_GREEN_ETHERNET_DATA    (0x1CU)
#define PHY_REG_ID1                 (0x02U)
#define PHY_REG_ID2                 (0x03U)
#define PHY_REG_PAGE_SELECT         (0x1FU)
#define PHY_REG_PHYSR               (0x1AU)
#define PHY_RESET_POLL_INTERVAL_MS  (10U)
#define PHY_RESET_TIMEOUT_MS        (1000U)
#define PHY_RTL8211F_PHYSR_PAGE     (0x0A43U)
#define PHY_RTL8211F_STANDARD_PAGE  (0x0A42U)
#define PING_DATA_LENGTH_BYTES     (32U)
#define PING_DESTINATION_IP        "223.5.5.5"
#define PING_PERIOD_MS             (1000U)
#define PING_REPLY_TIMEOUT_MS      (1000U)
#define PING_SEND_BLOCK_MS         (100U)

static TaskHandle_t                s_ping_task_handle;
static volatile uint16_t           s_ping_reply_identifier;
static volatile ePingReplyStatus_t s_ping_reply_status;

#if PHY_GREEN_ETHERNET_DISABLE_ENABLE
static bool disable_phy_green_ethernet_and_restart_autoneg(void);
#endif
static fsp_err_t ethernet_phy_reset(void);
#if PHY_AUTONEG_TRACE_ENABLE || PHY_DESTRUCTIVE_RESET_TEST_ENABLE
static void log_phy_auto_negotiation_sample(uint32_t elapsed_ms,
                                            uint32_t bmsr_latched,
                                            uint32_t bmsr_current,
                                            uint32_t aner,
                                            uint32_t gbcr,
                                            uint32_t gbsr,
                                            uint32_t physr,
                                            uint32_t standard_page,
                                            uint32_t physr_page,
                                            uint32_t restored_page);
#endif
static void log_phy_register(const char *p_name, uint32_t address, fsp_err_t err, uint32_t value);
static void log_phy_registers(void);
#if PHY_AUTONEG_TRACE_ENABLE || PHY_DESTRUCTIVE_RESET_TEST_ENABLE || PHY_GREEN_ETHERNET_DISABLE_ENABLE
static bool phy_select_page(uint32_t page, uint32_t *p_readback, fsp_err_t *p_access_err);
#endif
#if PHY_DESTRUCTIVE_RESET_TEST_ENABLE
static bool run_phy_gigabit_advertisement_test(void);
#endif
#if PHY_AUTONEG_TRACE_ENABLE || PHY_DESTRUCTIVE_RESET_TEST_ENABLE
static bool wait_for_phy_auto_negotiation(void);
#endif
static bool wait_for_gateway_arp(void);
static uint32_t wait_for_dhcp_address(void);
static bool wait_for_ping_reply(uint16_t expected_identifier,
                                TickType_t send_time,
                                TickType_t *p_elapsed_time,
                                ePingReplyStatus_t *p_reply_status);

void app_thread_entry(void *pvParameters)
{
    static const uint8_t dns_server_address[4] = {0U, 0U, 0U, 0U};
    static const uint8_t gateway_address[4]    = {0U, 0U, 0U, 0U};
    static const uint8_t ip_address[4]         = {0U, 0U, 0U, 0U};
    static const uint8_t mac_address[6]        = {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U};
    static const uint8_t netmask[4]            = {0U, 0U, 0U, 0U};
#if PHY_DESTRUCTIVE_RESET_TEST_ENABLE
    bool phy_gigabit_test_done                 = false;
#endif

    FSP_PARAMETER_NOT_USED(pvParameters);

    perfc_init(false);
#if CONSOLE_CFG_USE_RTT == 0
	SEGGER_RTT_Init();
#endif
	CONSOLE_Init();

	/* Clean screen */
#if CONSOLE_CFG_USE_RTT
    puts(RTT_CTRL_RESET);
    puts(RTT_CTRL_CLEAR);
#else
    puts("\x1b[2J\x1b[H");
    puts("\x1B[0m");
#endif

    fsp_err_t err = ethernet_phy_reset();
    if (FSP_SUCCESS != err)
    {
        LOG_E(LOG_TAG, "Ethernet PHY reset failed: 0x%08X", (unsigned int) err);
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }
    LOG_I(LOG_TAG, "Ethernet PHY reset released");

    s_ping_task_handle = xTaskGetCurrentTaskHandle();

    LOG_I(LOG_TAG, "Starting network with DHCP");
    if (pdPASS != FreeRTOS_IPInit(ip_address, netmask, gateway_address, dns_server_address, mac_address))
    {
        LOG_E(LOG_TAG, "FreeRTOS_IPInit failed");
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }
    LOG_I(LOG_TAG, "FreeRTOS_IPInit completed");
#if PHY_GREEN_ETHERNET_DISABLE_ENABLE
    if (!disable_phy_green_ethernet_and_restart_autoneg())
    {
        LOG_E(LOG_TAG, "Green Ethernet disable test was not applied");
    }
#endif
#if PHY_AUTONEG_TRACE_ENABLE
    LOG_I(LOG_TAG, "Tracing initial PHY auto-negotiation");
    (void) wait_for_phy_auto_negotiation();
#endif

    while (true)
    {
        TickType_t last_wake_time;
        uint32_t destination_address;

        (void) wait_for_dhcp_address();
#if PHY_DESTRUCTIVE_RESET_TEST_ENABLE
        if (!phy_gigabit_test_done)
        {
            phy_gigabit_test_done = true;
            if (run_phy_gigabit_advertisement_test())
            {
                (void) wait_for_phy_auto_negotiation();
            }
        }
#endif
        log_phy_registers();
        if (!wait_for_gateway_arp())
        {
            LOG_W(LOG_TAG, "Network down while resolving gateway ARP");
            continue;
        }

        destination_address = FreeRTOS_inet_addr(PING_DESTINATION_IP);
        last_wake_time       = xTaskGetTickCount();

        while (pdTRUE == FreeRTOS_IsNetworkUp())
        {
            BaseType_t ping_identifier;
            ePingReplyStatus_t reply_status;
            TickType_t elapsed_time = 0U;
            TickType_t send_time;

            (void) ulTaskNotifyTake(pdTRUE, 0U);
            send_time       = xTaskGetTickCount();
            ping_identifier = FreeRTOS_SendPingRequest(destination_address,
                                                       PING_DATA_LENGTH_BYTES,
                                                       pdMS_TO_TICKS(PING_SEND_BLOCK_MS));

            if (pdFAIL == ping_identifier)
            {
                LOG_E(LOG_TAG, "PING %s: send failed", PING_DESTINATION_IP);
            }
            else if (wait_for_ping_reply((uint16_t) ping_identifier,
                                         send_time,
                                         &elapsed_time,
                                         &reply_status))
            {
                if (eSuccess == reply_status)
                {
                    LOG_I(LOG_TAG,
                          "PING %s: reply, seq=%u, time=%lu ms",
                          PING_DESTINATION_IP,
                          (unsigned int) ping_identifier,
                          (unsigned long) pdTICKS_TO_MS(elapsed_time));
                }
                else if (eInvalidChecksum == reply_status)
                {
                    LOG_W(LOG_TAG,
                          "PING %s: invalid checksum, seq=%u",
                          PING_DESTINATION_IP,
                          (unsigned int) ping_identifier);
                }
                else
                {
                    LOG_W(LOG_TAG,
                          "PING %s: invalid data, seq=%u",
                          PING_DESTINATION_IP,
                          (unsigned int) ping_identifier);
                }
            }
            else
            {
                LOG_W(LOG_TAG,
                      "PING %s: timeout, seq=%u",
                      PING_DESTINATION_IP,
                      (unsigned int) ping_identifier);
            }

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PING_PERIOD_MS));
        }

        LOG_W(LOG_TAG, "Network down; waiting for DHCP");
    }
}

static fsp_err_t ethernet_phy_reset(void)
{
    fsp_err_t err;

    err = R_IOPORT_PinCfg(&g_ioport_ctrl,
                          ETHERNET_RESET_PIN,
                          (uint32_t) IOPORT_CFG_DRIVE_HIGH |
                          (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                          (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(ETHERNET_RESET_ASSERT_MS));

    err = R_IOPORT_PinWrite(&g_ioport_ctrl, ETHERNET_RESET_PIN, BSP_IO_LEVEL_HIGH);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(ETHERNET_RESET_RELEASE_MS));

    return FSP_SUCCESS;
}

static void log_phy_register(const char *p_name, uint32_t address, fsp_err_t err, uint32_t value)
{
    if (FSP_SUCCESS == err)
    {
        LOG_I(LOG_TAG,
              "PHY %s [0x%02X] = 0x%04X",
              p_name,
              (unsigned int) address,
              (unsigned int) value);
    }
    else
    {
        LOG_E(LOG_TAG,
              "PHY %s [0x%02X] read failed: 0x%08X",
              p_name,
              (unsigned int) address,
              (unsigned int) err);
    }
}

static void log_phy_registers(void)
{
    fsp_err_t anar_err;
    fsp_err_t anlpar_err;
    fsp_err_t bmcr_err;
    fsp_err_t bmsr_current_err;
    fsp_err_t bmsr_latched_err;
    fsp_err_t gbcr_err;
    fsp_err_t gbsr_err;
    fsp_err_t id1_err;
    fsp_err_t id2_err;
    fsp_err_t page_read_err;
    fsp_err_t page_restore_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_select_err  = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t physr_err        = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t port_select_err;
    uint32_t anar          = 0U;
    uint32_t anlpar        = 0U;
    uint32_t bmcr          = 0U;
    uint32_t bmsr_current  = 0U;
    uint32_t bmsr_latched  = 0U;
    uint32_t gbcr          = 0U;
    uint32_t gbsr          = 0U;
    uint32_t id1           = 0U;
    uint32_t id2           = 0U;
    uint32_t original_page = 0U;
    uint32_t physr         = 0U;

    vTaskSuspendAll();

    port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl, (uint8_t) g_rmac_phy0.p_cfg->channel);
    if (FSP_SUCCESS == port_select_err)
    {
        bmcr_err         = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_CONTROL, &bmcr);
        bmsr_latched_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_STATUS, &bmsr_latched);
        bmsr_current_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_STATUS, &bmsr_current);
        id1_err          = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_ID1, &id1);
        id2_err          = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_ID2, &id2);
        anar_err         = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_AN_ADVERTISEMENT, &anar);
        anlpar_err       = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_AN_LINK_PARTNER, &anlpar);
        gbcr_err         = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_GIGABIT_CONTROL, &gbcr);
        gbsr_err         = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_GIGABIT_STATUS, &gbsr);

        page_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_PAGE_SELECT, &original_page);
        if (FSP_SUCCESS == page_read_err)
        {
            page_select_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                               PHY_REG_PAGE_SELECT,
                                               PHY_RTL8211F_PHYSR_PAGE);
            if (FSP_SUCCESS == page_select_err)
            {
                physr_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_PHYSR, &physr);
                page_restore_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                                    PHY_REG_PAGE_SELECT,
                                                    original_page);
            }
        }
    }

    (void) xTaskResumeAll();

    if (FSP_SUCCESS != port_select_err)
    {
        LOG_E(LOG_TAG, "PHY port select failed: 0x%08X", (unsigned int) port_select_err);
        return;
    }

    LOG_I(LOG_TAG, "PHY register dump begin");
    log_phy_register("BMCR", PHY_REG_BASIC_CONTROL, bmcr_err, bmcr);
    log_phy_register("BMSR(latched)", PHY_REG_BASIC_STATUS, bmsr_latched_err, bmsr_latched);
    log_phy_register("BMSR(current)", PHY_REG_BASIC_STATUS, bmsr_current_err, bmsr_current);
    log_phy_register("PHYID1", PHY_REG_ID1, id1_err, id1);
    log_phy_register("PHYID2", PHY_REG_ID2, id2_err, id2);
    log_phy_register("ANAR", PHY_REG_AN_ADVERTISEMENT, anar_err, anar);
    log_phy_register("ANLPAR", PHY_REG_AN_LINK_PARTNER, anlpar_err, anlpar);
    log_phy_register("GBCR", PHY_REG_GIGABIT_CONTROL, gbcr_err, gbcr);
    log_phy_register("GBSR", PHY_REG_GIGABIT_STATUS, gbsr_err, gbsr);

    if (FSP_SUCCESS != page_read_err)
    {
        LOG_E(LOG_TAG, "PHY PAGSR [0x1F] read failed: 0x%08X", (unsigned int) page_read_err);
    }
    else if (FSP_SUCCESS != page_select_err)
    {
        LOG_E(LOG_TAG, "PHY page 0xA43 select failed: 0x%08X", (unsigned int) page_select_err);
    }
    else
    {
        log_phy_register("PHYSR(page 0xA43)", PHY_REG_PHYSR, physr_err, physr);
        if (FSP_SUCCESS != page_restore_err)
        {
            LOG_E(LOG_TAG,
                  "PHY page 0x%03X restore failed: 0x%08X",
                  (unsigned int) original_page,
                  (unsigned int) page_restore_err);
        }
    }

    if ((FSP_SUCCESS == bmcr_err) &&
        (FSP_SUCCESS == bmsr_current_err) &&
        (FSP_SUCCESS == gbcr_err) &&
        (FSP_SUCCESS == gbsr_err))
    {
        LOG_I(LOG_TAG,
              "PHY negotiation: enabled=%u, complete=%u, link=%u",
              (unsigned int) (0U != (bmcr & PHY_BMCR_AN_ENABLE_MASK)),
              (unsigned int) (0U != (bmsr_current & PHY_BMSR_AN_COMPLETE_MASK)),
              (unsigned int) (0U != (bmsr_current & PHY_BMSR_LINK_STATUS_MASK)));
        LOG_I(LOG_TAG,
              "PHY 1000Base-T: local full=%u half=%u, partner full=%u half=%u",
              (unsigned int) (0U != (gbcr & PHY_GBCR_1000_FULL_MASK)),
              (unsigned int) (0U != (gbcr & PHY_GBCR_1000_HALF_MASK)),
              (unsigned int) (0U != (gbsr & PHY_GBSR_1000_FULL_MASK)),
              (unsigned int) (0U != (gbsr & PHY_GBSR_1000_HALF_MASK)));
    }

    if (FSP_SUCCESS == physr_err)
    {
        static const char * const speed_names[4] =
        {
            "10 Mbps",
            "100 Mbps",
            "1000 Mbps",
            "reserved"
        };
        uint32_t speed_index = (physr & PHY_PHYSR_SPEED_MASK) >> PHY_PHYSR_SPEED_POS;

        LOG_I(LOG_TAG,
              "PHY resolved link: link=%u, speed=%s, duplex=%s",
              (unsigned int) (0U != (physr & PHY_PHYSR_LINK_MASK)),
              speed_names[speed_index],
              (0U != (physr & PHY_PHYSR_DUPLEX_MASK)) ? "full" : "half");
    }

    LOG_I(LOG_TAG, "PHY register dump end");
}

#if PHY_DESTRUCTIVE_RESET_TEST_ENABLE
static bool run_phy_gigabit_advertisement_test(void)
{
    fsp_err_t bmcr_read_err              = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t bmcr_write_err             = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t gbcr_read_err              = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t gbcr_readback_err          = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t gbcr_write_err             = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t gbesr_read_err             = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_read_after_reset_err  = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_read_before_reset_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_readback_err          = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_write_after_reset_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_write_before_reset_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t port_select_err;
    fsp_err_t reset_write_err            = FSP_ERR_NOT_INITIALIZED;
    uint32_t bmcr                        = 0U;
    uint32_t gbcr_readback               = 0U;
    uint32_t gbcr_reset_default          = 0U;
    uint32_t gbesr                       = 0U;
    uint32_t page_after_reset            = 0U;
    uint32_t page_before_reset           = 0U;
    uint32_t page_readback               = 0U;
    uint32_t restart_value               = 0U;
    TickType_t reset_start_time;

    vTaskSuspendAll();

    port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl, (uint8_t) g_rmac_phy0.p_cfg->channel);
    if (FSP_SUCCESS == port_select_err)
    {
        page_read_before_reset_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                     PHY_REG_PAGE_SELECT,
                                                     &page_before_reset);
        if (FSP_SUCCESS == page_read_before_reset_err)
        {
            page_write_before_reset_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                                           PHY_REG_PAGE_SELECT,
                                                           PHY_RTL8211F_STANDARD_PAGE);
        }
        if (FSP_SUCCESS == page_write_before_reset_err)
        {
            reset_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                               PHY_REG_BASIC_CONTROL,
                                               PHY_BMCR_RESET_MASK);
        }
    }

    (void) xTaskResumeAll();

    if (FSP_SUCCESS != port_select_err)
    {
        LOG_E(LOG_TAG, "PHY reset test: port select failed: 0x%08X", (unsigned int) port_select_err);
        return false;
    }
    if (FSP_SUCCESS != page_read_before_reset_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: PAGSR before reset read failed: 0x%08X",
              (unsigned int) page_read_before_reset_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: PAGSR before reset=0x%04X",
          (unsigned int) page_before_reset);

    if (FSP_SUCCESS != page_write_before_reset_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: standard page select before reset failed: 0x%08X",
              (unsigned int) page_write_before_reset_err);
        return false;
    }
    if (FSP_SUCCESS != reset_write_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: BMCR reset write failed: 0x%08X",
              (unsigned int) reset_write_err);
        return false;
    }

    LOG_I(LOG_TAG, "PHY reset test: BMCR write=0x%04X", (unsigned int) PHY_BMCR_RESET_MASK);

    reset_start_time = xTaskGetTickCount();
    while (true)
    {
        fsp_err_t poll_port_select_err;

        vTaskDelay(pdMS_TO_TICKS(PHY_RESET_POLL_INTERVAL_MS));
        vTaskSuspendAll();

        poll_port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl,
                                                     (uint8_t) g_rmac_phy0.p_cfg->channel);
        if (FSP_SUCCESS == poll_port_select_err)
        {
            bmcr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_CONTROL, &bmcr);
        }

        (void) xTaskResumeAll();

        if (FSP_SUCCESS != poll_port_select_err)
        {
            LOG_E(LOG_TAG,
                  "PHY reset test: port select while waiting failed: 0x%08X",
                  (unsigned int) poll_port_select_err);
            return false;
        }
        if (FSP_SUCCESS != bmcr_read_err)
        {
            LOG_E(LOG_TAG,
                  "PHY reset test: BMCR read while waiting failed: 0x%08X",
                  (unsigned int) bmcr_read_err);
            return false;
        }

        if ((0xFFFFU != (bmcr & 0xFFFFU)) && (0U == (bmcr & PHY_BMCR_RESET_MASK)))
        {
            break;
        }

        if (pdMS_TO_TICKS(PHY_RESET_TIMEOUT_MS) <= (xTaskGetTickCount() - reset_start_time))
        {
            LOG_E(LOG_TAG,
                  "PHY reset test: timeout after %u ms, BMCR=0x%04X",
                  (unsigned int) PHY_RESET_TIMEOUT_MS,
                  (unsigned int) bmcr);
            return false;
        }
    }

    LOG_I(LOG_TAG,
          "PHY reset test: completed in %lu ms, BMCR=0x%04X",
          (unsigned long) pdTICKS_TO_MS(xTaskGetTickCount() - reset_start_time),
          (unsigned int) bmcr);

    bmcr_read_err = FSP_ERR_NOT_INITIALIZED;
    vTaskSuspendAll();

    port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl, (uint8_t) g_rmac_phy0.p_cfg->channel);
    if (FSP_SUCCESS == port_select_err)
    {
        page_read_after_reset_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                    PHY_REG_PAGE_SELECT,
                                                    &page_after_reset);
        if (FSP_SUCCESS == page_read_after_reset_err)
        {
            page_write_after_reset_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                                          PHY_REG_PAGE_SELECT,
                                                          PHY_RTL8211F_STANDARD_PAGE);
        }
        if (FSP_SUCCESS == page_write_after_reset_err)
        {
            page_readback_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                PHY_REG_PAGE_SELECT,
                                                &page_readback);
        }
        if ((FSP_SUCCESS == page_readback_err) &&
            (PHY_RTL8211F_STANDARD_PAGE == (page_readback & PHY_PAGE_SELECT_MASK)))
        {
            gbesr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                             PHY_REG_GIGABIT_EXT_STATUS,
                                             &gbesr);
            gbcr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                            PHY_REG_GIGABIT_CONTROL,
                                            &gbcr_reset_default);
        }
        if (FSP_SUCCESS == gbcr_read_err)
        {
            gbcr_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                              PHY_REG_GIGABIT_CONTROL,
                                              PHY_GBCR_1000_FULL_MASK);
        }
        if (FSP_SUCCESS == gbcr_write_err)
        {
            gbcr_readback_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                PHY_REG_GIGABIT_CONTROL,
                                                &gbcr_readback);
        }
        if ((FSP_SUCCESS == gbcr_readback_err) &&
            (0U != (gbcr_readback & PHY_GBCR_1000_FULL_MASK)))
        {
            bmcr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_CONTROL, &bmcr);
        }
        if (FSP_SUCCESS == bmcr_read_err)
        {
            restart_value = bmcr | PHY_BMCR_AN_ENABLE_MASK | PHY_BMCR_AN_RESTART_MASK;
            bmcr_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                              PHY_REG_BASIC_CONTROL,
                                              restart_value);
        }
    }

    (void) xTaskResumeAll();

    if (FSP_SUCCESS != port_select_err)
    {
        LOG_E(LOG_TAG, "PHY reset test: port select after reset failed: 0x%08X", (unsigned int) port_select_err);
        return false;
    }

    if (FSP_SUCCESS != page_read_after_reset_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: PAGSR after reset read failed: 0x%08X",
              (unsigned int) page_read_after_reset_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: PAGSR after reset=0x%04X",
          (unsigned int) page_after_reset);

    if (FSP_SUCCESS != page_write_after_reset_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: PAGSR write failed: 0x%08X",
              (unsigned int) page_write_after_reset_err);
        return false;
    }
    if (FSP_SUCCESS != page_readback_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: PAGSR readback failed: 0x%08X",
              (unsigned int) page_readback_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: PAGSR write=0x%04X, readback=0x%04X",
          (unsigned int) PHY_RTL8211F_STANDARD_PAGE,
          (unsigned int) page_readback);

    if (PHY_RTL8211F_STANDARD_PAGE != (page_readback & PHY_PAGE_SELECT_MASK))
    {
        LOG_E(LOG_TAG, "PHY reset test: IEEE standard register page was not selected");
        return false;
    }

    if (FSP_SUCCESS == gbesr_read_err)
    {
        LOG_I(LOG_TAG,
              "PHY reset test: GBESR after reset=0x%04X, 1000Base-T full capability=%u",
              (unsigned int) gbesr,
              (unsigned int) (0U != (gbesr & PHY_GBESR_1000_FULL_MASK)));
    }
    else
    {
        LOG_E(LOG_TAG,
              "PHY reset test: GBESR after reset read failed: 0x%08X",
              (unsigned int) gbesr_read_err);
    }

    if (FSP_SUCCESS != gbcr_read_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: GBCR default read failed: 0x%08X",
              (unsigned int) gbcr_read_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: GBCR reset default=0x%04X",
          (unsigned int) gbcr_reset_default);

    if (FSP_SUCCESS != gbcr_write_err)
    {
        LOG_E(LOG_TAG, "PHY reset test: GBCR write failed: 0x%08X", (unsigned int) gbcr_write_err);
        return false;
    }
    if (FSP_SUCCESS != gbcr_readback_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: GBCR readback failed: 0x%08X",
              (unsigned int) gbcr_readback_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: GBCR write=0x%04X, readback=0x%04X",
          (unsigned int) PHY_GBCR_1000_FULL_MASK,
          (unsigned int) gbcr_readback);

    if (0U == (gbcr_readback & PHY_GBCR_1000_FULL_MASK))
    {
        LOG_E(LOG_TAG, "PHY reset test: 1000Base-T full-duplex advertisement was not retained");
        return false;
    }
    if (FSP_SUCCESS != bmcr_read_err)
    {
        LOG_E(LOG_TAG, "PHY reset test: BMCR read failed: 0x%08X", (unsigned int) bmcr_read_err);
        return false;
    }
    if (FSP_SUCCESS != bmcr_write_err)
    {
        LOG_E(LOG_TAG,
              "PHY reset test: auto-negotiation restart failed: 0x%08X",
              (unsigned int) bmcr_write_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "PHY reset test: auto-negotiation restarted, BMCR write=0x%04X",
          (unsigned int) restart_value);

    return true;
}
#endif

#if PHY_AUTONEG_TRACE_ENABLE || PHY_DESTRUCTIVE_RESET_TEST_ENABLE || PHY_GREEN_ETHERNET_DISABLE_ENABLE
static bool phy_select_page(uint32_t page, uint32_t *p_readback, fsp_err_t *p_access_err)
{
    for (uint32_t attempt = 0U; attempt < PHY_PAGE_SELECT_RETRY_COUNT; attempt++)
    {
        *p_access_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl, PHY_REG_PAGE_SELECT, page);
        if (FSP_SUCCESS != *p_access_err)
        {
            return false;
        }

        *p_access_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_PAGE_SELECT, p_readback);
        if (FSP_SUCCESS != *p_access_err)
        {
            return false;
        }

        if ((page & PHY_PAGE_SELECT_MASK) == (*p_readback & PHY_PAGE_SELECT_MASK))
        {
            return true;
        }
    }

    return false;
}
#endif

#if PHY_GREEN_ETHERNET_DISABLE_ENABLE
static bool disable_phy_green_ethernet_and_restart_autoneg(void)
{
    const TickType_t timeout = pdMS_TO_TICKS(PHY_DRIVER_READY_TIMEOUT_MS);
    TickType_t start_time    = xTaskGetTickCount();
    fsp_err_t bmcr_read_err  = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t bmcr_write_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t gbcr_read_err  = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t green_address_write_err = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t green_data_write_err    = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t green_page_access_err   = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t page_restore_err        = FSP_ERR_NOT_INITIALIZED;
    fsp_err_t port_select_err;
    uint32_t bmcr                     = 0U;
    uint32_t bmcr_restart             = 0U;
    uint32_t gbcr                     = 0U;
    uint32_t green_page_readback      = 0U;
    uint32_t restored_page_readback   = 0U;
    bool green_page_selected          = false;
    bool page_restored                = false;

    while (0U == g_ether0_ctrl.open)
    {
        if ((xTaskGetTickCount() - start_time) >= timeout)
        {
            LOG_E(LOG_TAG,
                  "Timed out waiting %u ms for RMAC initialization",
                  (unsigned int) PHY_DRIVER_READY_TIMEOUT_MS);
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(PHY_AUTONEG_POLL_INTERVAL_MS));
    }

    vTaskSuspendAll();

    port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl, (uint8_t) g_rmac_phy0.p_cfg->channel);
    if (FSP_SUCCESS == port_select_err)
    {
        green_page_selected = phy_select_page(PHY_RTL8211F_PHYSR_PAGE,
                                              &green_page_readback,
                                              &green_page_access_err);
        if (green_page_selected)
        {
            green_address_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                                       PHY_REG_GREEN_ETHERNET_ADDRESS,
                                                       PHY_GREEN_ETHERNET_ADDRESS_VALUE);
        }
        if (FSP_SUCCESS == green_address_write_err)
        {
            green_data_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                                    PHY_REG_GREEN_ETHERNET_DATA,
                                                    PHY_GREEN_ETHERNET_DISABLE_VALUE);
        }

        page_restored = phy_select_page(PHY_RTL8211F_STANDARD_PAGE,
                                        &restored_page_readback,
                                        &page_restore_err);
        if (page_restored && (FSP_SUCCESS == green_data_write_err))
        {
            gbcr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_GIGABIT_CONTROL, &gbcr);
            bmcr_read_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_BASIC_CONTROL, &bmcr);
        }
        if ((FSP_SUCCESS == bmcr_read_err) &&
            (PHY_INVALID_REGISTER_VALUE != (bmcr & 0xFFFFU)))
        {
            bmcr_restart   = bmcr | PHY_BMCR_AN_ENABLE_MASK | PHY_BMCR_AN_RESTART_MASK;
            bmcr_write_err = R_RMAC_PHY_Write(g_rmac_phy0.p_ctrl,
                                              PHY_REG_BASIC_CONTROL,
                                              bmcr_restart);
        }
    }

    (void) xTaskResumeAll();

    if (FSP_SUCCESS != port_select_err)
    {
        LOG_E(LOG_TAG, "PHY port select failed: 0x%08X", (unsigned int) port_select_err);
        return false;
    }
    if (FSP_SUCCESS != page_restore_err)
    {
        LOG_E(LOG_TAG, "Standard page restore access failed: 0x%08X", (unsigned int) page_restore_err);
        return false;
    }
    if (!page_restored)
    {
        LOG_E(LOG_TAG,
              "Standard page restore was not retained: PAGSR=0x%04X",
              (unsigned int) restored_page_readback);
        return false;
    }
    if (FSP_SUCCESS != green_page_access_err)
    {
        LOG_E(LOG_TAG, "Green Ethernet page access failed: 0x%08X", (unsigned int) green_page_access_err);
        return false;
    }
    if (!green_page_selected)
    {
        LOG_E(LOG_TAG,
              "Green Ethernet page was not retained: PAGSR=0x%04X",
              (unsigned int) green_page_readback);
        return false;
    }
    if (FSP_SUCCESS != green_address_write_err)
    {
        LOG_E(LOG_TAG,
              "Green Ethernet address write failed: 0x%08X",
              (unsigned int) green_address_write_err);
        return false;
    }
    if (FSP_SUCCESS != green_data_write_err)
    {
        LOG_E(LOG_TAG,
              "Green Ethernet disable value write failed: 0x%08X",
              (unsigned int) green_data_write_err);
        return false;
    }
    if (FSP_SUCCESS != gbcr_read_err)
    {
        LOG_E(LOG_TAG, "GBCR read before restart failed: 0x%08X", (unsigned int) gbcr_read_err);
        return false;
    }
    if (PHY_INVALID_REGISTER_VALUE == (gbcr & 0xFFFFU))
    {
        LOG_E(LOG_TAG, "GBCR read before restart was invalid: 0x%04X", (unsigned int) gbcr);
        return false;
    }
    if (FSP_SUCCESS != bmcr_read_err)
    {
        LOG_E(LOG_TAG, "BMCR read before restart failed: 0x%08X", (unsigned int) bmcr_read_err);
        return false;
    }
    if (PHY_INVALID_REGISTER_VALUE == (bmcr & 0xFFFFU))
    {
        LOG_E(LOG_TAG, "BMCR read before restart was invalid: 0x%04X", (unsigned int) bmcr);
        return false;
    }
    if (FSP_SUCCESS != bmcr_write_err)
    {
        LOG_E(LOG_TAG, "Auto-negotiation restart failed: 0x%08X", (unsigned int) bmcr_write_err);
        return false;
    }

    LOG_I(LOG_TAG,
          "Green Ethernet disabled: PAGSR=0x%03X, Reg27=0x%04X, Reg28=0x%04X, restored=0x%03X",
          (unsigned int) (green_page_readback & PHY_PAGE_SELECT_MASK),
          (unsigned int) PHY_GREEN_ETHERNET_ADDRESS_VALUE,
          (unsigned int) PHY_GREEN_ETHERNET_DISABLE_VALUE,
          (unsigned int) (restored_page_readback & PHY_PAGE_SELECT_MASK));
    LOG_I(LOG_TAG,
          "PHY auto-negotiation restarted: BMCR=0x%04X->0x%04X, GBCR=0x%04X",
          (unsigned int) bmcr,
          (unsigned int) bmcr_restart,
          (unsigned int) gbcr);

    return true;
}
#endif

#if PHY_AUTONEG_TRACE_ENABLE || PHY_DESTRUCTIVE_RESET_TEST_ENABLE
static void log_phy_auto_negotiation_sample(uint32_t elapsed_ms,
                                            uint32_t bmsr_latched,
                                            uint32_t bmsr_current,
                                            uint32_t aner,
                                            uint32_t gbcr,
                                            uint32_t gbsr,
                                            uint32_t physr,
                                            uint32_t standard_page,
                                            uint32_t physr_page,
                                            uint32_t restored_page)
{
    static const char * const speed_names[] = {"10 Mbps", "100 Mbps", "1000 Mbps", "reserved"};
    uint32_t speed_index = (physr & PHY_PHYSR_SPEED_MASK) >> PHY_PHYSR_SPEED_POS;

    LOG_I(LOG_TAG,
          "PHY AN trace t=%lu ms: BMSR=%04X/%04X, ANER=%04X, GBCR=%04X, GBSR=%04X, "
          "PHYSR=%04X, PAGSR=%03X/%03X/%03X",
          (unsigned long) elapsed_ms,
          (unsigned int) bmsr_latched,
          (unsigned int) bmsr_current,
          (unsigned int) aner,
          (unsigned int) gbcr,
          (unsigned int) gbsr,
          (unsigned int) physr,
          (unsigned int) (standard_page & PHY_PAGE_SELECT_MASK),
          (unsigned int) (physr_page & PHY_PAGE_SELECT_MASK),
          (unsigned int) (restored_page & PHY_PAGE_SELECT_MASK));
    LOG_I(LOG_TAG,
          "PHY AN state: complete=%u, link(bmsr/physr)=%u/%u, page=%u, partnerAN=%u, "
          "nextPage=%u/%u, parallelFault=%u, local1000=%u/%u, partner1000=%u/%u, "
          "msFault=%u, role(gbsr/physr)=%s/%s, rx=%u/%u, idleErrors=%u, resolved=%s %s",
          (unsigned int) (0U != (bmsr_current & PHY_BMSR_AN_COMPLETE_MASK)),
          (unsigned int) (0U != (bmsr_current & PHY_BMSR_LINK_STATUS_MASK)),
          (unsigned int) (0U != (physr & PHY_PHYSR_LINK_MASK)),
          (unsigned int) (0U != (aner & PHY_ANER_PAGE_RECEIVED_MASK)),
          (unsigned int) (0U != (aner & PHY_ANER_PARTNER_AN_ABLE_MASK)),
          (unsigned int) (0U != (aner & PHY_ANER_LOCAL_NP_ABLE_MASK)),
          (unsigned int) (0U != (aner & PHY_ANER_PARTNER_NP_ABLE_MASK)),
          (unsigned int) (0U != (aner & PHY_ANER_PARALLEL_FAULT_MASK)),
          (unsigned int) (0U != (gbcr & PHY_GBCR_1000_FULL_MASK)),
          (unsigned int) (0U != (gbcr & PHY_GBCR_1000_HALF_MASK)),
          (unsigned int) (0U != (gbsr & PHY_GBSR_1000_FULL_MASK)),
          (unsigned int) (0U != (gbsr & PHY_GBSR_1000_HALF_MASK)),
          (unsigned int) (0U != (gbsr & PHY_GBSR_MS_FAULT_MASK)),
          (0U != (gbsr & PHY_GBSR_MASTER_MASK)) ? "master" : "slave",
          (0U != (physr & PHY_PHYSR_MASTER_MASK)) ? "master" : "slave",
          (unsigned int) (0U != (gbsr & PHY_GBSR_LOCAL_RX_OK_MASK)),
          (unsigned int) (0U != (gbsr & PHY_GBSR_REMOTE_RX_OK_MASK)),
          (unsigned int) (gbsr & PHY_GBSR_IDLE_ERROR_MASK),
          speed_names[speed_index],
          (0U != (physr & PHY_PHYSR_DUPLEX_MASK)) ? "full" : "half");
}

static bool wait_for_phy_auto_negotiation(void)
{
    const TickType_t heartbeat_interval = pdMS_TO_TICKS(PHY_AUTONEG_TRACE_HEARTBEAT_MS);
    const TickType_t poll_interval      = pdMS_TO_TICKS(PHY_AUTONEG_POLL_INTERVAL_MS);
    const TickType_t timeout            = pdMS_TO_TICKS(PHY_AUTONEG_TIMEOUT_MS);
    TickType_t last_log_time            = xTaskGetTickCount();
    TickType_t start_time               = last_log_time;
    uint32_t aner                       = 0U;
    uint32_t bmsr_current               = 0U;
    uint32_t bmsr_latched               = 0U;
    uint32_t gbcr                       = 0U;
    uint32_t gbsr                       = 0U;
    uint32_t physr                      = 0U;
    uint32_t previous_aner              = 0xFFFFFFFFU;
    uint32_t previous_bmsr_current      = 0xFFFFFFFFU;
    uint32_t previous_bmsr_latched      = 0xFFFFFFFFU;
    uint32_t previous_gbcr              = 0xFFFFFFFFU;
    uint32_t previous_gbsr              = 0xFFFFFFFFU;
    uint32_t previous_physr             = 0xFFFFFFFFU;

    vTaskDelay(poll_interval);

    while ((xTaskGetTickCount() - start_time) < timeout)
    {
        fsp_err_t aner_err                 = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t bmsr_current_err         = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t bmsr_latched_err         = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t gbcr_err                 = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t gbsr_err                 = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t page_physr_select_err    = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t page_restore_err         = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t page_standard_select_err = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t physr_err                = FSP_ERR_NOT_INITIALIZED;
        fsp_err_t port_select_err;
        uint32_t page_physr_readback        = 0U;
        uint32_t page_restore_readback      = 0U;
        uint32_t page_standard_readback     = 0U;
        TickType_t now;
        bool invalid_sample         = false;
        bool page_physr_selected    = false;
        bool page_restored          = false;
        bool page_standard_selected = false;
        bool state_changed;

        /* R_RMAC_Open marks this open only after the switch has initialized the PHY and started auto-negotiation. */
        if (0U == g_ether0_ctrl.open)
        {
            now = xTaskGetTickCount();
            if ((now - last_log_time) >= heartbeat_interval)
            {
                LOG_I(LOG_TAG,
                      "PHY AN trace: waiting for RMAC initialization, elapsed=%lu ms",
                      (unsigned long) pdTICKS_TO_MS(now - start_time));
                last_log_time = now;
            }

            vTaskDelay(poll_interval);
            continue;
        }

        vTaskSuspendAll();

        port_select_err = R_RMAC_PHY_ChipSelect(g_rmac_phy0.p_ctrl, (uint8_t) g_rmac_phy0.p_cfg->channel);
        if (FSP_SUCCESS == port_select_err)
        {
            page_standard_selected = phy_select_page(PHY_RTL8211F_STANDARD_PAGE,
                                                     &page_standard_readback,
                                                     &page_standard_select_err);
            if (page_standard_selected)
            {
                bmsr_latched_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                   PHY_REG_BASIC_STATUS,
                                                   &bmsr_latched);
            }
            if (FSP_SUCCESS == bmsr_latched_err)
            {
                bmsr_current_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl,
                                                   PHY_REG_BASIC_STATUS,
                                                   &bmsr_current);
            }
            if (FSP_SUCCESS == bmsr_current_err)
            {
                /* ANER and GBSR contain latched read-clear fields; this trace is their intended consumer. */
                aner_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_AN_EXPANSION, &aner);
            }
            if (FSP_SUCCESS == aner_err)
            {
                gbcr_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_GIGABIT_CONTROL, &gbcr);
            }
            if (FSP_SUCCESS == gbcr_err)
            {
                gbsr_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_GIGABIT_STATUS, &gbsr);
            }
            if (FSP_SUCCESS == gbsr_err)
            {
                page_physr_selected = phy_select_page(PHY_RTL8211F_PHYSR_PAGE,
                                                      &page_physr_readback,
                                                      &page_physr_select_err);
            }
            if (page_physr_selected)
            {
                physr_err = R_RMAC_PHY_Read(g_rmac_phy0.p_ctrl, PHY_REG_PHYSR, &physr);
            }
            page_restored = phy_select_page(PHY_RTL8211F_STANDARD_PAGE,
                                            &page_restore_readback,
                                            &page_restore_err);
        }

        (void) xTaskResumeAll();

        if (FSP_ERR_NOT_OPEN == port_select_err)
        {
            now = xTaskGetTickCount();
            if ((now - last_log_time) >= heartbeat_interval)
            {
                LOG_I(LOG_TAG,
                      "PHY AN trace: waiting for RMAC PHY driver, elapsed=%lu ms",
                      (unsigned long) pdTICKS_TO_MS(now - start_time));
                last_log_time = now;
            }

            vTaskDelay(poll_interval);
            continue;
        }
        if (FSP_SUCCESS != port_select_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: port select failed: 0x%08X",
                  (unsigned int) port_select_err);
            return false;
        }
        if (FSP_SUCCESS != page_standard_select_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: standard page access failed: 0x%08X",
                  (unsigned int) page_standard_select_err);
            return false;
        }
        if (!page_standard_selected)
        {
            LOG_W(LOG_TAG,
                  "PHY AN trace: standard page was not retained, PAGSR=0x%04X; sample skipped",
                  (unsigned int) page_standard_readback);
            vTaskDelay(poll_interval);
            continue;
        }
        if (FSP_SUCCESS != bmsr_latched_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: BMSR latched read failed: 0x%08X",
                  (unsigned int) bmsr_latched_err);
            return false;
        }
        if (FSP_SUCCESS != bmsr_current_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: BMSR current read failed: 0x%08X",
                  (unsigned int) bmsr_current_err);
            return false;
        }
        if (FSP_SUCCESS != aner_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: ANER read failed: 0x%08X",
                  (unsigned int) aner_err);
            return false;
        }
        if (FSP_SUCCESS != gbcr_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: GBCR read failed: 0x%08X",
                  (unsigned int) gbcr_err);
            return false;
        }
        if (FSP_SUCCESS != gbsr_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: GBSR read failed: 0x%08X",
                  (unsigned int) gbsr_err);
            return false;
        }
        if (FSP_SUCCESS != page_restore_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: standard page restore access failed: 0x%08X",
                  (unsigned int) page_restore_err);
            return false;
        }
        if (!page_restored)
        {
            LOG_W(LOG_TAG,
                  "PHY AN trace: standard page restore was not retained, PAGSR=0x%04X; sample skipped",
                  (unsigned int) page_restore_readback);
            vTaskDelay(poll_interval);
            continue;
        }
        if (FSP_SUCCESS != page_physr_select_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: PHYSR page access failed: 0x%08X",
                  (unsigned int) page_physr_select_err);
            return false;
        }
        if (!page_physr_selected)
        {
            LOG_W(LOG_TAG,
                  "PHY AN trace: PHYSR page was not retained, PAGSR=0x%04X; sample skipped",
                  (unsigned int) page_physr_readback);
            vTaskDelay(poll_interval);
            continue;
        }
        if (FSP_SUCCESS != physr_err)
        {
            LOG_E(LOG_TAG,
                  "PHY AN trace: PHYSR read failed: 0x%08X",
                  (unsigned int) physr_err);
            return false;
        }

        now = xTaskGetTickCount();
        invalid_sample = (PHY_INVALID_REGISTER_VALUE == (bmsr_latched & 0xFFFFU)) ||
                         (PHY_INVALID_REGISTER_VALUE == (bmsr_current & 0xFFFFU)) ||
                         (PHY_INVALID_REGISTER_VALUE == (aner & 0xFFFFU)) ||
                         (PHY_INVALID_REGISTER_VALUE == (gbcr & 0xFFFFU)) ||
                         (PHY_INVALID_REGISTER_VALUE == (gbsr & 0xFFFFU)) ||
                         (PHY_INVALID_REGISTER_VALUE == (physr & 0xFFFFU));
        if (invalid_sample)
        {
            LOG_W(LOG_TAG,
                  "PHY AN trace: invalid MDIO sample skipped: BMSR=%04X/%04X, ANER=%04X, "
                  "GBCR=%04X, GBSR=%04X, PHYSR=%04X",
                  (unsigned int) bmsr_latched,
                  (unsigned int) bmsr_current,
                  (unsigned int) aner,
                  (unsigned int) gbcr,
                  (unsigned int) gbsr,
                  (unsigned int) physr);
            vTaskDelay(poll_interval);
            continue;
        }

        state_changed = (aner != previous_aner) ||
                        (bmsr_current != previous_bmsr_current) ||
                        (bmsr_latched != previous_bmsr_latched) ||
                        (gbcr != previous_gbcr) ||
                        (gbsr != previous_gbsr) ||
                        (physr != previous_physr);
        if (state_changed || ((now - last_log_time) >= heartbeat_interval))
        {
            log_phy_auto_negotiation_sample((uint32_t) pdTICKS_TO_MS(now - start_time),
                                            bmsr_latched,
                                            bmsr_current,
                                            aner,
                                            gbcr,
                                            gbsr,
                                            physr,
                                            page_standard_readback,
                                            page_physr_readback,
                                            page_restore_readback);
            last_log_time = now;
        }

        previous_aner         = aner;
        previous_bmsr_current = bmsr_current;
        previous_bmsr_latched = bmsr_latched;
        previous_gbcr         = gbcr;
        previous_gbsr         = gbsr;
        previous_physr        = physr;

        if ((0U != (bmsr_current & PHY_BMSR_AN_COMPLETE_MASK)) &&
            (0U != (bmsr_current & PHY_BMSR_LINK_STATUS_MASK)))
        {
            LOG_I(LOG_TAG,
                  "PHY AN trace: auto-negotiation completed in %lu ms",
                  (unsigned long) pdTICKS_TO_MS(xTaskGetTickCount() - start_time));
            return true;
        }

        vTaskDelay(poll_interval);
    }

    LOG_W(LOG_TAG,
          "PHY AN trace: auto-negotiation timeout after %u ms, "
          "BMSR=0x%04X, GBCR=0x%04X, GBSR=0x%04X",
          (unsigned int) PHY_AUTONEG_TIMEOUT_MS,
          (unsigned int) bmsr_current,
          (unsigned int) gbcr,
          (unsigned int) gbsr);

    return false;
}
#endif

static bool wait_for_gateway_arp(void)
{
    uint32_t gateway_address = FreeRTOS_GetGatewayAddress();
    char gateway_address_string[16];

    (void) FreeRTOS_inet_ntoa(gateway_address, gateway_address_string);
    LOG_I(LOG_TAG, "Resolving gateway ARP: %s", gateway_address_string);

    while (pdTRUE == FreeRTOS_IsNetworkUp())
    {
        if (0 == xARPWaitResolution(gateway_address, pdMS_TO_TICKS(ARP_RESOLUTION_TIMEOUT_MS)))
        {
            LOG_I(LOG_TAG, "Gateway ARP resolved: %s", gateway_address_string);
            return true;
        }

        LOG_W(LOG_TAG, "Gateway ARP resolution failed: %s; retrying", gateway_address_string);
        vTaskDelay(pdMS_TO_TICKS(ARP_RESOLUTION_RETRY_MS));
    }

    return false;
}

eDHCPCallbackAnswer_t xApplicationDHCPHook(eDHCPCallbackPhase_t eDHCPPhase, uint32_t ulIPAddress)
{
    if (eDHCPPhasePreDiscover == eDHCPPhase)
    {
        LOG_I(LOG_TAG, "DHCP phase: sending DISCOVER");
    }
    else if (eDHCPPhasePreRequest == eDHCPPhase)
    {
        char offered_address_string[16];

        (void) FreeRTOS_inet_ntoa(ulIPAddress, offered_address_string);
        LOG_I(LOG_TAG, "DHCP phase: requesting offered address %s", offered_address_string);
    }
    else
    {
        LOG_W(LOG_TAG, "DHCP phase: unknown=%u", (unsigned int) eDHCPPhase);
    }

    return eDHCPContinue;
}

const char *pcApplicationHostnameHook(void)
{
    return DHCP_HOSTNAME;
}

void vApplicationPingReplyHook(ePingReplyStatus_t eStatus, uint16_t usIdentifier)
{
    if (NULL != s_ping_task_handle)
    {
        s_ping_reply_status     = eStatus;
        s_ping_reply_identifier = usIdentifier;
        xTaskNotifyGive(s_ping_task_handle);
    }
}

static uint32_t wait_for_dhcp_address(void)
{
    const TickType_t status_log_interval = pdMS_TO_TICKS(DHCP_STATUS_LOG_INTERVAL_MS);
    TickType_t last_status_log           = xTaskGetTickCount();
    TickType_t start_time                = last_status_log;
    uint32_t ip_address                  = 0U;
    bool dhcp_restart_requested          = false;

    LOG_I(LOG_TAG, "Waiting for DHCP address");
    while ((pdTRUE != FreeRTOS_IsNetworkUp()) || (0U == ip_address))
    {
        BaseType_t network_up;
        TickType_t now;

        vTaskDelay(pdMS_TO_TICKS(DHCP_POLL_INTERVAL_MS));
        network_up = FreeRTOS_IsNetworkUp();
        ip_address = FreeRTOS_GetIPAddress();
        now        = xTaskGetTickCount();

        if ((!dhcp_restart_requested) && (pdTRUE == network_up) && (0U == ip_address))
        {
            NetworkInterface_t *p_interface = FreeRTOS_FirstNetworkInterface();

            /* Recheck the address in case DHCP completed between the two status reads above. */
            ip_address = FreeRTOS_GetIPAddress();
            if (0U == ip_address)
            {
                dhcp_restart_requested = true;
                if (NULL != p_interface)
                {
                    LOG_W(LOG_TAG,
                          "PHY link is up before DHCP completed; restarting network interface");
                    FreeRTOS_NetworkDown(p_interface);
                    last_status_log = now;
                    continue;
                }

                LOG_E(LOG_TAG, "DHCP recovery failed: network interface is unavailable");
            }
        }

        if ((now - last_status_log) >= status_log_interval)
        {
            LOG_I(LOG_TAG,
                  "DHCP still waiting: elapsed=%lu ms, networkUp=%u, ip=0x%08X",
                  (unsigned long) pdTICKS_TO_MS(now - start_time),
                  (unsigned int) network_up,
                  (unsigned int) ip_address);
            last_status_log = now;
        }
    }

    {
        char ip_address_string[16];

        (void) FreeRTOS_inet_ntoa(ip_address, ip_address_string);
        LOG_I(LOG_TAG, "DHCP address: %s", ip_address_string);
    }

    return ip_address;
}

static bool wait_for_ping_reply(uint16_t expected_identifier,
                                TickType_t send_time,
                                TickType_t *p_elapsed_time,
                                ePingReplyStatus_t *p_reply_status)
{
    const TickType_t timeout = pdMS_TO_TICKS(PING_REPLY_TIMEOUT_MS);
    TickType_t elapsed_time  = 0U;

    while (elapsed_time < timeout)
    {
        TickType_t remaining_time = timeout - elapsed_time;

        if (0U == ulTaskNotifyTake(pdTRUE, remaining_time))
        {
            return false;
        }

        elapsed_time = xTaskGetTickCount() - send_time;
        if (expected_identifier == s_ping_reply_identifier)
        {
            *p_elapsed_time = elapsed_time;
            *p_reply_status = s_ping_reply_status;
            return true;
        }
    }

    return false;
}

#if LOG_CFG_EN_TIMESTAMP
void LOG_GetTime(uint32_t *s, uint32_t *ms)
{
	int64_t t = get_system_ms();
	*s = (uint32_t)(t / 1000);
	*ms = (uint32_t)(t % 1000);
}
#endif
