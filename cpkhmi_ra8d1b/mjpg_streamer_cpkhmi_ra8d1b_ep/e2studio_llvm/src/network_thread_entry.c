/***********************************************************************************************************************
 * File Name    : dhcp_client_thread_entry.c
 * Description  : Contains entry function of DHCPV4 Client.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#include "network_thread.h"
#include "common_utils.h"
#include "dhcpv4_client_ep.h"
#include "stdio.h"
#include "stdlib.h"
#include "ra_usbx_host_uvc_common.h"
#include "tx_port.h"
#include "bsp_sdram.h"
/*******************************************************************************************************************//**
 * @addtogroup NetX_dhcpv4_client_ep
 * @{
 **********************************************************************************************************************/

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


NX_TCP_SOCKET           g_tcp_socket;

UCHAR data_buffer[4096];
volatile UCHAR data_ready_flag=0;

char *received_http_data;


#define TCP_PORT (80)
#define MAX_TCP_CLIENTS (1)

#define MAX_PACKET_SIZE  (1500)


const static char http_streamer_payload[] = "HTTP/1.1 200 OK\r\n"\
                   "Connection: Keep-Alive\r\n"\
                   "Server: MJPG-Streamer/0.8\r\n"\
                   "Cache-Control:no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"\
                   "Pragma:no-cache\r\n"\
                   "Expires:WED, 23 Jan 2030 01:00:00 GMT\r\n"\
                   "Content-Type: multipart/x-mixed-replace;boundary=ramcu\r\n\r\n"\
	                 "--ramcu\r\n"; 

#define WEB_SERVER_PORT			           80   
typedef enum 
{
    MJPG_HTTP_OK,
    MJPG_HTTP_ERR,
    MJPG_HTTP_NONE,
}MJPG_HTTP_STATUS;

UCHAR pic_num=1;

uint8_t buffer[500*1024] BSP_PLACE_IN_SECTION(".sdram");

char tmp[200];
extern unsigned char s_arrImage[3580];

extern TX_THREAD ra_usbx_host_uvc_camera;

extern streamer_data_t     streamer_data,*pstreamer_data;


MJPG_HTTP_STATUS Streaming_Data_Send(void);
MJPG_HTTP_STATUS Init_Streaming_Data_Send(void);



void send_data_over_tcp(const char *data, UINT data_size);
/* DHCP Client Thread entry function */
void network_thread_entry(void)
{
    UINT               status  = NX_SUCCESS;
    ULONG socket_state;

    NX_PACKET *data_packet;
    ULONG bytes_read;

    ULONG peer_ip_address;
    ULONG peer_port;

    NX_PACKET *my_packet;    
    UINT tx_state;

    
    bsp_sdram_init();
    R_BSP_PinAccessEnable();
    /* Initialize the RTT Thread.*/
    rtt_thread_init_check();
    /* print the banner and EP info. */
    //app_rtt_print_data(RTT_OUTPUT_MESSAGE_BANNER, RESET_VALUE, NULL);
    APP_PRINT("\r\nra8d1 cpkhmi usbx uvc and mjpg streamer demo. \r\n");
    /* Initialize the NetX system.*/
    nx_common_init0();

    /* Initialize the packet pool.*/
    packet_pool_init0();

    /* create the ip instance.*/
    ip_init0();

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
    app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof("DHCPV4 Client EP completed."), "DHCPV4 Client EP completed.");

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

    status =  tx_thread_info_get(&ra_usbx_host_uvc_camera, TX_NULL, &tx_state, TX_NULL, TX_NULL, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
    if (TX_SUCCESS != status)
    {
        /* error */
        //R_BSP_PinWrite(LED3_R, BSP_IO_LEVEL_HIGH);
        __BKPT(0);
    }
    /* Check the UVC function thread state */
    if (TX_SUSPENDED == tx_state)
    {
        /* resume the Thread */
        tx_thread_resume(&ra_usbx_host_uvc_camera);
    }
    else
    {
        /*  Trap error */
        //R_BSP_PinWrite(LED3_R, BSP_IO_LEVEL_HIGH);

        APP_PRINT(" tx_thread_resume(&ra_usbx_host_uvc_camera) = %d\r\n", tx_state);
    }

    nx_tcp_socket_create(&g_ip0,
                        &g_tcp_socket,            /* TCP控制块 */
                        "TCP Server Socket",   /* TCP Socket名 */
                        NX_IP_NORMAL,          /* IP服务类型 */
                        NX_FRAGMENT_OKAY,      /* 使能IP分段 */
                        NX_IP_TIME_TO_LIVE,    /* 指定一个 8 位的值，用于定义此数据包在被丢弃之前可通过的路由器数目 */
                        4320,                  /* TCP Socket接收队列中允许的最大字节数 */
                        NX_NULL,               /* 用于在接收流中检测到紧急数据时调用的回调函数 */
                        NX_NULL              /* TCP Socket另一端发出断开连接时调用的回调函数 */
    );

    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tcp socket create failed."), "tcp socket create failed.");   
    }

    status = nx_tcp_server_socket_listen(&g_ip0, 
                                      TCP_PORT,           /* 端口 */         
                                      &g_tcp_socket,             /* TCP Socket控制块 */
                                      MAX_TCP_CLIENTS,        /* 可以监听的连接数 */
                                      NULL);                  /* 监听回调函数 */
    if(NX_SUCCESS != status)
    {
        app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof("tcp socket listen failed."), "tcp socket listen failed.");   
    }

    status = nx_tcp_server_socket_accept (&g_tcp_socket, NX_WAIT_FOREVER);
    if (status)
    {
        APP_PRINT ("socket accept failed: 0x%x\n", status);
    }

    //APP_PRINT("NX TCP Server Socket Accept \r\n ");

    while (true)
    {
        /* 获取socket状态 */
        nx_tcp_socket_info_get(&g_tcp_socket,     /* TCP Socket控制块 */
                                       NULL,           /* 发送的TCP数据包总数目 */
                                       NULL,           /* 发送的TCP总字节数 */
                                       NULL,           /* 接收TCP数据包总数目 */
                                       NULL,           /* 接收的TCP总字节数 */
                                       NULL,           /* 重新传输的TCP数据包总数目 */
                                       NULL,           /* Socket上TCP排队的TCP数据包总数 */
                                       NULL,           /* Socket上有校验和错误的TCP数据包总数 */
                                       &socket_state,  /* Socket当前状态 */
                                       NULL,           /* 仍在排队等待ACK的发送数据包总数 */
                                       NULL,           /* 当前发送窗口大小 */
                                       NULL);          /* 当前接收窗口大小 */

        /* 如果连接还没有建立，继续接受新连接，成功的话开启接收数据 */
        if(socket_state != NX_TCP_ESTABLISHED)
        {
            /* 启动TCP通信前，接收新连接 */
            status = nx_tcp_server_socket_accept(&g_tcp_socket,       /* TCP Socket控制块 */
                                                TX_WAIT_FOREVER); /* 等待连接 */

            if (status)
            {
                APP_PRINT ("socket accept failed: 0x%x\n", status);
            }
        }
        
        if((socket_state == NX_TCP_ESTABLISHED)&&(status == NX_SUCCESS))
        {
               
            /* 接收TCP客户端发的TCP数据包 */
            status = nx_tcp_socket_receive(&g_tcp_socket,        /* TCP Socket控制块 */
                                                    &data_packet,      /* 接收到的数据包 */
                                                    NX_WAIT_FOREVER);  /* 永久等待 */
            if (status == NX_SUCCESS)
            {
               
                                /* 获取客户端的IP地址和端口 */
                                nx_tcp_socket_peer_info_get(&g_tcp_socket,       /* TCP Socket控制块 */
                                                                                        &peer_ip_address, /* 远程IP地址 */
                                                                                        &peer_port);      /* 远程端口号 */

                /* 获取客户端发来的数据 */
                nx_packet_data_retrieve(data_packet,    /* 接收到的数据包 */
                                                        data_buffer,    /* 解析出数据 */
                                                        &bytes_read);   /* 数据大小 */


                //APP_PRINT("recv data \r\n ");

                if (bytes_read <= 0)
                {
                    APP_PRINT("\r\nbytes_received <= 0,socket close...\r\n");
                    break;
                }

                data_buffer[bytes_read] = '\0';      //有接收到数据，把末端清零 
                received_http_data=strtok(data_buffer,"\r\n");
  

                if(strcmp((const char*)received_http_data,"GET /?RA8=mjpeg HTTP/1.1")==0)
                {
                    APP_PRINT("\r\nrecv GET /?RA8=mjpeg HTTP/1.1!\r\n");
                    if(Init_Streaming_Data_Send()==MJPG_HTTP_OK)
                    {
                        while (1)
                        {                            
                            if(Streaming_Data_Send()==MJPG_HTTP_OK)
                            {
                                //APP_PRINT("\r\nStreaming_Data_Send OK!");
                            }
                            else 
                            {
                                //APP_PRINT("\r\nStreaming_Data_Send error!\r\n");
                            }
                        }                          
                    }
                }
                tx_thread_sleep (10);
            }
            else
            {
                /* 断开连接 */
                nx_tcp_socket_disconnect(&g_tcp_socket,
                                                         NX_WAIT_FOREVER);
                               
                                /* 解除Socket和服务器端口的绑定 */
                nx_tcp_server_socket_unaccept(&g_tcp_socket);
                               
                                /* 重新监听 */
                nx_tcp_server_socket_relisten(&g_ip0,
                                                              TCP_PORT,
                                                              &g_tcp_socket);
            }
            tx_thread_sleep (10);
        }




    }
    

    while (true)
    {
        tx_thread_sleep (1);
    }
}

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



void send_data_over_tcp(const char *data, UINT data_size) {
    UINT bytes_remaining = data_size;
    UINT bytes_sent = 0;
    UINT bytes_to_send;
    const char *data_ptr = data;

    NX_PACKET   *my_packet        = NULL;
    UINT               status  = NX_SUCCESS;

    // Loop until all data is sent
    while (bytes_remaining > 0) {
        // Allocate a packet from the packet pool
        status = nx_packet_allocate(&g_packet_pool0, &my_packet, NX_TCP_PACKET,NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            // Handle error
            APP_PRINT("\r\nnx_packet_allocate error%s %d!\r\n",__FUNCTION__,__LINE__);
            break;
        }

        // Calculate how many bytes to send in this packet
        bytes_to_send = (bytes_remaining > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : bytes_remaining;

        // Copy data to packet payload
        // memcpy(my_packet->nx_packet_prepend_ptr, data_ptr, bytes_to_send);
        // my_packet->nx_packet_length += bytes_to_send;

               nx_packet_data_append(my_packet, data_ptr,
                   bytes_to_send,
                   &g_packet_pool0, NX_WAIT_FOREVER);        

        // Advance pointers
        data_ptr += bytes_to_send;
        bytes_remaining -= bytes_to_send;

        // Send packet over TCP
        status = nx_tcp_socket_send(&g_tcp_socket, my_packet, NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            // Handle error
            APP_PRINT("\r\nnx_tcp_socket_send error%s %d!\r\n",__FUNCTION__,__LINE__);
            break;
        }
        //APP_PRINT("\r\n %s %d!\r\n",__FUNCTION__,__LINE__);
        // Increment total bytes sent
        bytes_sent += bytes_to_send;
    }
}


MJPG_HTTP_STATUS Init_Streaming_Data_Send(void)
{
    //uint32_t status  = NX_SUCCESS;
    int frame_size=0;
    uint16_t head_len=0;


    send_data_over_tcp(http_streamer_payload,strlen(http_streamer_payload));

    frame_size=sizeof(s_arrImage);
    memset(buffer,0,sizeof(buffer));
	sprintf(buffer, "Content-Type: image/jpeg\r\n"\
                   "Content-Length: %d\r\n\r\n", frame_size);	    
    send_data_over_tcp(buffer,strlen(buffer));

    memset(buffer,0,sizeof(buffer));
    memcpy(&buffer[0],(char*)s_arrImage,frame_size); 
    send_data_over_tcp(buffer,frame_size);
    //APP_PRINT("\r\n%s %d\r\n",__FUNCTION__,__LINE__);
	return MJPG_HTTP_OK;
}


UCHAR pin_level =0;
MJPG_HTTP_STATUS Streaming_Data_Send(void)
{
    int32_t frame_size=0,send_sta;
    uint16_t head_len=0;

	
    sprintf(tmp, "\r\n--ramcu\r\n"\
                    "Content-Type: image/jpeg\r\n"\
                "Content-Length: %d\r\n\r\n", frame_size);
    head_len=strlen(tmp);


    send_data_over_tcp(tmp,strlen(tmp));
    //APP_PRINT("\r\n%s %d\r\n",__FUNCTION__,__LINE__);

    tx_semaphore_get(&g_http_data_ready, TX_WAIT_FOREVER);
    frame_size = pstreamer_data->len[pstreamer_data->index];
    if(pstreamer_data->index==0)
    {

        send_data_over_tcp(pstreamer_data->http_jpg_data_left,frame_size);
    }
    else
    {
        send_data_over_tcp(pstreamer_data->http_jpg_data_right,frame_size);

    }
    //APP_PRINT("\r\nframe_size is %x pstreamer_data->index is %x %s %d",frame_size,pstreamer_data->index,__FUNCTION__,__LINE__);

    tx_thread_sleep (5);
    R_BSP_PinWrite(BSP_IO_PORT_10_PIN_01, pin_level);
    pin_level = !pin_level;
    return MJPG_HTTP_OK;
}
/*******************************************************************************************************************//**
 * @} (end addtogroup NetX_dhcpv4_client_ep)
 **********************************************************************************************************************/

