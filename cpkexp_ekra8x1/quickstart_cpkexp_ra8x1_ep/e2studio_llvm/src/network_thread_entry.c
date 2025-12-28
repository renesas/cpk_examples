/***********************************************************************************************************************
* Copyright (c) 2023 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : network_thread_entry.c
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/

#include "network_thread.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP.h"
#include "core_http_client.h"
#include "transport_mbedtls_pkcs11.h"
#include "mbedtls/debug.h"
#include "log.h"

#include "board_cfg.h"
#include "common_init.h"

#include "iot/root_ca.h"

#define ENABLE_CONSOLE  (0)
#define DEBUG_HTTPS     (0)
#define DEBUG_HTTPS_WEATHER		1

#define CONNECTION_ABORT_CRTL              (0x00)
#define MENU_EXIT_CRTL                     (0x20)
#define BACKSPACE                          (0x08)
#define CARRIAGE_RETURN                    (0x0D)
#define TRANSACTION_DELAY                  (1000) /* Number Milliseconds between https calls (6 sec) */

#define ABORTING_MESSAGE                   "\r\nAborting data retrieval\r\n"

#define HTTPS_GET_API_WEATHER_HDR          "/v1/forecast.json?key="

#define HTTPS_GET_API_LOCAL_TIME_HDR       "/v1/timezone.json?key="
#define HTTPS_GET_API_LOCAL_TIME_FOOTER    "&q=London"

#define ACTIVE_KEY_NULL                    "cur_live_none"

#define UPDATE_CURRENCY_DATA               (0)

#define RESET_VALUE                        (0x00)
#define USER_BUFF                          (4096)
#define RES_USER_BUFF                      (32 * 1024)

/** HOST Address **/

#define HTTP_REQUEST_KEEP_ALIVE_FLAG       0x1U

#define HTTPS_CONNECTION_NUM_RETRY         ((uint32_t) 5)

#define SOCKET_SEND_RECV_TIME_OUT_MS       ((uint32_t) 10000)

const char * p_char_timezone_prekey = "/v1/timezone.json?key=";

struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

extern void   eth_abort(void);
extern bool_t update_key_data(e_hp_data_flash_iot_keys_t id);

extern uint8_t * stored_api_key;

char_t         p_api_key_target_string[100] = {0};
char_t         newpath[100]                 = {0};
uint32_t     * tptr      = NULL;
uint32_t       test[32U] = {0U};
uint32_t       dummydata = 0xAABBCCDD;
flash_status_t flash_status;
uint8_t      * stored_api_key = NULL;

e_https_certificate_t g_https_certificate = API_CERTIFICATE_CURRENCYAPI;
static char_t         s_net_buffer[32768] __attribute__((section(".sdram_noinit")));

uint8_t ucMACAddress[6]       = {0x00, 0x11, 0x22, 0x33, 0x44, 0x98};
uint8_t ucIPAddress[4]        = {192, 168, 0, 90};
uint8_t ucNetMask[4]          = {255, 255, 255, 0};
uint8_t ucGatewayAddress[4]   = {192, 168, 0, 1};
uint8_t ucDNSServerAddress[4] = {192, 168, 0, 1};

TransportInterface_t xTransportInterface          = {RESET_VALUE};
NetworkContext_t     xNetworkContext              = {RESET_VALUE};
uint8_t              resUserBuffer[RES_USER_BUFF] = {RESET_VALUE};
uint8_t              reqUserBuffer[USER_BUFF]     = {RESET_VALUE};

NetworkInterface_t xInterfaces[1];
NetworkEndPoint_t  xEndPoints[1];

/* #HTTPClient_InitializeRequestHeaders. */
HTTPRequestInfo_t xRequestInfo = {RESET_VALUE};

/* Represents a response returned from an HTTP server. */
HTTPResponse_t xResponse = {RESET_VALUE};

/* Represents header data that will be sent in an HTTP request. */
HTTPRequestHeaders_t xRequestHeaders = {RESET_VALUE};

NetworkCredentials_t connConfig                = {RESET_VALUE};
TlsTransportParams_t xPlaintextTransportParams = {RESET_VALUE};

uint32_t volatile             dhcp_in_use = RESET_VALUE;
IPV4Parameters_t xNd = { RESET_VALUE };
void print_ipconfig(void);

static void GetNetworkWeather(void);
static void GetNetworkTime(void);
static void GetNetworkCurrency(void);

HTTPStatus_t add_header(HTTPRequestHeaders_t * pRequestHeaders);
HTTPStatus_t connect_https_client(NetworkContext_t    * NetworkContext,
                                  const unsigned char * pTrustedRootCA,
                                  uint16_t              certSize);

static e_hp_data_flash_iot_keys_t s_active_website = IOT_KEY_NONE;

extern NetworkInterface_t * pxFSP_Eth_FillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t * pxInterface);
extern void                 update_current_weather_data(char * p_src, size_t src_len, uint32_t region);
extern void                 update_currency_data_tables(char * p_src, size_t src_len, uint32_t region);
extern void                 update_current_time_data(char * p_src, size_t src_len);
extern bool_t               is_key_data_available(e_hp_data_flash_iot_keys_t id);
extern bool_t               get_key_data(e_hp_data_flash_iot_keys_t id, char_t * active_key);

static void GetNetworkTime (void)
{
    HTTPStatus_t httpsClientStatus = HTTPSuccess;

    /* Initialize HTTPS client with presigned URL */
    httpsClientStatus = connect_https_client(&xNetworkContext,
                                             (const unsigned char *) HTTPS_TRUSTED_ROOT_CA_WEATHER,
                                             sizeof(HTTPS_TRUSTED_ROOT_CA_WEATHER));

    /* Handle_error */
    if (HTTPSuccess != httpsClientStatus)
    {
        sprintf(s_net_buffer, "\r\nFailed in server connection establishment");
    #if ENABLE_CONSOLE
        print_to_console(s_net_buffer);
    #endif
        mbedtls_platform_teardown(NULL);
    }
    else
    {
        xTransportInterface.pNetworkContext = &xNetworkContext;
        xTransportInterface.send            = TLS_FreeRTOS_send;
        xTransportInterface.recv            = TLS_FreeRTOS_recv;
    }

    bool_t retry = true;
    while (retry == true)
    {
        /* Initialize all HTTP Client library API structs to 0. */
        (void) memset(&xRequestInfo, 0, sizeof(HTTPRequestInfo_t));
        (void) memset(&xResponse, 0, sizeof(HTTPResponse_t));
        (void) memset(&xRequestHeaders, 0, sizeof(HTTPRequestHeaders_t));

        char_t TimeAPIPath[256] = "";
        char_t active_key[64]   = "";

        get_key_data(s_active_website, (char_t *) &active_key);
        sprintf(TimeAPIPath, "%s%s%s", HTTPS_GET_API_LOCAL_TIME_HDR, active_key, HTTPS_GET_API_LOCAL_TIME_FOOTER);

        /* Initialize the request object. */
        xRequestInfo.pPath   = (const char *) &TimeAPIPath; /* Apply stored key */
        xRequestInfo.pathLen = strlen(TimeAPIPath);

        xRequestInfo.pHost     = HTTPS_WEATHER_HOST_ADDRESS;
        xRequestInfo.hostLen   = strlen(HTTPS_WEATHER_HOST_ADDRESS);
        xRequestInfo.pMethod   = HTTP_METHOD_GET;
        xRequestInfo.methodLen = strlen(HTTP_METHOD_GET);

        /* Set "Connection" HTTP header to "keep-alive" so that multiple requests
         * can be sent over the same established TCP connection. */
        xRequestInfo.reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;

        /* Set the buffer used for storing request headers. */

        xRequestHeaders.pBuffer   = reqUserBuffer;
        xRequestHeaders.bufferLen = sizeof(reqUserBuffer);
        memset(xRequestHeaders.pBuffer, 0, xRequestHeaders.bufferLen);

        httpsClientStatus = HTTPClient_InitializeRequestHeaders(&xRequestHeaders, &xRequestInfo);

        /* Add header */
        if (httpsClientStatus == HTTPSuccess)
        {
            httpsClientStatus = add_header(&xRequestHeaders);
        }
        else
        {
            sprintf(s_net_buffer, "Failed to initialize HTTP request headers: Error=%s. \r\n", HTTPClient_strerror(httpsClientStatus));
        #if ENABLE_CONSOLE
            print_to_console(s_net_buffer);
        #endif
        }

        xResponse.pBuffer   = resUserBuffer;
        xResponse.bufferLen = sizeof(resUserBuffer);
        memset(xResponse.pBuffer, 0, xResponse.bufferLen);

        if (httpsClientStatus == HTTPSuccess)
        {
            httpsClientStatus = HTTPClient_Send(&xTransportInterface, &xRequestHeaders, NULL, 0, &xResponse, 0);
        }

    #if DEBUG_HTTPS
        if (httpsClientStatus == HTTPSuccess)
        {
            print_to_console("xRequestHeaders.pBuffer:\r\n");
            print_to_console((char_t *)xRequestHeaders.pBuffer);

            sprintf(s_net_buffer, "Received HTTP response from\r\n%s %s...\r\n", HTTPS_WEATHER_HOST_ADDRESS, xRequestInfo.pPath);
            print_to_console(s_net_buffer);

            sprintf(s_net_buffer, "Response Headers:\r\n%s\r\n", xResponse.pHeaders);
            print_to_console(s_net_buffer);

            sprintf(s_net_buffer, "Status Code:%u\r\n", xResponse.statusCode);
            print_to_console(s_net_buffer);
            sprintf(s_net_buffer, "Response Body:\r\n%s\r\n", xResponse.pBody);
            print_to_console(s_net_buffer);
        }
        else
        {
            sprintf(s_net_buffer,
                    "Failed to send HTTP %s request to %s%s: Error=%s.",
                    xRequestInfo.pMethod,
                    HTTPS_WEATHER_HOST_ADDRESS,
                    xRequestInfo.pPath,
                    HTTPClient_strerror(httpsClientStatus));
            print_to_console(s_net_buffer);
        }
    #endif
        if (HTTPSuccess != httpsClientStatus)
        {
        #if ENABLE_CONSOLE
            sprintf(s_net_buffer, "** Failed in GET Request ** \r\n");
            print_to_console(s_net_buffer);
            sprintf(s_net_buffer, " \r\nReturned Error Code: 0x%x  \r\n", httpsClientStatus);
            print_to_console(s_net_buffer);
        #endif
            if (false == update_key_data(s_active_website))
            {
                // print_to_console(ABORTING_MESSAGE);
                // print_to_console("\r\nPress space bar to return to MENU.\r\n");
                eth_abort();
                retry = false;
            }
        }
        else
        {
            char_t err_mess[]  = "\"message\":\"";
            char_t err_mess2[] = "<html>";

            char_t * ptr  = strstr((char_t *) xResponse.pBody, err_mess);
            char_t * ptr2 = strstr((char_t *) xResponse.pBody, err_mess2);

            sprintf(s_net_buffer, "GetNetworkTime Request Header = %s\r\n", xRequestHeaders.pBuffer);

            // print_to_console(s_net_buffer);

            if ((ptr != NULL) || (ptr2 != NULL) || (xResponse.bodyLen == 0))
            {
                // print_to_console("KEY BAD SKIPPING UPDATE\r\n");
                if (false == update_key_data(s_active_website))
                {
                    // print_to_console(ABORTING_MESSAGE);
                    // print_to_console("\r\nPress space bar to return to MENU.\r\n");
                    eth_abort();
                    retry = false;
                }
            }
            else
            {
                if (xResponse.bodyLen > 0)
                {
                    sprintf(s_net_buffer, "GetNetworkTime Received data using GET Request = %s\r\n", xResponse.pBody);
                }
                else
                {
                    sprintf(s_net_buffer, "Bad Message : No message body\r\n");
                }

            #if ENABLE_CONSOLE
                print_to_console("KEY OK UPDATING TABLES\r\n");
                print_to_console(s_net_buffer);
            #endif
                update_current_time_data((char_t *) xResponse.pBody, xResponse.bodyLen);

                print_to_console("\r\n数据刷新成功\r\n");

                print_to_console("\r\n按空格返回菜单，然后按 2 运行交互式 AI, 连接性与人机接口演示");
                print_to_console("\r\n可在 MIPI 屏幕上看到更新的数据\r\n");
                retry = false;
            }
        }
    }

    TLS_FreeRTOS_Disconnect(&xNetworkContext);

    mbedtls_platform_teardown(NULL);
}

const char https_get_request_weather[][64] =
{
    {"&q=hong_kong&days=1&aqi=no&alerts=no"    },
    {"&q=kyoto&days=1&aqi=no&alerts=no"        },
    {"&q=london&days=1&aqi=no&alerts=no"       },
    {"&q=miami&days=1&aqi=no&alerts=no"        },
    {"&q=munich&days=1&aqi=no&alerts=no"       },
    {"&q=new_york&days=1&aqi=no&alerts=no"     },
    {"&q=paris&days=1&aqi=no&alerts=no"        },
    {"&q=prague&days=1&aqi=no&alerts=no"       },
    {"&q=queenstown&days=1&aqi=no&alerts=no"   },
    {"&q=rio&days=1&aqi=no&alerts=no"          },
    {"&q=rome&days=1&aqi=no&alerts=no"         },
    {"&q=san_francisco&days=1&aqi=no&alerts=no"},
    {"&q=shanghai&days=1&aqi=no&alerts=no"     },
    {"&q=singapore&days=1&aqi=no&alerts=no"    },
    {"&q=sydney&days=1&aqi=no&alerts=no"       },
    {"&q=toronto&days=1&aqi=no&alerts=no"      }
};

static void GetNetworkWeather (void)
{
	char WeatherAPIPath[1024 + 128];

	char active_key[64] = "";
	char err_mess[]  = "\"message\":\"";
	char err_mess2[] = "<html>";
	char *ptr = NULL;
	char *ptr2 = NULL;
    HTTPStatus_t httpsClientStatus = HTTPSuccess;
    uint32_t region = 0;

    /* Initialize HTTPS client with presigned URL */
    httpsClientStatus = connect_https_client(&xNetworkContext, (const unsigned char *)HTTPS_TRUSTED_ROOT_CA_WEATHER, sizeof(HTTPS_TRUSTED_ROOT_CA_WEATHER));
    if (HTTPSuccess != httpsClientStatus) {
    		LOG_E(__FUNCTION__, "Failed in server connection establishment.");
        mbedtls_platform_teardown(NULL);
    }
    else {
        xTransportInterface.pNetworkContext = &xNetworkContext;
        xTransportInterface.send = TLS_FreeRTOS_send;
        xTransportInterface.recv = TLS_FreeRTOS_recv;
    }

    LOG_D(__FUNCTION__, "Start");
    for (region = 0; region < 16; ) {
        /* Initialize all HTTP Client library API structs to 0. */
        memset(&xRequestInfo, 0, sizeof(HTTPRequestInfo_t));
        memset(&xResponse, 0, sizeof(HTTPResponse_t));
        memset(&xRequestHeaders, 0, sizeof(HTTPRequestHeaders_t));

        get_key_data(s_active_website, (char_t *) &active_key);
        sprintf(WeatherAPIPath, "%s%s%s", HTTPS_GET_API_WEATHER_HDR, active_key, https_get_request_weather[region]);

        /* Initialize the request object. */
        xRequestInfo.pPath = (const char *) &WeatherAPIPath; /* Apply stored key */
        xRequestInfo.pathLen = strlen(WeatherAPIPath);
        xRequestInfo.pHost = HTTPS_WEATHER_HOST_ADDRESS;
        xRequestInfo.hostLen = strlen(HTTPS_WEATHER_HOST_ADDRESS);
        xRequestInfo.pMethod = HTTP_METHOD_GET;
        xRequestInfo.methodLen = strlen(HTTP_METHOD_GET);

        /* Set "Connection" HTTP header to "keep-alive" so that multiple requests
         * can be sent over the same established TCP connection. */
        xRequestInfo.reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;

        /* Set the buffer used for storing request headers. */

        xRequestHeaders.pBuffer = reqUserBuffer;
        xRequestHeaders.bufferLen = sizeof(reqUserBuffer);
        memset(xRequestHeaders.pBuffer, 0, xRequestHeaders.bufferLen);

        httpsClientStatus = HTTPClient_InitializeRequestHeaders(&xRequestHeaders, &xRequestInfo);
        if (httpsClientStatus == HTTPSuccess) {
        		LOG_D(__FUNCTION__, "Add header");
            httpsClientStatus = add_header(&xRequestHeaders);
        }
        else {
        		LOG_E(__FUNCTION__, "Failed to initialize HTTP request headers: %s", HTTPClient_strerror(httpsClientStatus));
        }

        xResponse.pBuffer   = resUserBuffer;
        xResponse.bufferLen = sizeof(resUserBuffer);
        memset(xResponse.pBuffer, 0, xResponse.bufferLen);

        if (httpsClientStatus == HTTPSuccess) {
            httpsClientStatus = HTTPClient_Send(&xTransportInterface, &xRequestHeaders, NULL, 0, &xResponse, 0);
        }
        else {
        		LOG_E(__FUNCTION__, "Add header faild");
        }

    #if DEBUG_HTTPS_WEATHER
        if (httpsClientStatus == HTTPSuccess) {
        		LOG_D(__FUNCTION__, "Request:");
        		LOG_PutsEndl((char *)xRequestHeaders.pBuffer);
        }
    #endif

        if (HTTPSuccess != httpsClientStatus) {
        		LOG_E(__FUNCTION__, "Failed in GET Request: %d", httpsClientStatus);
            if (false == update_key_data(s_active_website)) {
                LOG_E(__FUNCTION__, "Update API key faild");
                eth_abort();
                break;
            }
        }
        else {
        		LOG_D(__FUNCTION__, "Response length: %u", xResponse.bodyLen);
            LOG_D(__FUNCTION__, "Response body:");
            LOG_PutsEndl((const char *)xResponse.pBody);

            ptr = strstr((char_t *) xResponse.pBody, err_mess);
            ptr2 = strstr((char_t *) xResponse.pBody, err_mess2);
            LOG_D(__FUNCTION__, "Request header: %s", xRequestHeaders.pBuffer);
            if ((ptr != NULL) || (ptr2 != NULL) || (xResponse.bodyLen == 0)) {
            		LOG_W(__FUNCTION__, "KEY BAD SKIPPING UPDATE");
                if (false == update_key_data(s_active_website)) {
                		LOG_E(__FUNCTION__, "Update API key faild");
                    eth_abort();
                    region = 18;
                    break;
                }
            }
            else {
                if (xResponse.bodyLen > 0) {
                		LOG_D(__FUNCTION__, "Response length: %u", xResponse.bodyLen);
                		LOG_D(__FUNCTION__, "Response body:");
                		LOG_PutsEndl((const char *)xResponse.pBody);
                }
                else {
                    sprintf(s_net_buffer, "Bad Message : No message body\r\n");
                    LOG_W(__FUNCTION__, "No response body");
                }
                update_current_weather_data((char *) xResponse.pBody, strlen((char_t *) xResponse.pBody), region);

                if (region == 0) {
                    print_to_console("刷新数据中");
                }

                region++;              // only move one once the current data is successful

                print_to_console(".");
            }
        }

        vTaskDelay(100);
    }

    print_to_console("\r\n");

    if (region == 16)
    {
        print_to_console("\r\n数据刷新成功\r\n");

        print_to_console("\r\n按空格返回菜单，然后按 2 运行交互式 AI, 连接性与人机接口演示");
        print_to_console("\r\n可在 MIPI 屏幕上看到更新的数据");
    }

    TLS_FreeRTOS_Disconnect(&xNetworkContext);

    mbedtls_platform_teardown(NULL);
}

static void GetNetworkCurrency (void)
{
    HTTPStatus_t httpsClientStatus = HTTPSuccess;

    char https_get_request[][128] =
    {
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=AUD"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=GBP"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=CAD"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=CNY"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=EUR"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=HKD"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=INR"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=JPY"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=SGD"},
        {"/v3/latest?currencies=GBP%2CEUR%2CUSD%2CCAD%2CHKD%2CJPY%2CSGD%2CAUD%2CINR%2CCNY&base_currency=USD"}
    };

    uint32_t region = 0;
    {
        LOG_D(__FUNCTION__, "Start");
        for (region = 0; region < 10; )
        {
            httpsClientStatus = HTTPSuccess;

            /* Initialize HTTPS client with presigned URL */
            LOG_D(__FUNCTION__, "Connect https client");
            httpsClientStatus = connect_https_client(&xNetworkContext,
                                                     (const unsigned char *) HTTPS_TRUSTED_ROOT_CA_CURRENCY,
                                                     sizeof(HTTPS_TRUSTED_ROOT_CA_CURRENCY));

            /* Handle_error */
            if (HTTPSuccess != httpsClientStatus)
            {
                LOG_E(__FUNCTION__, "Connect failed");
                mbedtls_platform_teardown(NULL);
            }
            else
            {
                LOG_D(__FUNCTION__, "Connect success");
                xTransportInterface.pNetworkContext = &xNetworkContext;
                xTransportInterface.send            = TLS_FreeRTOS_send;
                xTransportInterface.recv            = TLS_FreeRTOS_recv;
            }

            /* Initialize all HTTP Client library API structs to 0. */
            (void) memset(&xRequestInfo, 0, sizeof(HTTPRequestInfo_t));
            (void) memset(&xResponse, 0, sizeof(HTTPResponse_t));
            (void) memset(&xRequestHeaders, 0, sizeof(HTTPRequestHeaders_t));
            (void) memset(&resUserBuffer, 0, sizeof(RES_USER_BUFF));
            (void) memset(&reqUserBuffer, 0, sizeof(USER_BUFF));

            /* Initialize the request object. */
            xRequestInfo.pPath     = https_get_request[region];
            xRequestInfo.pathLen   = strlen(https_get_request[region]);
            xRequestInfo.pHost     = HTTPS_CURRENCY_HOST_ADDRESS;
            xRequestInfo.hostLen   = strlen(HTTPS_CURRENCY_HOST_ADDRESS);
            xRequestInfo.pMethod   = HTTP_METHOD_GET;
            xRequestInfo.methodLen = strlen(HTTP_METHOD_GET);

            /* Set "Connection" HTTP header to "keep-alive" so that multiple requests
             * can be sent over the same established TCP connection. */
            xRequestInfo.reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;

            /* Set the buffer used for storing request headers. */

            xRequestHeaders.pBuffer   = reqUserBuffer;
            xRequestHeaders.bufferLen = sizeof(reqUserBuffer);
            memset(xRequestHeaders.pBuffer, 0, xRequestHeaders.bufferLen);

            httpsClientStatus = HTTPClient_InitializeRequestHeaders(&xRequestHeaders, &xRequestInfo);

            /* Add header */
            if (httpsClientStatus == HTTPSuccess) {
                LOG_D(__FUNCTION__, "Initial req header success");
                httpsClientStatus = add_header(&xRequestHeaders);
            }
            else {
                LOG_E(__FUNCTION__, "Initial req header faild. Error: %s", HTTPClient_strerror(httpsClientStatus));
            }

            LOG_D(__FUNCTION__, "Fill Response");
            xResponse.pBuffer   = resUserBuffer;
            xResponse.bufferLen = sizeof(resUserBuffer);
            memset(xResponse.pBuffer, 0, xResponse.bufferLen);

            if (httpsClientStatus == HTTPSuccess) {
                LOG_D(__FUNCTION__, "Send request");
                httpsClientStatus = HTTPClient_Send(&xTransportInterface, &xRequestHeaders, NULL, 0, &xResponse, 0);
            }

        #if DEBUG_HTTPS
            if (httpsClientStatus == HTTPSuccess)
            {
                print_to_console("xRequestHeaders.pBuffer:\r\n");
                print_to_console((char_t *)xRequestHeaders.pBuffer);

                sprintf(s_net_buffer, "Received HTTP response from\r\n%s %s...\r\n", HTTPS_CURRENCY_HOST_ADDRESS, xRequestInfo.pPath);
                print_to_console(s_net_buffer);

                sprintf(s_net_buffer, "Response Headers:\r\n%s", xResponse.pHeaders);
                print_to_console(s_net_buffer);

                sprintf(s_net_buffer, "Status Code:%u\r\n", xResponse.statusCode);
                print_to_console(s_net_buffer);
                sprintf(s_net_buffer, "Response Body:\r\n%s\r\n", xResponse.pBody);
                print_to_console(s_net_buffer);
            }
            else
            {
                sprintf(s_net_buffer,
                        "Failed to send HTTP %s request to %s%s: Error=%s.",
                        xRequestInfo.pMethod,
                        HTTPS_CURRENCY_HOST_ADDRESS,
                        xRequestInfo.pPath,
                        HTTPClient_strerror(httpsClientStatus));
                print_to_console(s_net_buffer);
            }
        #endif
            if (HTTPSuccess != httpsClientStatus) {
                LOG_E(__FUNCTION__, "Failed in GET Request");
                LOG_D(__FUNCTION__, "Update key");
                if (false == update_key_data(s_active_website)) {
                    LOG_E(__FUNCTION__, "Update key fail");
                    eth_abort();

                    region = 11;
                    break;
                }
            }
            else {
                char_t   err_mess[] = "\"message\":\"";
                char_t * ptr        = strstr((char_t *) xResponse.pBody, err_mess);

                LOG_D(__FUNCTION__, "Get request header:");
                LOG_PutsEndl((const char *)xRequestHeaders.pBuffer);

                if (ptr != NULL) {
                    if (false == update_key_data(s_active_website)) {
                        eth_abort();

                        region = 11;
                        break;
                    }
                }
                else {
                		LOG_D(__FUNCTION__, "Response length: %u", strlen((const char *)xResponse.pBody));
                		LOG_D(__FUNCTION__, "Response body:");
                		LOG_PutsEndl((const char *)xResponse.pBody);

                    update_currency_data_tables((char_t *) xResponse.pBody, strlen((char_t *) xResponse.pBody), region);

                    if (region == 0) {
                        print_to_console("更新数据中");
                    }
                    region++;          // only move one once the current data is successful
                    print_to_console(".");
                }
            }

            vTaskDelay(TRANSACTION_DELAY);
            TLS_FreeRTOS_Disconnect(&xNetworkContext);
            mbedtls_platform_teardown(NULL);
        }

        print_to_console("\r\n");
        if (region == 10) {
            print_to_console("\r\n数据刷新成功\r\n");
            print_to_console("\r\n按空格返回菜单，然后按 2 运行交互式 AI, 连接性与人机接口演示");
            print_to_console("\r\n可在 MIPI 屏幕上看到更新的数据");
        }
    }

    if (region == 11) {
        TLS_FreeRTOS_Disconnect(&xNetworkContext);
        mbedtls_platform_teardown(NULL);
    }
}

void network_thread_entry(void * pvParameters)
{
	EventBits_t uxBits;
    fsp_err_t ierr;

    BaseType_t status = pdFALSE;
    uint32_t ip_status = RESET_VALUE;

    FSP_PARAMETER_NOT_USED(pvParameters);

    memset(s_net_buffer, 0, 32768);
    vTaskDelay(100);
    while (1) {
        uxBits = xEventGroupWaitBits(g_update_console_event, STATUS_ENABLE_ETHERNET, pdFALSE, pdTRUE, 1);

        if ((uxBits & (STATUS_ENABLE_ETHERNET)) == (STATUS_ENABLE_ETHERNET)) {
            xEventGroupClearBits(g_update_console_event, STATUS_ENABLE_ETHERNET);
            break;
        }
        vTaskDelay(10);
    }

    // Take the PHY out of Reset
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_HIGH);
    R_BSP_PinAccessDisable();
    vTaskDelay(100);

    /* Initialize the crypto hardware acceleration. */
    /* Initialize mbedtls. */
    ierr = mbedtls_platform_setup(NULL);

    /* Error Handler */
    if (FSP_SUCCESS != ierr)
    {
        print_to_console("mbedtls_platform_setup error\r\n");
    }

#if (1 == ipconfigIPv4_BACKWARD_COMPATIBLE)

    /* FreeRTOS IP initializes the IP stack  */
    status = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

    /* Error Handler */
    if (pdFALSE == status)
    {
        __BKPT(0);
    }

#else

    /* IF the following function should be declared in the NetworkInterface.c
     * linked in the project. */
    xInterfaces[0].pvArgument = &g_freertos_plus_tcp0;
    pxFSP_Eth_FillInterfaceDescriptor(0, &(xInterfaces[0]));
    FreeRTOS_FillEndPoint(&(xInterfaces[0]), &(xEndPoints[0]), ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
 #if (ipconfigUSE_DHCP != 0)
    {
        xEndPoints[0].bits.bWantDHCP = pdTRUE;
    }
 #endif                                /* ipconfigUSE_DHCP */

    vTaskDelay(100);
    status = FreeRTOS_IPInit_Multi();
    if (pdFALSE == status)
    {
        print_to_console("FreeRTOS_IPInit_Multi error\r\n");
    }
#endif

    // print_ipconfig();

    // sprintf(s_net_buffer, "等待网络启动事件...");
    // print_to_console(s_net_buffer);

    vTaskDelay(100);
    status = pdFALSE;
    while (status == pdFALSE)
    {
        status = xTaskNotifyWait(pdFALSE, pdFALSE, &ip_status, 0x3000);
        vTaskDelay(100);
    }

    vTaskDelay(100);
    LOG_D(__FUNCTION__, "Wait Event: STATUS_ENABLE_ETHERNET");
    while (1)
    {
        uxBits = xEventGroupWaitBits(g_update_console_event, STATUS_ENABLE_ETHERNET, pdFALSE, pdTRUE, 1);

        if ((uxBits & (STATUS_ENABLE_ETHERNET)) == (STATUS_ENABLE_ETHERNET))
        {
            xEventGroupClearBits(g_update_console_event, STATUS_ENABLE_ETHERNET);
            LOG_D(__FUNCTION__, "Get Event: STATUS_ENABLE_ETHERNET");
            break;
        }

        vTaskDelay(10);
    }

    /*
     * It's no necessary in FSP6.2.0
    status = pdFALSE;
    LOG_D(__FUNCTION__, "Wait TaskNotify");
    while (status == pdFALSE)
    {
        status = xTaskNotifyWait(pdFALSE, pdFALSE, &ip_status, 0x30000);
        vTaskDelay(100);
    }
    LOG_D(__FUNCTION__, "Get TaskNotify");
    */

    print_ipconfig();

    while (1)
    {
        vTaskDelay(1);

        uxBits = xEventGroupWaitBits(g_update_console_event,
                                     STATUS_IOT_REQUEST_WEATHER | STATUS_IOT_REQUEST_CURRENCY,
                                     pdFALSE,
                                     pdTRUE,
                                     1);

        if ((uxBits & (STATUS_IOT_REQUEST_TIME)) == (STATUS_IOT_REQUEST_TIME))
        {
            xEventGroupClearBits(g_update_console_event, STATUS_IOT_REQUEST_TIME);

            // check for valid credentials
            s_active_website = IOT_KEY_WEATHER;

            // is_key_data_available(s_active_website); // Time uses weather key

            /* Set up time request */
            GetNetworkTime();
            xEventGroupSetBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);
        }

        if ((uxBits & (STATUS_IOT_REQUEST_WEATHER)) == (STATUS_IOT_REQUEST_WEATHER))
        {
            // check for valid credentials
            s_active_website = IOT_KEY_WEATHER;

            xEventGroupClearBits(g_update_console_event, STATUS_IOT_REQUEST_WEATHER);

            /* Set up weather request */
            GetNetworkWeather();
            xEventGroupSetBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);
        }

        if ((uxBits & (STATUS_IOT_REQUEST_CURRENCY)) == (STATUS_IOT_REQUEST_CURRENCY))
        {
            // check for valid credentials
            s_active_website = IOT_KEY_CURRENCY;

            xEventGroupClearBits(g_update_console_event, STATUS_IOT_REQUEST_CURRENCY);
            LOG_D(__FUNCTION__, "Get Event: STATUS_IOT_REQUEST_CURRENCY");
            /* Set up currency request */
            GetNetworkCurrency();
            xEventGroupSetBits(g_update_console_event, STATUS_IOT_RESPONSE_COMPLETE);
        }
    }
}

HTTPStatus_t connect_https_client (NetworkContext_t    * NetworkContext,
                                   const unsigned char * pTrustedRootCA,
                                   uint16_t              certSize)
{
    HTTPStatus_t         httpsClientStatus  = HTTPSuccess;
    TlsTransportStatus_t TCP_connect_status = TLS_TRANSPORT_SUCCESS;

    /* The current attempt in the number of connection tries. */
    uint32_t     connAttempt = RESET_VALUE;
    const char * pHostAddr   = NULL;
    assert(NetworkContext != NULL);

    if (0 == memcmp(pTrustedRootCA, HTTPS_TRUSTED_ROOT_CA_WEATHER, sizeof(HTTPS_TRUSTED_ROOT_CA_WEATHER)))
    {
        pHostAddr = HTTPS_WEATHER_HOST_ADDRESS;
    }
    else
    {
        pHostAddr = HTTPS_CURRENCY_HOST_ADDRESS;
    }

    (void) memset(&connConfig, 0U, sizeof(NetworkCredentials_t));
    (void) memset(NetworkContext, 0U, sizeof(NetworkContext_t));
    NetworkContext->pParams = &xPlaintextTransportParams;

    /* Set the connection configurations. */
    connConfig.disableSni       = pdFALSE;
    connConfig.pRootCa          = pTrustedRootCA;
    connConfig.rootCaSize       = certSize;
    connConfig.pUserName        = NULL;
    connConfig.userNameSize     = 0;
    connConfig.pPassword        = NULL;
    connConfig.passwordSize     = 0;
    connConfig.pClientCertLabel = "myCertLabel";
    connConfig.pPrivateKeyLabel = "myKeyLabel";
    connConfig.pAlpnProtos      = NULL;

    /* Connect to server. */
    for (connAttempt = 1; connAttempt <= HTTPS_CONNECTION_NUM_RETRY; connAttempt++)
    {
        TCP_connect_status = TLS_FreeRTOS_Connect(NetworkContext,
                                                  pHostAddr,
                                                  HTTPS_PORT,
                                                  &connConfig,
                                                  SOCKET_SEND_RECV_TIME_OUT_MS,
                                                  SOCKET_SEND_RECV_TIME_OUT_MS);

        if ((TCP_connect_status != TLS_TRANSPORT_SUCCESS) && (connAttempt < HTTPS_CONNECTION_NUM_RETRY))
        {
            LOG_D(__FUNCTION__, "Failed to connect the server, retrying after 3000 ms.");
            LOG_D(__FUNCTION__, "TCP_connect_status = %d", TCP_connect_status);
            vTaskDelay(3000);
            continue;
        }
        else
        {
            break;
        }
    }

    if (TLS_TRANSPORT_SUCCESS != TCP_connect_status)
    {
        LOG_D(__FUNCTION__, "Unable to connect the server. Error code: %d.", TCP_connect_status);
        httpsClientStatus = HTTPNetworkError;
    }
    else
    {
        // print_to_console("\r\nConnected to the server\r\n");
    }

    return httpsClientStatus;
}

HTTPStatus_t add_header(HTTPRequestHeaders_t * pRequestHeaders)
{
	bool res;
    HTTPStatus_t Status;

    char active_key[64] = "";

    memset(s_net_buffer, '\0', sizeof(s_net_buffer));

    res = get_key_data(s_active_website, (char_t *) &active_key);
    if (res == false) {
    		LOG_W(__FUNCTION__, "Get key data fail");
    		return HTTPInvalidParameter;
    }
    LOG_D(__FUNCTION__, "Key data: %s", active_key);

    Status = HTTPClient_AddHeader(pRequestHeaders, "apikey", strlen("apikey"), active_key, strlen(active_key));

    if (Status != HTTPSuccess)
    {
        LOG_D(__FUNCTION__,
        		"An error occurred at adding Active Key in HTTPClient_AddHeader() with error code: Error=%s. ",
			HTTPClient_strerror(Status));
    }
    else {
        LOG_D(__FUNCTION__, "Add header success");
    }

    memset(s_net_buffer, '\0', sizeof(s_net_buffer));
    LOG_D(__FUNCTION__, "Clean s_net_buffer");

    return Status;
}

void print_ipconfig (void)
{
    if (dhcp_in_use)
    {
#if (ipconfigIPv4_BACKWARD_COMPATIBLE == 1)
        ucNetMask[3] = (uint8_t) ((xNd.ulNetMask & 0xFF000000) >> 24);
        ucNetMask[2] = (uint8_t) ((xNd.ulNetMask & 0x00FF0000) >> 16);
        ucNetMask[1] = (uint8_t) ((xNd.ulNetMask & 0x0000FF00) >> 8);
        ucNetMask[0] = (uint8_t) (xNd.ulNetMask & 0x000000FF);

        ucGatewayAddress[3] = (uint8_t) ((xNd.ulGatewayAddress & 0xFF000000) >> 24);
        ucGatewayAddress[2] = (uint8_t) ((xNd.ulGatewayAddress & 0x00FF0000) >> 16);
        ucGatewayAddress[1] = (uint8_t) ((xNd.ulGatewayAddress & 0x0000FF00) >> 8);
        ucGatewayAddress[0] = (uint8_t) (xNd.ulGatewayAddress & 0x000000FF);

        ucDNSServerAddress[3] = (uint8_t)((xNd.ulDNSServerAddresses[0] & 0xFF000000)>> 24);
        ucDNSServerAddress[2] = (uint8_t)((xNd.ulDNSServerAddresses[0] & 0x00FF0000)>> 16);
        ucDNSServerAddress[1] = (uint8_t)((xNd.ulDNSServerAddresses[0] & 0x0000FF00)>> 8);
        ucDNSServerAddress[0] = (uint8_t)(xNd.ulDNSServerAddresses[0] & 0x000000FF);

        ucIPAddress[3] = (uint8_t)((xNd.ulIPAddress & 0xFF000000) >> 24);
        ucIPAddress[2] = (uint8_t)((xNd.ulIPAddress & 0x00FF0000) >> 16);
        ucIPAddress[1] = (uint8_t)((xNd.ulIPAddress & 0x0000FF00) >> 8);
        ucIPAddress[0] = (uint8_t)(xNd.ulIPAddress & 0x000000FF);
#else
        ucNetMask[3] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulNetMask & 0xFF000000) >> 24);
        ucNetMask[2] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulNetMask & 0x00FF0000) >> 16);
        ucNetMask[1] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulNetMask & 0x0000FF00) >> 8);
        ucNetMask[0] = (uint8_t) (xEndPoints[0].ipv4_settings.ulNetMask & 0x000000FF);

        ucGatewayAddress[3] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulGatewayAddress & 0xFF000000) >> 24);
        ucGatewayAddress[2] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulGatewayAddress & 0x00FF0000) >> 16);
        ucGatewayAddress[1] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulGatewayAddress & 0x0000FF00) >> 8);
        ucGatewayAddress[0] = (uint8_t) (xEndPoints[0].ipv4_settings.ulGatewayAddress & 0x000000FF);

        ucDNSServerAddress[3] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulDNSServerAddresses[0] & 0xFF000000) >> 24);
        ucDNSServerAddress[2] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulDNSServerAddresses[0] & 0x00FF0000) >> 16);
        ucDNSServerAddress[1] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulDNSServerAddresses[0] & 0x0000FF00) >> 8);
        ucDNSServerAddress[0] = (uint8_t) (xEndPoints[0].ipv4_settings.ulDNSServerAddresses[0] & 0x000000FF);

        ucIPAddress[3] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulIPAddress & 0xFF000000) >> 24);
        ucIPAddress[2] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulIPAddress & 0x00FF0000) >> 16);
        ucIPAddress[1] = (uint8_t) ((xEndPoints[0].ipv4_settings.ulIPAddress & 0x0000FF00) >> 8);
        ucIPAddress[0] = (uint8_t) (xEndPoints[0].ipv4_settings.ulIPAddress & 0x000000FF);
#endif
    }

#ifdef ENABLE_CONSOLE
    sprintf(s_net_buffer, "\r\nRenesas %s 以太网适配器信息\r\n", KIT_NAME);
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer, "\t简要信息                  : Renesas %s Ethernet\r\n", KIT_NAME);
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer,
            "\t物理地址                  : %02x-%02x-%02x-%02x-%02x-%02x\r\n",
            ucMACAddress[0],
            ucMACAddress[1],
            ucMACAddress[2],
            ucMACAddress[3],
            ucMACAddress[4],
            ucMACAddress[5]);

    print_to_console(s_net_buffer);

    sprintf(s_net_buffer, "\tDHCP 启用                 : %s\r\n", dhcp_in_use ? "Yes" : "No");
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer,
            "\tIPv4 地址                 : %d.%d.%d.%d\r\n",
            ucIPAddress[0],
            ucIPAddress[1],
            ucIPAddress[2],
            ucIPAddress[3]);
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer,
            "\t子网掩码                  : %d.%d.%d.%d\r\n",
            ucNetMask[0],
            ucNetMask[1],
            ucNetMask[2],
            ucNetMask[3]);
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer,
            "\t默认网关                  : %d.%d.%d.%d\r\n",
            ucGatewayAddress[0],
            ucGatewayAddress[1],
            ucGatewayAddress[2],
            ucGatewayAddress[3]);
    print_to_console(s_net_buffer);

    sprintf(s_net_buffer,
            "\tDNS 服务器                : %d.%d.%d.%d\r\n",
            ucDNSServerAddress[0],
            ucDNSServerAddress[1],
            ucDNSServerAddress[2],
            ucDNSServerAddress[3]);
    print_to_console(s_net_buffer);
#endif
}

/**********************************************************************************************************************
 * End of function print_ipconfig
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: vApplicationMallocFailedHook
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
void vApplicationMallocFailedHook (void)
{
    __BKPT(0);
}

/**********************************************************************************************************************
 * End of function vApplicationMallocFailedHook
 *********************************************************************************************************************/
