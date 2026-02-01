#include <stdio.h>
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

#include "hal_data.h"
#include "task.h"
#include "freertos.h"
#include "common_utils.h"
#include "lwprintf.h"


volatile uint32_t REMOTE_EPT_ADDR = 40U;
#define LOCAL_EPT_ADDR                (30U)

typedef struct the_message
{
    uint32_t DATA;
} THE_MESSAGE, *THE_MESSAGE_PTR;

static THE_MESSAGE msg = {0};

static struct rpmsg_lite_instance *my_rpmsg;

#define RPMSG_LITE_SHMEM_BASE 0x220E2000

#define RPMSG_LITE_NS_ANNOUNCE_STRING "rpmsg-m33"

static void app_nameservice_isr_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
    uint32_t *data = (uint32_t *)user_data;

    *data = new_ept;
}

void rp_ping_pong(void *param)
{
    volatile uint32_t remote_addr;
    volatile rpmsg_ns_handle ns_handle;

    struct rpmsg_lite_endpoint *my_ept;
    rpmsg_queue_handle my_queue;
    uint32_t len;
    /* Print the initial banner */
    lwprintf_printf("\r\nRPMSG Ping-Pong FreeRTOS RTOS API Demo...\r\n");

    my_rpmsg = rpmsg_lite_remote_init((void *)RPMSG_LITE_SHMEM_BASE, RL_PLATFORM_RA8T2_M33_LINK_ID, RL_NO_FLAGS);
    lwprintf_printf("remote init!\r\n");
    lwprintf_printf("Wait for link up!\r\n");
    rpmsg_lite_wait_for_link_up(my_rpmsg, portMAX_DELAY);
    lwprintf_printf("Link is up!\r\n");
    my_queue  = rpmsg_queue_create(my_rpmsg);
    my_ept    = rpmsg_lite_create_ept(my_rpmsg, LOCAL_EPT_ADDR, rpmsg_queue_rx_cb, my_queue);
    ns_handle = rpmsg_ns_bind(my_rpmsg, app_nameservice_isr_cb, ((void *)0));

    vTaskDelay(100);

    while (msg.DATA <= 100U)
    {
    	lwprintf_printf("Waiting for ping...\r\n");
        (void)rpmsg_queue_recv(my_rpmsg, my_queue, (uint32_t *)&REMOTE_EPT_ADDR, (char *)&msg, sizeof(THE_MESSAGE),&len, RL_BLOCK);//((void *)0)
        lwprintf_printf("Secondary core received a msg\r\n");
        lwprintf_printf("Message: Size=%x, DATA = %i\r\n", len, msg.DATA);
        msg.DATA++;
        lwprintf_printf("Sending pong...\r\n");
        vTaskDelay(100);
        (void)rpmsg_lite_send(my_rpmsg, my_ept, REMOTE_EPT_ADDR, (char *)&msg, sizeof(THE_MESSAGE), RL_BLOCK);
    }

    lwprintf_printf("Ping pong done, deinitializing...\r\n");

    (void)rpmsg_lite_destroy_ept(my_rpmsg, my_ept);
    my_ept = ((void *)0);
    (void)rpmsg_queue_destroy(my_rpmsg, my_queue);
    my_queue = ((void *)0);
    (void)rpmsg_ns_unbind(my_rpmsg, ns_handle);
    (void)rpmsg_lite_deinit(my_rpmsg);
    my_rpmsg = ((void *)0);
    msg.DATA = 0U;

    lwprintf_printf("\n\nAll test procedures have been completed.\n");
}
