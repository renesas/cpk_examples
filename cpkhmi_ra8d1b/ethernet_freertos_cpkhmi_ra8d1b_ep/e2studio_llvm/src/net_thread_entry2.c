/**
 * FreeRTOS-TCP/UDP Server for Renesas RA8D1
 *
 * This program creates a simple TCP and UDP server on the RA8D1 development board
 * using FreeRTOS-TCP components. The server can handle both TCP and UDP connections
 * for performance testing from Linux or Windows clients.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "common_utils.h"

#define safe_printf APP_PRINT

/* Network configuration parameters */
#define TCP_SERVER_PORT        5001    /* TCP server port */
#define UDP_SERVER_PORT        5002    /* UDP server port */
#define MAX_TCP_CLIENTS        5       /* Maximum number of TCP clients */
#define RX_BUFFER_SIZE         1460    /* Buffer size for receiving data */
#define TX_BUFFER_SIZE         1460    /* Buffer size for sending data */
#define TASK_STACK_SIZE        1024   /* Stack size for tasks */
#define TCP_TASK_PRIORITY      3       /* Priority for TCP task */
#define UDP_TASK_PRIORITY      3       /* Priority for UDP task */

/* Task function prototypes */
static void prvTCPServerTask(void *pvParameters);
static void prvUDPServerTask(void *pvParameters);

/* Buffer for network communication */
static char ucRxBuffer[RX_BUFFER_SIZE];
static char ucTxBuffer[TX_BUFFER_SIZE];

/* Statistics counters */
static uint32_t ulTCPBytesReceived = 0;
static uint32_t ulTCPBytesSent = 0;
static uint32_t ulUDPPacketsReceived = 0;
static uint32_t ulUDPBytesSent = 0;

/**
 * Network event hook - This function is called by the FreeRTOS-TCP stack when network events occur
 */
void create_tcp_udp_server(void)
{
    static BaseType_t xTasksAlreadyCreated = pdFALSE;
    static TaskHandle_t xTCPTaskHandle = NULL;
    static TaskHandle_t xUDPTaskHandle = NULL;

    /* If network is up and tasks not yet created, create server tasks */
    if (xTasksAlreadyCreated == pdFALSE)
    {
         safe_printf("Network is up. Starting server tasks...\r\n");

         /* Create the TCP server task */
         BaseType_t xTCPTaskCreated = xTaskCreate(prvTCPServerTask, "TCPServer",
                                         TASK_STACK_SIZE, NULL, TCP_TASK_PRIORITY, &xTCPTaskHandle);
         if (xTCPTaskCreated != pdPASS)
         {
             safe_printf("Failed to create TCP server task\r\n");
         }
         else
         {
             safe_printf("TCP server task created successfully\r\n");
         }

         /* Create the UDP server task */
         BaseType_t xUDPTaskCreated = xTaskCreate(prvUDPServerTask, "UDPServer", TASK_STACK_SIZE,
                                                  NULL, UDP_TASK_PRIORITY, &xUDPTaskHandle);
         if (xUDPTaskCreated != pdPASS)
         {
             safe_printf("Failed to create UDP server task\r\n");
         }
         else
         {
             safe_printf("UDP server task created successfully\r\n");
         }

         /* Only set flag if both tasks were created successfully */
         if (xTCPTaskCreated == pdPASS && xUDPTaskCreated == pdPASS)
         {
             xTasksAlreadyCreated = pdTRUE;
             safe_printf("All server tasks started successfully\r\n");
         }
    }
}

/**
 * TCP Server Task - Handles TCP connections and data exchange
 */
static void prvTCPServerTask(void *pvParameters)
{
    Socket_t xListeningSocket, xClientSocket;
    struct freertos_sockaddr xBindAddress, xClientAddress;
    socklen_t xClientAddressLength = sizeof(xClientAddress);
    const TickType_t xReceiveTimeOut = pdMS_TO_TICKS(5000);
    const BaseType_t xBacklog = MAX_TCP_CLIENTS;
    BaseType_t xReceivedBytes;

    /* Create a socket */
    xListeningSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (xListeningSocket == FREERTOS_INVALID_SOCKET)
    {
        safe_printf("Failed to create TCP socket\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* Set socket options */
    FreeRTOS_setsockopt(xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof(xReceiveTimeOut));

    /* Bind the socket to the server port */
    xBindAddress.sin_port = FreeRTOS_htons(TCP_SERVER_PORT);
    xBindAddress.sin_port = FreeRTOS_htons(TCP_SERVER_PORT);
    xBindAddress.sin_addr = FreeRTOS_inet_addr_quick(0, 0, 0, 0); // Accept connections from any address

    if (FreeRTOS_bind(xListeningSocket, &xBindAddress, sizeof(xBindAddress)) != 0)
    {
        safe_printf("Failed to bind TCP socket\r\n");
        FreeRTOS_closesocket(xListeningSocket);
        vTaskDelete(NULL);
        return;
    }

    /* Put the socket into listening mode */
    if (FreeRTOS_listen(xListeningSocket, xBacklog) != 0)
    {
        safe_printf("Failed to listen on TCP socket\r\n");
        FreeRTOS_closesocket(xListeningSocket);
        vTaskDelete(NULL);
        return;
    }

    safe_printf("TCP server started on port %d\r\n", TCP_SERVER_PORT);
    vTaskDelay(1000);

    /* Process incoming connections */
    for (;;)
    {
        /* Accept incoming connections */
        xClientSocket = FreeRTOS_accept(xListeningSocket, &xClientAddress, &xClientAddressLength);

        if (xClientSocket != FREERTOS_INVALID_SOCKET)
        {
            uint32_t ulClientIP = FreeRTOS_ntohl(xClientAddress.sin_addr);
            uint16_t usClientPort = FreeRTOS_ntohs(xClientAddress.sin_port);

            safe_printf("TCP: New connection from %d.%d.%d.%d:%d\r\n",
                (int)(ulClientIP >> 24) & 0xff,
                (int)(ulClientIP >> 16) & 0xff,
                (int)(ulClientIP >> 8) & 0xff,
                (int)ulClientIP & 0xff,
                usClientPort);

            /* Process data from this client */
            for (;;)
            {
                /* Receive data */
                xReceivedBytes = FreeRTOS_recv(xClientSocket, ucRxBuffer, RX_BUFFER_SIZE, 0);

                if (xReceivedBytes <= 0)
                {
                    /* Connection closed or error */
                    break;
                }

                /* Update statistics */
                ulTCPBytesReceived += xReceivedBytes;

                /* Echo the data back to the client (simple echo server) */
                //FreeRTOS_send(xClientSocket, ucRxBuffer, xReceivedBytes, 0);

                /* Update statistics */
                ulTCPBytesSent += xReceivedBytes;

                /* Print statistics periodically */
                // safe_printf("TCP: Received %lu bytes, Sent %lu bytes\r\n",
                //        ulTCPBytesReceived, ulTCPBytesSent);
            }

            /* Close the client socket */
            FreeRTOS_closesocket(xClientSocket);
            safe_printf("TCP: Connection closed\r\n");
        }
    }
}

static void prvUDPServerTask(void *pvParameters)
{
    Socket_t xUDPSocket;
    struct freertos_sockaddr xBindAddress, xClientAddress;
    socklen_t xClientAddressLength = sizeof(xClientAddress);
    const TickType_t xReceiveTimeOut = pdMS_TO_TICKS(5000);
    BaseType_t xReceivedBytes;

    safe_printf("UDP server task is start\r\n");

    /* 延迟一小段时间，确保网络栈已初始化 */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Create a socket */
    xUDPSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);

    if (xUDPSocket == FREERTOS_INVALID_SOCKET)
    {
        safe_printf("Failed to create UDP socket\r\n");
        vTaskDelete(NULL);
        return;
    }

    safe_printf("UDP socket created successfully\r\n");

    /* Set socket options */
    FreeRTOS_setsockopt(xUDPSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof(xReceiveTimeOut));

    /* Bind the socket to the server port */
    memset(&xBindAddress, 0, sizeof(xBindAddress));
    xBindAddress.sin_port = FreeRTOS_htons(UDP_SERVER_PORT);
    xBindAddress.sin_addr = FreeRTOS_inet_addr_quick(0, 0, 0, 0); // Accept connections from any address

    BaseType_t xBindResult = FreeRTOS_bind(xUDPSocket, &xBindAddress, sizeof(xBindAddress));
    if (xBindResult != 0)
    {
        safe_printf("Failed to bind UDP socket, error: %ld\r\n", xBindResult);
        FreeRTOS_closesocket(xUDPSocket);
        vTaskDelete(NULL);
        return;
    }

    safe_printf("UDP server successfully bound to port %d\r\n", UDP_SERVER_PORT);
    safe_printf("UDP server is now listening for incoming packets\r\n");

    /* Initialize the client address */
    memset(&xClientAddress, 0, sizeof(xClientAddress));

    /* Process incoming UDP packets */
    for (;;)
    {
        /* Receive data */
        xReceivedBytes = FreeRTOS_recvfrom(xUDPSocket, ucRxBuffer, RX_BUFFER_SIZE, 0,
                                          &xClientAddress, &xClientAddressLength);
        if (xReceivedBytes > 0)
        {
            uint32_t ulClientIP = FreeRTOS_ntohl(xClientAddress.sin_addr);
            uint16_t usClientPort = FreeRTOS_ntohs(xClientAddress.sin_port);

            /* Update statistics */
            ulUDPPacketsReceived++;

            /* Echo the data back to the client */
            BaseType_t xSendResult = FreeRTOS_sendto(xUDPSocket, ucRxBuffer, xReceivedBytes, 0,
                                                    &xClientAddress, xClientAddressLength);

            if (xSendResult >= 0)
                ulUDPBytesSent += xReceivedBytes;

            /* Print statistics */
            // safe_printf("UDP : Receive %lu packages, send data %lu\r\n",
            //    ulUDPPacketsReceived, ulUDPBytesSent);
        }
        else if (xReceivedBytes < 0)
        {
            if (xReceivedBytes != -pdFREERTOS_ERRNO_EAGAIN)
                safe_printf("UDP: Error receiving data: %ld\r\n", (long)xReceivedBytes);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}


