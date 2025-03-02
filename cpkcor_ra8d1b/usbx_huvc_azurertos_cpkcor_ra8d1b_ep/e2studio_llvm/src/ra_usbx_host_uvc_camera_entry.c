#include "ra_usbx_host_uvc_camera.h"
#include "ux_api.h"
#include "ux_host_class_video.h"
#include "ra_usbx_host_uvc_common.h"
#include "common_utils.h"



#define VIDEO_DATA_NX_PACKET_PTR_OFFSET (VIDEO_PACKET_HEADER_LENGTH - UVC_VIDEO_PAYLOAD_HEADER_SIZE)

#define BANDWIDTH_SEL    (0)

UINT ra_ux_host_uvc_usr_event_notification(ULONG event, UX_HOST_CLASS * host_class, VOID * instance);
VOID ra_ux_host_uvc_transfer_completion_callback(UX_TRANSFER *transfer_request);


static void set_video_transfer_callback_get_usb_camera_config(void);
static void set_video_parameters_start_video(void);

void ra_usbx_uvc_class_specific_req(void);



ULONG                   desired_frame_interval;
ULONG                   desired_format;
ULONG                   desired_resolution_x;
ULONG                   desired_resolution_y;
volatile ULONG                   completed_transfer_index;

ULONG   event_fvlaue;
ULONG   max_buffer_size;

/* Mem-pool size of 18k is required for USBX host-class pre-built libraries
 * and it is valid only if it with default USBX configurations. */
static uint32_t g_ux_uvc_pool_memory[MEMPOOL_SIZE / BYTE_SIZE];

UX_HOST_CLASS_VIDEO  *volatile gp_ra_ux_video_host_class;

UINT g_actual_tra_req_len[BUFFER_LEN];
UINT g_callback_index;

ULONG g_frame_intervals[16];

uvc_stream_header_t    *p_uvc_stream_header;
jpeg_stream_t           jpeg_stream;

_Bool g_detach_attach;

extern TX_THREAD ra_usbx_host_uvc_uninit_init;

#define PIC_WIDTH               (320)//(176)
#define PIC_HEIGHT              (240)//(144)
#define PIC_FRAME_INTERVAL      (1000000)//(333333)//(666666)//(333333)


#define PIC_FORMAT              UX_HOST_CLASS_VIDEO_VS_FORMAT_UNCOMPRESSED
//#define PIC_FORMAT            UX_HOST_CLASS_VIDEO_VS_FORMAT_MJPEG

ULONG buffer_index;
UCHAR *buffer_ptr[MAX_NUM_BUFFERS];
UCHAR data_left[4096];
UCHAR data_right[4096];
UINT packetno=0;
UCHAR pic_data[PIC_WIDTH*PIC_HEIGHT*2]={0};
ULONG cnt=0;



#if (CLASS_SPECIFIC_REQUEST == 1)
/*UVC Class Specific request*/
static UX_HOST_CLASS_VIDEO_PARAMETER_CHANNEL       g_channel_parameter;
static UX_ENDPOINT                                *gp_control_endpoint;
static UX_TRANSFER                                *gp_transfer_request;
static UINT                                        g_streaming_interface;
static UCHAR                                      *gp_control_buffer;
static UX_CONFIGURATION                           *gp_configuration;
static UX_INTERFACE                               *gp_interface_ptr;

#endif

///* USBX Host event notification callback function */

UINT ra_ux_host_uvc_usr_event_notification(ULONG event, UX_HOST_CLASS * host_class, VOID * instance)
{
    UINT status = UX_HOST_CLASS_INSTANCE_UNKNOWN;
    FSP_PARAMETER_NOT_USED(host_class);

    /* Callback user function for the USBX Host Class CDC-ACM. */
    if (UX_DEVICE_INSERTION == event) /* Check if there is a device insertion. */
    {
        if (UX_SUCCESS == _ux_utility_memory_compare(_ux_system_host_class_video_name, host_class,
                                                     _ux_utility_string_length_get(_ux_system_host_class_hub_name) ))
        {


            gp_ra_ux_video_host_class = (UX_HOST_CLASS_VIDEO *)instance;

            APP_PRINT("\r\n USB Camera Device is Connect \r\n");
            tx_event_flags_set(&g_usb_camera_device_connected_event_flags0, USB_CAMERA_DEVICE_CONNECT_FLAG, TX_OR);
            g_detach_attach = true;

        }
    }
    else if (UX_DEVICE_REMOVAL == event) /* Check if there is a device removal. */
    {

        if (UX_SUCCESS == _ux_utility_memory_compare(_ux_system_host_class_video_name, host_class,
                                                     _ux_utility_string_length_get(_ux_system_host_class_hub_name)))
        {
            gp_ra_ux_video_host_class = NULL;
            APP_PRINT("\r\n USB Camera Device is Removed \r\n");
            tx_event_flags_set(&g_usb_camera_device_connected_event_flags0, USB_CAMERA_DEVICE_REMOVED_FLAG, TX_AND);
            g_detach_attach = false;
        }
    }
    else
    {
        ((void)0);
    }


    return status;
}

UX_HOST_CLASS_VIDEO_PARAMETER_CHANNEL channel;

///* RA USBX Host UVC entry function */
void ra_usbx_host_uvc_camera_entry(void)
{
    /* TODO: add your own code here */
    UINT    status;

    UCHAR    is_eof;
    ULONG    cur_frame = 0;
    ULONG    frame_bytes_sent = 0;

    g_detach_attach = false;


    /* ux_system_initialization */
    status = ux_system_initialize(g_ux_uvc_pool_memory, MEMPOOL_SIZE, UX_NULL, UX_ZERO);
    if (UX_SUCCESS != status)
    {
        __BKPT(0);
    }

    /* initialize the ra usbx host uvc */
    ra_usbx_host_uvc_init();

    APP_PRINT("\r\n RA USBX Host UVC Initialization Completed \r\n");

    if(g_basic0_cfg.usb_speed == 1)
    {
        APP_PRINT("\r\n RA USBX Host UVC Operating On Full Speed\r\n");
    }else if(g_basic0_cfg.usb_speed == 2)
    {
        APP_PRINT("\r\n RA USBX Host UVC Operating On High Speed\r\n");
    }

    /* Wait for USB camera device connection status. */
    tx_event_flags_get (&g_usb_camera_device_connected_event_flags0, USB_CAMERA_DEVICE_CONNECT_FLAG, TX_OR, &event_fvlaue, TX_WAIT_FOREVER);
    tx_thread_sleep(100);


    g_detach_attach = false;



    APP_PRINT("\r\n%s %d\r\n",__FUNCTION__,__LINE__);
    /* register the ra ux host uvc transfer completion callback and Get usb camera configurations */
    set_video_transfer_callback_get_usb_camera_config();

    /*Set video parameter and start video streaming */
    set_video_parameters_start_video();
    APP_PRINT("\r\n%s %d\r\n",__FUNCTION__,__LINE__);

    while (1)
    {
        tx_event_flags_get (&g_usb_camera_device_connected_event_flags0, USB_CAMERA_DEVICE_CONNECT_FLAG, TX_OR, &event_fvlaue, TX_WAIT_FOREVER);
        APP_PRINT("\r\n%s %d\r\n",__FUNCTION__,__LINE__);
        if (gp_ra_ux_video_host_class != NULL)
        {
            while (1)
            {
                /* Was there any data received in this transfer? */
                if (0 != g_actual_tra_req_len[completed_transfer_index])
                {

                    /* Save the video stream header. */
                    //p_uvc_stream_header = (uvc_stream_header_t *)(video_data_nx_packet_ptrs[completed_transfer_index] -> nx_packet_prepend_ptr + VIDEO_DATA_NX_PACKET_PTR_OFFSET);
                    p_uvc_stream_header = (uvc_stream_header_t *)(buffer_ptr[completed_transfer_index]);
                    if (p_uvc_stream_header->bmHeaderInfo & UVC_STREAM_HEADER_HEADERINFO_ERR)
                    {
                        /* It may have only been this packet that had an error, so continue. */

                        APP_PRINT("\r\n error in packet header\n");
                        APP_PRINT("It may have only been this packet that had an error, so continue.\n");
                    }

                    /* Get JPEG data address and the size. */
                    //jpeg_stream.p_pointer = video_data_nx_packet_ptrs[completed_transfer_index] -> nx_packet_prepend_ptr + VIDEO_PACKET_HEADER_LENGTH;
                    jpeg_stream.p_pointer = buffer_ptr[completed_transfer_index] + UVC_VIDEO_PAYLOAD_HEADER_SIZE;
                    jpeg_stream.size      = g_actual_tra_req_len[completed_transfer_index] - UVC_VIDEO_PAYLOAD_HEADER_SIZE;

                    if (0 != jpeg_stream.size)
                    {
                        //APP_PRINT("\r\njpeg_stream.size is %d %s %d\r\n",jpeg_stream.size,__FUNCTION__,__LINE__);
                        /* SOF? */
                        if (0xD8FFU == *(USHORT *) jpeg_stream.p_pointer)
                        {
                            jpeg_stream.flag = UVC_JPEG_DATA_FLAG_SOF;
                        }
                        /* Check the end of UVC packet for a JPEG frame. */
                        else if (p_uvc_stream_header->bmHeaderInfo & UVC_STREAM_HEADER_HEADERINFO_EOF)
                        {
                            jpeg_stream.flag = UVC_JPEG_DATA_FLAG_EOF;
                        }
                        else
                        {
                            jpeg_stream.flag = UVC_JPEG_DATA_FLAG_MID;
                        }
                        memcpy(&pic_data[cnt],jpeg_stream.p_pointer,jpeg_stream.size);

                        cnt+=jpeg_stream.size;

                        is_eof = !!(jpeg_stream.flag == UVC_JPEG_DATA_FLAG_EOF);

                        /* Increase the number of bytes sent this frame. */
                        frame_bytes_sent += jpeg_stream.size;
                        //packetno ++;

                        if (is_eof)
                        {
                            cur_frame++;
                            APP_PRINT("per frame len: 0x%x total frames: 0x%x\n", frame_bytes_sent,cur_frame);
                            cnt = 0;
                            /* Reset variables for next frame. */
                            frame_bytes_sent = 0;
                        }
                    }
                }

                /* Suspend here until a UVC invokes Transfer Request Done callback . */
                tx_semaphore_get(&g_uvc_transfer_complete, TX_WAIT_FOREVER);

                status = ux_host_class_video_transfer_buffer_add(gp_ra_ux_video_host_class, buffer_ptr[completed_transfer_index]);
                if (status != UX_SUCCESS)
                {
                    tx_thread_sleep(200);
                    APP_PRINT ("failed to add buffer using ux_host_class_video_transfer_buffer_add API:\n");
                 }

                completed_transfer_index++;
                if (completed_transfer_index == MAX_NUM_BUFFERS)
                    completed_transfer_index = 0;
            }
        }
        else
        {
            tx_thread_sleep (100);
        }
    }

}



void ra_usbx_host_uvc_init(void)
{
    UINT status = UX_SUCCESS;

    // host stack initialization
    status = ux_host_stack_initialize (ra_ux_host_uvc_usr_event_notification);
    if (status != UX_SUCCESS)
    {
        uvc_err_status();
    }
    /*Start the USB module by calling open function */
    status = g_usb_on_usb.open(&g_basic0_ctrl, &g_basic0_cfg);
    if (status != UX_SUCCESS)
    {
        uvc_err_status();
    }
}

void ra_usbx_host_uvc_un_init(void)
{
    UINT status = UX_SUCCESS;

    /*Invoke host un-initializes function*/
    status = ux_host_stack_uninitialize();
    if (status != UX_SUCCESS)
    {
        uvc_err_status();
    }
    /*Stop the USB module by calling close function */
    status = g_usb_on_usb.close(&g_basic0_ctrl);
    if (status != UX_SUCCESS)
    {
        uvc_err_status();
    }

}
/* LED error indication */
void uvc_err_status(void)
{
    /* Error Indication */
    APP_PRINT("uvc error!\r\n");
    tx_thread_sleep(5);
}

VOID ra_ux_host_uvc_transfer_completion_callback(UX_TRANSFER *transfer_request)
{

    g_actual_tra_req_len[g_callback_index] = transfer_request->ux_transfer_request_actual_length;
    g_callback_index++;
    if (g_callback_index == ARR_EL_COUNT(g_actual_tra_req_len))
    {
        g_callback_index = 0;
    }
    tx_semaphore_put(&g_uvc_transfer_complete);

}




/* register the ra ux host uvc transfer completion callback and Get usb camera configurations*/
static void set_video_transfer_callback_get_usb_camera_config(void)
{

    /* Wait for USB camera device connection status. */
    tx_event_flags_get (&g_usb_camera_device_connected_event_flags0, USB_CAMERA_DEVICE_CONNECT_FLAG, TX_OR, &event_fvlaue, TX_WAIT_FOREVER);
    tx_thread_sleep(100);

    /* register the ra ux host uvc transfer completion callback*/
    ux_host_class_video_transfer_callback_set(gp_ra_ux_video_host_class, &ra_ux_host_uvc_transfer_completion_callback);
    tx_thread_sleep(100);



    APP_PRINT("\r\n Get usb camera configurations and Send to Video App \r\n");

    tx_thread_sleep(100);
    ra_usbx_uvc_class_specific_req();
}


static void set_video_parameters_start_video(void)
{

    UINT status;
    UINT   i;


    while(true)
    {
//        desired_format = UX_HOST_CLASS_VIDEO_VS_FORMAT_UNCOMPRESSED;//UX_HOST_CLASS_VIDEO_VS_FORMAT_MJPEG;

//        desired_resolution_x = 320;
//        desired_resolution_y = 240;
//        desired_frame_interval = 333333;
        desired_format = PIC_FORMAT;
        desired_resolution_x = PIC_WIDTH;
        desired_resolution_y = PIC_HEIGHT;
        desired_frame_interval = PIC_FRAME_INTERVAL;
        APP_PRINT("\r\n video parameters:\r\n Desired_format = %d.\r\n Resolution X = %d.\r\n Resolution Y = %d. \r\n Frame interval = %d. \r\n",
                desired_format, desired_resolution_x, desired_resolution_y, desired_frame_interval);
        /* Set video parameters. */
        status = ux_host_class_video_frame_parameters_set(gp_ra_ux_video_host_class, desired_format, desired_resolution_x, desired_resolution_y, desired_frame_interval);
        if(status != UX_SUCCESS)
        {
            APP_PRINT ("failed to set video parameters: 0x%x\n", status);
        }

        /* Find out the maximum memory buffer size for the video configuration
         * set above. */
        max_buffer_size = ux_host_class_video_max_payload_get(gp_ra_ux_video_host_class);
        APP_PRINT ("\r\n Max Pay load Get: %d\r\n", max_buffer_size);


#if(BANDWIDTH_SEL)
        {
            /* Start the channel. */

            /* Setting these to zero is a hack since we're mixing old and new APIs (new API does this and is required for reads). */
            gp_ra_ux_video_host_class -> ux_host_class_video_transfer_request_start_index = 0;
            gp_ra_ux_video_host_class -> ux_host_class_video_transfer_request_end_index = 0;

            channel.ux_host_class_video_parameter_format_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_format;
            channel.ux_host_class_video_parameter_frame_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame;
            channel.ux_host_class_video_parameter_frame_interval_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame_interval;
            channel.ux_host_class_video_parameter_channel_bandwidth_selection = 1024;

            status = _ux_host_class_video_ioctl(gp_ra_ux_video_host_class, UX_HOST_CLASS_VIDEO_IOCTL_CHANNEL_START, &channel);
            if(status)
            {

                APP_PRINT ("failed to start video: 0x%x\n", status);
                status = _ux_host_class_video_ioctl(gp_ra_ux_video_host_class, UX_HOST_CLASS_VIDEO_IOCTL_CHANNEL_STOP, &channel);

                tcp_send_ulong_sync(&g_tcp_socket, BoardMessage_ConfigurationNotSupported);

                ra_select_desired_configuration();
                tx_thread_sleep(10);
                continue;
            }
            else
            {
                break;
            }

        }

#else
        {
            status = ux_host_class_video_start(gp_ra_ux_video_host_class);
            if (status != UX_SUCCESS)
            {

                APP_PRINT("failed to start video: 0x%x\n", status);

                status = ux_host_class_video_stop(gp_ra_ux_video_host_class);
                if (status)
                {
                    APP_PRINT ("failed to stop channel: 0x%x\n", status);
                }
                /* Setting these to zero is a hack since we're mixing old and new APIs (new API does this and is required for reads). */
                gp_ra_ux_video_host_class -> ux_host_class_video_transfer_request_start_index = 0;
                gp_ra_ux_video_host_class -> ux_host_class_video_transfer_request_end_index = 0;

                channel.ux_host_class_video_parameter_format_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_format;
                channel.ux_host_class_video_parameter_frame_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame;
                channel.ux_host_class_video_parameter_frame_interval_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame_interval;
                channel.ux_host_class_video_parameter_channel_bandwidth_selection = 1024;

                status = _ux_host_class_video_ioctl(gp_ra_ux_video_host_class, UX_HOST_CLASS_VIDEO_IOCTL_CHANNEL_START, &channel);
                if(status)
                {

                    APP_PRINT ("failed to start video: 0x%x\n", status);
                    status = _ux_host_class_video_ioctl(gp_ra_ux_video_host_class, UX_HOST_CLASS_VIDEO_IOCTL_CHANNEL_STOP, &channel);
                    tx_thread_sleep(10);
                    continue;
                }

            }
            else
            {
                break;
            }
        }
#endif
    }

    /* Clear semaphore to zero; this ensures that while reading transfer requests, we don't trick ourselves into thinking we read one. */
    for(i = 0; i < (UINT)ARR_EL_COUNT(buffer_ptr); i++)
        tx_semaphore_get(&g_uvc_transfer_complete, 100);

    completed_transfer_index = 0;
    g_callback_index = 0;
    APP_PRINT ("Video Streaming Started...\r\n ");
    /* Send all of our transfer requests to USBX. */
    for (i = 0; i < ARR_EL_COUNT(buffer_ptr); i++)
    {

        if (i > 0) // Just confirm first data buffer added and got completion callback before adding second data buffer
            tx_semaphore_get(&g_uvc_transfer_complete, TX_WAIT_FOREVER);
        if(i==0)
            buffer_ptr[0]=&data_left[0];
        else
            buffer_ptr[1]=&data_right[0];
        //status = _ux_host_class_video_transfer_buffer_add(gp_ra_ux_video_host_class, video_data_nx_packet_ptrs[i] -> nx_packet_prepend_ptr + VIDEO_DATA_NX_PACKET_PTR_OFFSET);
        status = _ux_host_class_video_transfer_buffer_add(gp_ra_ux_video_host_class, buffer_ptr[buffer_index]);
        if (status != UX_SUCCESS)
        {
            APP_PRINT ("failed to add buffer using ux_host_class_video_transfer_buffer_add API:\n");
        }
    }


}



void ra_usbx_uvc_class_specific_req(void)
{

#if (CLASS_SPECIFIC_REQUEST == 1)
    UINT status;
    UINT uvc_specific_req[6] = {(UINT)UX_HOST_CLASS_VIDEO_GET_INFO,(UINT)UX_HOST_CLASS_VIDEO_GET_CUR,(UINT)UX_HOST_CLASS_VIDEO_GET_MIN,(UINT)UX_HOST_CLASS_VIDEO_GET_MAX,(UINT)UX_HOST_CLASS_VIDEO_GET_RES,(UINT)0};

    /* Get current parameters from the video instance.  */
    g_channel_parameter.ux_host_class_video_parameter_format_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_format;
    g_channel_parameter.ux_host_class_video_parameter_frame_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame;
    g_channel_parameter.ux_host_class_video_parameter_frame_interval_requested = gp_ra_ux_video_host_class -> ux_host_class_video_current_frame_interval;
    g_channel_parameter.ux_host_class_video_parameter_channel_bandwidth_selection = 0;

    /* We found the alternate setting for the sampling values demanded, now we need
        to search its container.  */
    gp_configuration =        gp_ra_ux_video_host_class -> ux_host_class_video_streaming_interface -> ux_interface_configuration;
    gp_interface_ptr =        gp_configuration -> ux_configuration_first_interface;

    /* We need to get the default control endpoint transfer request pointer.  */
    gp_control_endpoint =  &gp_ra_ux_video_host_class -> ux_host_class_video_device -> ux_device_control_endpoint;
    gp_transfer_request =  &gp_control_endpoint -> ux_endpoint_transfer_request;

    /* Get the interface number of the video streaming interface.  */
    g_streaming_interface =  gp_ra_ux_video_host_class -> ux_host_class_video_streaming_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Scan all interfaces.  */
    while (gp_interface_ptr != UX_NULL)
    {

        /* We search for both the right interface and alternate setting.  */
        if ((gp_interface_ptr -> ux_interface_descriptor.bInterfaceNumber == g_streaming_interface) &&
                (gp_interface_ptr -> ux_interface_descriptor.bAlternateSetting == 0))
        {

            /* We have found the right interface/alternate setting combination
               The stack will select it for us.  */
            status =  _ux_host_stack_interface_setting_select(gp_interface_ptr);

            /* If the alternate setting for the streaming interface could be selected, we memorize it.  */
            if (status == UX_SUCCESS)
            {

                /* Memorize the interface.  */
                gp_ra_ux_video_host_class -> ux_host_class_video_streaming_interface =  gp_interface_ptr;

                /* There is no endpoint for the alternate setting 0.  */
                gp_ra_ux_video_host_class -> ux_host_class_video_isochronous_endpoint = UX_NULL;

            }
        }

        /* Move to next interface.  */
        gp_interface_ptr =  gp_interface_ptr -> ux_interface_next_interface;
    }



    /* Need to allocate memory for the control_buffer.  */
    gp_control_buffer =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_VIDEO_PROBE_COMMIT_LENGTH);
    if (gp_control_buffer != UX_NULL)
    {

        *(gp_control_buffer + UX_HOST_CLASS_VIDEO_PROBE_COMMIT_FORMAT_INDEX) = (UCHAR)g_channel_parameter.ux_host_class_video_parameter_format_requested;
        *(gp_control_buffer + UX_HOST_CLASS_VIDEO_PROBE_COMMIT_FRAME_INDEX) = (UCHAR)g_channel_parameter.ux_host_class_video_parameter_frame_requested;

        _ux_utility_long_put(gp_control_buffer + UX_HOST_CLASS_VIDEO_PROBE_COMMIT_FRAME_INTERVAL,
                g_channel_parameter.ux_host_class_video_parameter_frame_interval_requested);

        APP_PRINT ("\r\n VideoStreaming Interface Control Selectors \r\n");
        for (UINT i = 0; (0U != uvc_specific_req[i]); i++)
        {
            /* Create a transfer request for the SET_CUR_buffer request.  */
            gp_transfer_request -> ux_transfer_request_data_pointer =      gp_control_buffer;
            if(i == 0)
            {
                gp_transfer_request -> ux_transfer_request_requested_length =  1;
                APP_PRINT("GET_INFO Request: \r\n");
            }
            else
            {
                if(i == 1)
                {
                    APP_PRINT("GET_CUR Request: \r\n");
                }else if (i == 2)
                {
                    APP_PRINT("GET_MIN Request: \r\n");
                }
                else if (i == 3)
                {
                    APP_PRINT("GET_MAX Request: \r\n");
                }
                else
                {
                    APP_PRINT("GET_RES Request:\r\n");
                }
                gp_transfer_request -> ux_transfer_request_requested_length = UX_HOST_CLASS_VIDEO_PROBE_COMMIT_LENGTH;
            }
            gp_transfer_request -> ux_transfer_request_function =          uvc_specific_req[i];
            gp_transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
            gp_transfer_request -> ux_transfer_request_value =             UX_HOST_CLASS_VIDEO_VS_PROBE_CONTROL << 8;
            gp_transfer_request -> ux_transfer_request_index =             g_streaming_interface;

            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(gp_transfer_request);
            if (status == UX_SUCCESS)
            {

                for (ULONG j = 0; j < gp_transfer_request -> ux_transfer_request_requested_length; j++)
                {
                    APP_PRINT ("0X%x ",*(gp_control_buffer+j));
                }
                APP_PRINT("\r\n");
                tx_thread_sleep(100);

            }
            else
            {
                APP_PRINT ("ux_host_stack_transfer_request failed \r\n");
            }

            tx_thread_sleep(100);

        }

        APP_PRINT ("\r\nExposure Time (Absolute) Control \r\n");
        for (UINT i = 0; (0U != uvc_specific_req[i]); i++)
        {
            /* Create a transfer request for the GET_INFO request.  */
            gp_transfer_request -> ux_transfer_request_data_pointer =      gp_control_buffer;
            if (i == 0)
            {
                gp_transfer_request -> ux_transfer_request_requested_length =  1;
                APP_PRINT("GET_INFO Request: \r\n");
            }
            else
            {
                if(i == 1)
                {
                    APP_PRINT("GET_CUR Request: \r\n");
                }else if (i == 2)
                {
                    APP_PRINT("GET_MIN Request: \r\n");
                }
                else if (i == 3)
                {
                    APP_PRINT("GET_MAX Request: \r\n");
                }
                else
                {
                    APP_PRINT("GET_RES Request: \r\n");
                }
                gp_transfer_request -> ux_transfer_request_requested_length =  4;
            }
            gp_transfer_request -> ux_transfer_request_function =          uvc_specific_req[i];
            gp_transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
            gp_transfer_request -> ux_transfer_request_value =             0x400;
            gp_transfer_request -> ux_transfer_request_index =             0x100;

            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(gp_transfer_request);
            if (status == UX_SUCCESS)
            {
                for (ULONG j = 0; j < gp_transfer_request -> ux_transfer_request_requested_length; j++)
                {
                    APP_PRINT ("0X%x ",*(gp_control_buffer+j));
                }
                APP_PRINT("\r\n");
                tx_thread_sleep(100);
            }
            else
            {
                APP_PRINT ("ux_host_stack_transfer_request failed \r\n");
            }

            tx_thread_sleep(100);
        }

        tx_thread_sleep(200);

    }
    else
    {
        APP_PRINT ("failed to allocate memory\r\n");

    }
#else
    (void)0;

#endif
}
