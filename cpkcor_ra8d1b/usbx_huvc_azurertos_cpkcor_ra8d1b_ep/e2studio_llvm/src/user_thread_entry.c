#include "user_thread.h"
#include "common_utils.h"
#include "stdio.h"
#include "stdlib.h"
#include "ra_usbx_host_uvc_common.h"


extern TX_THREAD ra_usbx_host_uvc_camera;
/* User Thread entry function */
void user_thread_entry(void)
{
    /* TODO: add your own code here */
    UINT status  = 0;
    UINT tx_state;

    status =  tx_thread_info_get(&ra_usbx_host_uvc_camera, TX_NULL, &tx_state, TX_NULL, TX_NULL, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
    if (TX_SUCCESS != status)
    {
        /* error */
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

        APP_PRINT(" tx_thread_resume(&ra_usbx_host_uvc_camera) = %d\r\n", tx_state);
    }

    while (true)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
        APP_PRINT("thread1 echo!\r\n");
    }
}
