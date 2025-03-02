/*
 * ra_usbx_host_uvc_common.h
 *
 *  Created on: 24-Nov-2022
 *      Author: chandrashekhar.d
 */

#ifndef RA_USBX_HOST_UVC_COMMON_H_
#define RA_USBX_HOST_UVC_COMMON_H_

#define ON                   (1U)
#define OFF                  (0U)
#define UX_ZERO              (0U)

#define BYTE_SIZE            (4U)

#define MEMPOOL_SIZE         (1024U*60)

#define NETXDUO_TCP_PORT     (1234)
#define NETXDUO_UDP_PORT     (1234)

#define JPEG_BUFFER_NUMBER   (2)
#define BUFFER_LEN           (2U)
#define UVC_VIDEO_PAYLOAD_HEADER_SIZE       (12)

#define G_UDP_CL_ADDRESS      (IP_ADDRESS(192,2,2,1))
/* Length of the header of each video data packet we send. */
#define VIDEO_PACKET_HEADER_LENGTH 22

#define CLASS_SPECIFIC_REQUEST   (1U)
/*On Board LEDs Pins */
// #define LED1_B BSP_IO_PORT_04_PIN_03            ///< LED1_B
// #define LED2_G BSP_IO_PORT_04_PIN_00            ///< LED2_G
// #define LED3_R BSP_IO_PORT_01_PIN_00            ///< LED3_R

#define LED1_B BSP_IO_PORT_06_PIN_00            ///< LED1_B
#define LED2_G BSP_IO_PORT_04_PIN_14            ///< LED2_G
#define LED3_R BSP_IO_PORT_01_PIN_07            ///< LED3_R


#define USB_CAMERA_DEVICE_CONNECT_FLAG     ((ULONG)0x0001)
#define USB_CAMERA_DEVICE_REMOVED_FLAG     ((ULONG)0x0000)

#define ARR_EL_COUNT(array) (sizeof(array)/sizeof(array[0]))

#define CHECKSUM_STEPS 5


#define EVENTFLAG_USB_DEVICE_INSERTED          (1U << 0)
#define EVENTFLAG_USB_DEVICE2_INSERTED         (1U << 1)
#define EVENTFLAG_USB_TRANSFER_REQUEST_DONE    (1U << 8)
#define EVENTFLAG_UVC_FRAME_DONE               (1U << 9)
#define EVENTFLAG_JPEG_DECODE_DONE             (1U << 10)

#define UVC_STREAM_HEADER_HEADERINFO_EOH       (1U << 7)
#define UVC_STREAM_HEADER_HEADERINFO_ERR       (1U << 6)
#define UVC_STREAM_HEADER_HEADERINFO_STI       (1U << 5)
#define UVC_STREAM_HEADER_HEADERINFO_RES       (1U << 4)
#define UVC_STREAM_HEADER_HEADERINFO_SCR       (1U << 3)
#define UVC_STREAM_HEADER_HEADERINFO_PTS       (1U << 2)
#define UVC_STREAM_HEADER_HEADERINFO_EOF       (1U << 1)
#define UVC_STREAM_HEADER_HEADERINFO_FID       (1U << 0)

#define UVC_JPEG_DATA_FLAG_SOF                 (1U << 2)
#define UVC_JPEG_DATA_FLAG_MID                 (1U << 1)
#define UVC_JPEG_DATA_FLAG_EOF                 (1U << 0)

typedef struct uvc_stream_header_struct
{
    uint8_t bHeaderLength;
    uint8_t bmHeaderInfo;
    uint8_t dwPresentationTime[4];
    uint8_t scrSourceClock_SourceTimeClock[4];
    uint8_t scrSourceClock_SOFCounter[2];
} uvc_stream_header_t;

typedef struct jpeg_stream_struct
{
    void    * p_pointer;
    uint32_t  size;
    uint32_t  flag;
} jpeg_stream_t;

enum video_format
{
    VideoFormat_Uncompressed,
    VideoFormat_MJPEG,
    VideoFormat_Unknown,
    VideoFormat_Failed,
};

/* Define messages we send to client. */
enum board_message
{
    BoardMessage_GetInputTerminalFailed,
    BoardMessage_CameraInsertedSuccessfully,
    BoardMessage_CameraUnplugged,
    BoardMessage_ReceivedConfig,
    BoardMessage_ConfigurationNotSupported,
    BoardMessage_CameraError,
    BoardMessage_UxReadFailed,
};

/* Define messages we receive from client. */
enum client_message
{
    ClientMessage_ChangeConfig,
    ClientMessage_StopStreaming,
};

/* Define different parameter formats we ask the client to abide by. */
enum parameter_format
{
    ParameterFormat_Null,
    ParameterFormat_Indices,
    ParameterFormat_Values,
};

#define MAX_STREAMER_DATA_PACKET_SIZE      (200*1024)//(150*1024)//(40*1024)
#define MAX_STREAMER_DATA_PACKET_NO      (0x2)
#define MAX_NUM_BUFFERS  2
typedef struct streamer_data_struct
{
    /* data */
    UCHAR http_jpg_data_left[MAX_STREAMER_DATA_PACKET_SIZE];
    UCHAR http_jpg_data_right[MAX_STREAMER_DATA_PACKET_SIZE];
    UCHAR index;
    ULONG len[MAX_NUM_BUFFERS];

}streamer_data_t;

extern NX_TCP_SOCKET           g_tcp_socket;
extern NX_UDP_SOCKET           g_udp_socket;

//extern NX_PACKET   *video_data_nx_packet_ptrs[JPEG_BUFFER_NUMBER];

extern NX_PACKET   *gp_packet_ptr;


extern ULONG                   g_client_ip_address;

/* Function prototypes */
void ra_usbx_host_uvc_init(void);
void ra_usbx_host_uvc_un_init(void);
void uvc_err_status(void);

#endif /* RA_USBX_HOST_UVC_COMMON_H_ */
