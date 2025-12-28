#include "network_thread.h"
#include "common_utils.h"
#include "dhcpv4_client_ep.h"

/*******************************************************************************************************************//**
 * @addtogroup NetX_dhcpv4_client_ep
 * @{
 **********************************************************************************************************************/




#define TCP_CLIENT      (1)
#define TCP_SERVER      (2)

#define UDP_SERVER      (3)
#define UDP_CLIENT      (4)

#define PROTOCOL_TYPE  TCP_SERVER
//#define PROTOCOL_TYPE  TCP_CLIENT
//#define PROTOCOL_TYPE  UDP_SERVER
//#define PROTOCOL_TYPE  UDP_CLIENT

#define SERVER_ADDRESS              IP_ADDRESS(192,168,1,104)   // server IP
#define TCP_PORT                    5001                       // tcp port
#define UDP_PORT                    5002                       // udp port


#define MAX_TCP_CLIENTS (5)
#define IPERF_BUFSZ     (4 * 1024)
#define MAX_PACKET_SIZE  (1500)

#define STATIC_IP_ADDRESS     IP_ADDRESS(192, 168, 0, 52)
#define STATIC_NETMASK        IP_ADDRESS(255, 255, 255, 0)
#define STATIC_GATEWAY        IP_ADDRESS(192, 168, 0, 3)

/* Function declarations*/
/* Define the function to call for running a DHCP Client session. */
static UINT run_dhcp_client_session(NX_DHCP *client_ptr, NX_IP *ip_ptr);
static VOID dhcpv4_client_notify_callback(NX_DHCP *client_ptr, UCHAR state);
static void nx_common_init0(void);
static void packet_pool_init0(void);
static void ip_init0(void);
static void dhcp_client_init0(void);

/* Global variables */
TX_EVENT_FLAGS_GROUP my_event_flags_group;
/* DHCP instance. */
NX_DHCP g_dhcp_client0;

/* IP instance */
NX_IP g_ip0;

/* Stack memory for g_ip0. */
uint8_t g_ip0_stack_memory[G_IP0_TASK_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.g_ip0") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

/* ARP cache memory for g_ip0. */
uint8_t g_ip0_arp_cache_memory[G_IP0_ARP_CACHE_SIZE] BSP_ALIGN_VARIABLE(4);

/* Packet pool instance (If this is a Trustzone part, the memory must be placed in Non-secure memory). */
NX_PACKET_POOL g_packet_pool0;
uint8_t g_packet_pool0_pool_memory[G_PACKET_POOL0_PACKET_NUM * (G_PACKET_POOL0_PACKET_SIZE + sizeof(NX_PACKET))] BSP_ALIGN_VARIABLE(4) ETHER_BUFFER_PLACE_IN_SECTION;

UCHAR send_buf[IPERF_BUFSZ];

#if (PROTOCOL_TYPE==TCP_SERVER) || (PROTOCOL_TYPE==TCP_CLIENT)
NX_TCP_SOCKET           g_tcp_socket;
#elif (PROTOCOL_TYPE==UDP_SERVER) || (PROTOCOL_TYPE==UDP_CLIENT)
NX_UDP_SOCKET           g_udp_socket;
#endif

#if (PROTOCOL_TYPE==TCP_SERVER) || (PROTOCOL_TYPE==TCP_CLIENT)
void send_data_over_tcp(char *data, UINT data_size);
#endif

void send_data_over_udp(char *data, UINT data_size);
void netcomm_process(void);
/* Network Thread entry function */
void network_thread_entry(void)
{
    UINT               status  = NX_SUCCESS;

    /* Initialize the RTT Thread.*/
    rtt_thread_init_check();
    /* print the banner and EP info. */
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_BANNER, RESET_VALUE, NULL);

    /* Initialize the NetX system.*/
    nx_common_init0();

    /* Initialize the packet pool.*/
    packet_pool_init0();

    /* create the ip instance.*/
    ip_init0();

#if 1
    /* Initialize the dhcp client.*/
    dhcp_client_init0();

    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Network Initialization completed successfully."), "Network Initialization completed successfully.");

    /* Start and run a brief DHCP Client session. */
    status = run_dhcp_client_session(&g_dhcp_client0, &g_ip0);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("run_dhcp_client_session failed."), "run_dhcp_client_session failed.");
        /* Internal DHCP error or NetX internal error. We cannot continue this test. */
        nx_dhcp_delete(&g_dhcp_client0);
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }
#else
    // 设置静态 IP 地址
    nx_ip_address_set(&g_ip0, STATIC_IP_ADDRESS, STATIC_NETMASK);

    // 设置默认网关（可选）
    nx_ip_gateway_address_set(&g_ip0, STATIC_GATEWAY);
#endif
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCPV4 Client EP completed."), "DHCPV4 Client EP completed.");
    netcomm_process();
}

void netcomm_process(void)
{
    UINT status  = NX_SUCCESS;
    NX_PACKET *data_packet;
    ULONG bytes_read;
    static UCHAR data_buffer[2048];
    uint32_t total_bytes = 0;

#if (PROTOCOL_TYPE==TCP_SERVER) || (PROTOCOL_TYPE==TCP_CLIENT)
    ULONG socket_state;
    /* Enable NX TCP Module */
    status = nx_tcp_enable(&g_ip0);
    if(NX_SUCCESS != status)
    {
        APP_PRINT(" NX TCP Enable err = %d\r\n", status);
    }
    else
    {
        APP_PRINT("NX TCP Enabled\r\n");
    }    

     for (int i = 0; i < IPERF_BUFSZ; i ++)
         send_buf[i] = i & 0xff;

    nx_tcp_socket_create(&g_ip0, &g_tcp_socket,"TCP demo Socket",NX_IP_NORMAL,NX_FRAGMENT_OKAY,NX_IP_TIME_TO_LIVE,32 * 1024,NX_NULL,NX_NULL);

    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tcp socket create failed."), "tcp socket create failed.");
        nx_tcp_socket_delete(&g_tcp_socket);
        return; 
    }
#elif (PROTOCOL_TYPE==UDP_SERVER) || (PROTOCOL_TYPE==UDP_CLIENT)

    /* create udp socket */
    status = nx_udp_socket_create(&g_ip0, &g_udp_socket, "UDP Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 100);
    if (status != NX_SUCCESS)
    {
        // error
        nx_udp_socket_delete(&g_udp_socket);
        return;
    }

#endif

#if (PROTOCOL_TYPE==TCP_SERVER)
    status = nx_tcp_server_socket_listen(&g_ip0,TCP_PORT, &g_tcp_socket,MAX_TCP_CLIENTS, NULL); 
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tcp socket listen failed."), "tcp socket listen failed.");
    }

    status = nx_tcp_server_socket_accept (&g_tcp_socket, NX_WAIT_FOREVER);
    if (status!=NX_SUCCESS)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("socket accept failed"), "socket accept failed");
    }

    while (true)
    {
        status = nx_tcp_server_socket_accept(&g_tcp_socket, TX_WAIT_FOREVER);
         if (status != NX_SUCCESS)
         {
             APP_PRINT("Socket accept failed: 0x%x\n", status);
             continue;
         }

         APP_PRINT("TCP Client connected\n");

         while (1)
         {
             status = nx_tcp_socket_receive(&g_tcp_socket, &data_packet, NX_WAIT_FOREVER);
             if (status == NX_SUCCESS)
             {
                 status = nx_packet_data_retrieve(data_packet, data_buffer, &bytes_read);
                 nx_packet_release(data_packet);
                 total_bytes += bytes_read;
                 // APP_PRINT("Received %d bytes\r\n", bytes_read);
             }
             else
             {
                 APP_PRINT("Receive failed or disconnected: 0x%x read total %u bytes data\n",
                           status, total_bytes);
                 break;
             }

             tx_thread_sleep(10);
         }

         nx_tcp_socket_disconnect(&g_tcp_socket, NX_WAIT_FOREVER);
         nx_tcp_server_socket_unaccept(&g_tcp_socket);
         nx_tcp_server_socket_relisten(&g_ip0, TCP_PORT, &g_tcp_socket);
    }

#elif (PROTOCOL_TYPE==TCP_CLIENT)
    status = nx_tcp_client_socket_bind(&g_tcp_socket,TCP_PORT, TX_WAIT_FOREVER);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tcp socket bind failed."), "tcp socket bind failed.");

        nx_tcp_socket_delete(&g_tcp_socket);
        return;        
    }
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("binding..."), "binding...");

    /* connect to server */
    status = nx_tcp_client_socket_connect(&g_tcp_socket, SERVER_ADDRESS, TCP_PORT, NX_WAIT_FOREVER);

    if (status != NX_SUCCESS)
    {
        /* connection failed */
        APP_PRINT("\r\n Connected server failed!\r\n");
        nx_tcp_socket_delete(&g_tcp_socket);
        return;
    }

    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Connected server successfully!"), "Connected server successfully!");
  

    while (1)
    {
        /* send data */
        send_data_over_tcp(send_buf,1500);
        if (status != NX_SUCCESS)
        {
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("TCP send failed."), "TCP send failed.");
            break;
        }
        tx_thread_sleep(100); //
    }

#elif (PROTOCOL_TYPE==UDP_SERVER)

    /* socket bind */
    status = nx_udp_socket_bind(&g_udp_socket, UDP_PORT, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        // error
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_udp_socket_bind failed."), "nx_udp_socket_bind failed.");
        return;
    }

    while(1)
    {
        /* wait for receiving data */
        status = nx_udp_socket_receive(&g_udp_socket, &data_packet, NX_WAIT_FOREVER);
        if(status == NX_SUCCESS)
        {
            /* received data */
            status = nx_packet_data_retrieve(data_packet,data_buffer,&bytes_read);

            if (status == NX_SUCCESS)
            {
               // APP_PRINT("received %d data \r\n ",bytes_read);
            }
            // release buffer is necessary
            nx_packet_release(data_packet);

            tx_thread_sleep (10);
        }
        else
        {
            continue;
        }
    }

#elif (PROTOCOL_TYPE==UDP_CLIENT)

    /* socket bind */
    status = nx_udp_socket_bind(&g_udp_socket, UDP_PORT, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        // error
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_udp_socket_bind failed."), "nx_udp_socket_bind failed.");
        return;
    }

    while (1)
    {
        // will implement in next release
        tx_thread_sleep(10);
    }

#endif 
}



#if (PROTOCOL_TYPE==TCP_SERVER) || (PROTOCOL_TYPE==TCP_CLIENT)
void send_data_over_tcp(char *data, UINT data_size) {
    UINT bytes_remaining = data_size;
    UINT bytes_to_send;
    char *data_ptr = data;

    NX_PACKET   *my_packet        = NULL;
    UINT               status  = NX_SUCCESS;

    // Loop until all data is sent
    while (bytes_remaining > 0) {
        // Allocate a packet from the packet pool
        status = nx_packet_allocate(&g_packet_pool0, &my_packet, NX_TCP_PACKET,NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            // Handle error
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_packet_allocate failed."), "nx_packet_allocate failed.");
            break;
        }

        // Calculate how many bytes to send in this packet
        bytes_to_send = (bytes_remaining > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : bytes_remaining;

        nx_packet_data_append(my_packet, data_ptr,bytes_to_send,&g_packet_pool0, NX_WAIT_FOREVER);

        // Advance pointers
        data_ptr += bytes_to_send;
        bytes_remaining -= bytes_to_send;

        // Send packet over TCP
        status = nx_tcp_socket_send(&g_tcp_socket, my_packet, NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            // Handle error
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_tcp_socket_send error."), "nx_tcp_socket_send error.");
            break;
        }
    }
}

#elif (PROTOCOL_TYPE==UDP_SERVER) || (PROTOCOL_TYPE==UDP_CLIENT)

void send_data_over_udp(char *data, UINT data_size) {
    UINT bytes_remaining = data_size;
    UINT bytes_to_send;
    char *data_ptr = data;

    NX_PACKET   *my_packet        = NULL;
    UINT               status  = NX_SUCCESS;

    // Loop until all data is sent
    while (bytes_remaining > 0) {
        // Allocate a packet from the packet pool
        status = nx_packet_allocate(&g_packet_pool0, &my_packet, NX_UDP_PACKET,NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            // Handle error
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_packet_allocate failed."), "nx_packet_allocate failed.");
            break;
        }

        // Calculate how many bytes to send in this packet
        bytes_to_send = (bytes_remaining > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : bytes_remaining;

        nx_packet_data_append(my_packet, data_ptr,bytes_to_send,&g_packet_pool0, NX_WAIT_FOREVER);

        // Advance pointers
        data_ptr += bytes_to_send;
        bytes_remaining -= bytes_to_send;

        // Send packet over UDP
        status = nx_udp_socket_send(&g_udp_socket, my_packet, SERVER_ADDRESS, UDP_PORT);
        if (status != NX_SUCCESS) {
            // Handle error
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_udp_socket_send error."), "nx_udp_socket_send error.");
            break;
        }
    }
}

#endif

/*******************************************************************************************************************//**
 * @brief     Initialization of NetX system.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
static void nx_common_init0(void)
{
    /* Initialize the NetX system. */
    nx_system_initialize ();
}

/*******************************************************************************************************************//**
 * @brief     Create the packet pool.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
static void packet_pool_init0(void)
{
    /* Create the packet pool. */
    UINT status = nx_packet_pool_create(&g_packet_pool0,
                                        "g_packet_pool0 Packet Pool",
                                        G_PACKET_POOL0_PACKET_SIZE,
                                        &g_packet_pool0_pool_memory[0],
                                        G_PACKET_POOL0_PACKET_NUM * (G_PACKET_POOL0_PACKET_SIZE + sizeof(NX_PACKET)));
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_packet_pool_create failed."), "nx_packet_pool_create failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }
}

/*******************************************************************************************************************//**
 * @brief     Create the ip instance and enables ARP,UDP,ICMP.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
static void ip_init0(void)
{
    UINT status = NX_SUCCESS;

    /* Create the ip instance. */
    status = nx_ip_create(&g_ip0,
                          "g_ip0 IP Instance",
                          G_IP0_ADDRESS,
                          G_IP0_SUBNET_MASK,
                          &g_packet_pool0,
                          g_netxduo_ether_0,
                          &g_ip0_stack_memory[0],
                          G_IP0_TASK_STACK_SIZE,
                          G_IP0_TASK_PRIORITY);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_ip_create failed."), "nx_ip_create failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }

    /* Set the gateway address if it is configured. */
    if (IP_ADDRESS(0, 0, 0, 0) != G_IP0_GATEWAY_ADDRESS)
    {
        status = nx_ip_gateway_address_set (&g_ip0, G_IP0_GATEWAY_ADDRESS);
        if(NX_SUCCESS != status)
        {
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_ip_gateway_address_set failed."), "nx_ip_gateway_address_set failed.");
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
        }
    }

    /* Enables Address Resolution Protocol (ARP).*/
    status = nx_arp_enable(&g_ip0, &g_ip0_arp_cache_memory[0], G_IP0_ARP_CACHE_SIZE);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_arp_enable failed."), "nx_arp_enable failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }

    /* Enable udp.*/
    status = nx_udp_enable(&g_ip0);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_udp_enable failed."), "nx_udp_enable failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }

    /* Enable icmp.*/
    status = nx_icmp_enable(&g_ip0);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_icmp_enable failed."), "nx_icmp_enable failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }

    /* Wait for the link to be enabled. */
    ULONG current_state;
    status = nx_ip_status_check(&g_ip0, NX_IP_LINK_ENABLED, &current_state, LINK_ENABLE_WAIT_TIME);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_ip_status_check failed."), "nx_ip_status_check failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }
}

/*******************************************************************************************************************//**
 * @brief     Create the DHCP instance and Set the DHCP Client packet pool.
 * @param[IN] None
 * @retval    None
 **********************************************************************************************************************/
static void dhcp_client_init0(void)
{
    /* Create the DHCP instance. */
    UINT status = nx_dhcp_create(&g_dhcp_client0,
                                 &g_ip0,
                                 "g_dhcp_client0");
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_create failed."), "nx_dhcp_create failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }

    /* Set the DHCP Client packet pool. */
    status = nx_dhcp_packet_pool_set(&g_dhcp_client0, &g_packet_pool0);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_packet_pool_set failed."), "nx_dhcp_packet_pool_set failed.");
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *), &status);
    }
}

/*******************************************************************************************************************//**
 * @brief     This function runs a DHCP Client session.
 * @param[IN] *client_ptr    pointer to an NX_DHCP instance, an already created DHCP Client instance.
 * @param[IN] *ip_ptr        pointer to an NX_IP instance, an already created IP instance.
 * @retval    Any Other Error code apart from NX_SUCCESS on Unsuccessful operation.
 **********************************************************************************************************************/
static UINT run_dhcp_client_session(NX_DHCP *client_ptr, NX_IP *ip_ptr)
{
    UINT        status            = NX_SUCCESS;
    ULONG       actual_status     = RESET_VALUE;
    NX_PACKET   *my_packet        = NULL;
    ULONG       server_address    = RESET_VALUE;
    ULONG       client_address    = RESET_VALUE;
    ULONG       network_mask      = RESET_VALUE;
    UCHAR       dns_buffer[4*MAX_DNS_SERVERS]  = {INITIAL_VALUE};
    UINT        dns_buffer_size                = IP_V4_SIZE;
    UCHAR       gateway_buffer[4*MAX_GATEWAYS] = {INITIAL_VALUE};
    UINT        gateway_buffer_size            = IP_V4_SIZE;
    ULONG       *dns_server_ptr                = NULL;
    ULONG       *gateway_ptr                   = NULL;
    UINT        wait                           = RESET_VALUE;
    ULONG       actual_events                  = RESET_VALUE;

    /* Create an event flags group. */
    status = tx_event_flags_create(&my_event_flags_group, "my_event_group_name");
    if(TX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tx_event_flags_create failed."), "tx_event_flags_create failed.");
        return status;
    }

    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Checking Ethernet Link..."), "Checking Ethernet Link...");

    /* Wait for the link to come up.  */
    do
    {
        status = nx_ip_driver_direct_command(&g_ip0, NX_LINK_GET_STATUS, &actual_status);

    } while (NX_TRUE != actual_status);

    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Ethernet link is up."), "Ethernet link is up.");

    /* Register a DHCP state change callback function. */
    status = nx_dhcp_state_change_notify(client_ptr, dhcpv4_client_notify_callback);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_state_change_notify failed."), "nx_dhcp_state_change_notify failed.");
        return status;
    }

    /* Now we're ready to start the DHCP Client.  */
    status =  nx_dhcp_start(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_start failed."), "nx_dhcp_start failed.");
        return status;
    }
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCP client is running."), "DHCP client is running.");

    /* Wait until an IP address is acquired via DHCP. */
    /* wait for the bound event*/
    status = tx_event_flags_get(&my_event_flags_group, 0x1,TX_AND_CLEAR, &actual_events, 2000);
    if((TX_SUCCESS == status) && (true == actual_events))
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCP client is assigned an IP address."), "DHCP client is assigned an IP address.");

        /* It is. Get the client IP address from this NetX service. */
        status = nx_ip_address_get(ip_ptr, &client_address, &network_mask);
        if(NX_SUCCESS != status)
        {
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_ip_address_get failed."), "nx_ip_address_get failed.");
            return status;
        }
        /* print client IP address. */
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_PRINT_CLIENT_IP, sizeof(ULONG *), &client_address);

        /* Get the DHCP Server IP address.  */
        status = nx_dhcp_server_address_get(client_ptr, &server_address);
        if(NX_SUCCESS != status)
        {
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_server_address_get failed."), "nx_dhcp_server_address_get failed.");
            return status;
        }
        /* print server IP address. */
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_PRINT_SERVER_IP, sizeof(ULONG *), &server_address);

        /* Check that the device is able to send and receive packets with this IP address. */
        status =  nx_icmp_ping(ip_ptr, server_address, "Hello World", sizeof("Hello World"), &my_packet, NX_WAIT_FOREVER);
        if(NX_SUCCESS != status)
        {
            app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_icmp_ping failed."), "nx_icmp_ping failed.");
            return status;
        }
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Successfully Pinged DHCP Server."), "Successfully Pinged DHCP Server.");

        /* Release the echo response packet when we are done with it. */
        nx_packet_release(my_packet);
    }
#if 1
    /* Perform lease time operation. */
    NX_DHCP_CLIENT_RECORD dhcp_record;

    /* Obtain a record of the current client state. */
    status=  nx_dhcp_client_get_record(client_ptr, &dhcp_record);;
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_client_get_record failed."), "nx_dhcp_client_get_record failed.");
        return status;
    }
    /* print lease time. */
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_VAL, sizeof(ULONG *), &(dhcp_record.nx_dhcp_lease_time));

    /* Stop the DHCP Client. The application can still send and receive network packets. */
    status = nx_dhcp_stop(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_stop failed."), "nx_dhcp_stop failed.");
        return status;
    }

    /* Prepare the DHCP Client to restart. We can still send and receive
     * packets except broadcast packets, but with a source IP address
     * of zero, is not very useful except for DHCP. */
    status = nx_dhcp_reinitialize(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_reinitialize failed."), "nx_dhcp_reinitialize failed.");
        return status;
    }
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCP Client is reinitializing..."), "DHCP Client is reinitializing...");

    /* Clear our previous DHCP session event flag. */
    actual_events = NX_FALSE;
    /* Restart the DHCP Client thread task. */
    status = nx_dhcp_start(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_start failed."), "nx_dhcp_start failed.");
        return status;
    }
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCP client restarted."), "DHCP client restarted.");

    /* This time we'll poll the IP instance directly for a valid IP address.  */
    do
    {
        /* Check for address resolution.  */
        status = nx_ip_status_check(ip_ptr, NX_IP_ADDRESS_RESOLVED, (ULONG *) &actual_status, NX_IP_PERIODIC_RATE);

        /* Check status.  */
        if (status)
        {
            /* wait a bit. */
            tx_thread_sleep(NX_IP_PERIODIC_RATE);

            wait += NX_IP_PERIODIC_RATE;
            if (WAIT_TO_BE_BOUND <= wait)
            {
                break;
            }
        }

    } while (status != NX_SUCCESS);

    /* Check if we have a valid address. */
    if (status == NX_SUCCESS)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCP client is assigned IP address lease for second time."), "DHCP client is assigned IP address lease for second time.");
        /* We do. This time, query the DHCP Client for the DNS Server address.  */
        status = nx_dhcp_user_option_retrieve(client_ptr, NX_DHCP_OPTION_DNS_SVR, dns_buffer, &dns_buffer_size);
        while(status == NX_DHCP_DEST_TO_SMALL)
        {
            if(dns_buffer_size < (MAX_DNS_SERVERS*IP_V4_SIZE))
            {
                dns_buffer_size = dns_buffer_size+IP_V4_SIZE;
                status = nx_dhcp_user_option_retrieve(client_ptr, NX_DHCP_OPTION_DNS_SVR, dns_buffer, &dns_buffer_size);
            }
            else
            {
                break;
            }
        }

        if(NX_SUCCESS == status)
        {
            dns_server_ptr = (ULONG *)(dns_buffer);

            for(uint8_t cnt = RESET_VALUE; cnt < (dns_buffer_size/IP_V4_SIZE); cnt++, dns_server_ptr++)
            {
                /* Send a ping request to the DNS server. */
                status =  nx_icmp_ping(ip_ptr, *dns_server_ptr, "Hello", sizeof("Hello"), &my_packet, (3* NX_IP_PERIODIC_RATE));
                /* No valid ICMP packet received (no packet to release). */
                if (NX_SUCCESS != status)
                {
                    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_icmp_ping failed."), "nx_icmp_ping failed.");
                    return status;
                }
                else
                {
                    /* valid ICMP packet received . */
                    /* print DNS server address.*/
                    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_PRINT_DNS_SERVER_IP, sizeof(ULONG *), dns_server_ptr);

                    /* Release the echo response packet when we are done with it. */
                    nx_packet_release(my_packet);
                }
            }
        }
    }

    /* We do. This time, query the DHCP Client for the DNS Gateway address.  */
    status = nx_dhcp_user_option_retrieve(client_ptr, NX_DHCP_OPTION_GATEWAYS, gateway_buffer, &gateway_buffer_size);

    while(status == NX_DHCP_DEST_TO_SMALL)
    {
        /* increase the buffer size*/
        if(gateway_buffer_size<MAX_GATEWAYS*IP_V4_SIZE)
        {
            gateway_buffer_size = gateway_buffer_size+IP_V4_SIZE;
            status = nx_dhcp_user_option_retrieve(client_ptr, NX_DHCP_OPTION_GATEWAYS, gateway_buffer, &gateway_buffer_size);
        }
        else
        {
            break;
        }
    }

    /* Check status.  */
    if (NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_user_option_retrieve failed."), "nx_dhcp_user_option_retrieve failed.");
        return status;
    }
    else
    {
        gateway_ptr = (ULONG *)(gateway_buffer);

        for(uint8_t cnt = RESET_VALUE; cnt < (gateway_buffer_size/IP_V4_SIZE); cnt++, dns_server_ptr++)
        {
            /* Send a ping request to the gateway. */
            status =  nx_icmp_ping(ip_ptr, *gateway_ptr, "Hello Gateway", sizeof("Hello Gateway"), &my_packet, 3* NX_IP_PERIODIC_RATE);

            /* No valid ICMP packet received (no packet to release). */
            if (NX_SUCCESS != status)
            {
                app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_icmp_ping failed."), "nx_icmp_ping failed.");
                return status;
            }
            else
            {
                /* print DNS gateway. */
                app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_PRINT_GATEWAY, sizeof(ULONG *), gateway_ptr);

                /* Release the echo response packet when we are done with it. */
                nx_packet_release(my_packet);
            }
        }
    }

#endif

    #if 0
    /* We're done with the DHCP Client. */
    /* Release the IP address back to the Server. This application should not
       send or receive packets with this IP address now. */
    status = nx_dhcp_release(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_release failed."), "nx_dhcp_release failed.");
        return status;
    }

    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("Released IP address back to Server."), "Released IP address back to Server.");

    /* Stop the DHCP Client and unbind the DHCP UDP socket.*/
    status = nx_dhcp_stop(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_stop failed."), "nx_dhcp_stop failed.");
        return status;
    }

    /* All done. Delete the Client and release resources to NetX and ThreadX. */
    status = nx_dhcp_delete(client_ptr);
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("nx_dhcp_delete failed."), "nx_dhcp_delete failed.");
        return status;
    }
    #endif
    return status;
}

/*******************************************************************************************************************//**
 * @brief       This function defines a user callback for DHCP Client to notify when there is a DHCP state change.
 *              In this callback, we only check if the DHCP Client has changed to the bound state (has a valid IP address)
 *              and we set a event flag for the application to check.
 * @param[IN]   client_ptr    Previously created DHCP Client instance.
 * @param[IN]   state         2 byte numeric representation of DHCP state.
 * @retval      None
 **********************************************************************************************************************/
static VOID dhcpv4_client_notify_callback(NX_DHCP *client_ptr, UCHAR state)
{
    UINT new_state = (UINT)state;

    NX_PARAMETER_NOT_USED(client_ptr);

    /* Check if we have transitioned to the bound state
       (have a valid IP address). */
    if (new_state == NX_DHCP_STATE_BOUND)
    {
        /* Set event flag. */
        tx_event_flags_set(&my_event_flags_group, DHCP_EVENT, TX_OR);
    }
}


