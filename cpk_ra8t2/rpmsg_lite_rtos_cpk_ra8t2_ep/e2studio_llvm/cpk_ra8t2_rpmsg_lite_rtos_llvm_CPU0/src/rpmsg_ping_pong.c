#include <platform/ra8t2_m85/rpmsg_platform.h>
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

#include "hal_data.h"

#include "freertos.h"
#include "task.h"
#include "common_utils.h"
#include "lwprintf.h"



volatile uint32_t REMOTE_EPT_ADDR = 30U;
#define LOCAL_EPT_ADDR                (40U)

typedef struct the_message
{
    uint32_t DATA;
} THE_MESSAGE, *THE_MESSAGE_PTR;

static THE_MESSAGE msg = {0};

static struct rpmsg_lite_instance *my_rpmsg;

#define RPMSG_LITE_SHMEM_BASE 0x220E2000

#define SH_MEM_TOTAL_SIZE (64 * 1024)
//rt_mailbox_t my_rpmsg_mb;

static void app_nameservice_isr_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
    uint32_t *data = (uint32_t *)user_data;

    *data = new_ept;
}

void rp_ping_pong(void *param)
{
    volatile uint32_t remote_addr = 0U;
    struct rpmsg_lite_endpoint *my_ept;
    rpmsg_queue_handle my_queue;
    struct rpmsg_lite_instance *my_rpmsg;
    uint32_t len;
    lwprintf_printf("\r\nRPMSG Ping-Pong FreeRTOS RTOS API Demo...\r\n");
    lwprintf_printf("RPMSG Share Base Addr is 0x%x\r\n", RPMSG_LITE_SHMEM_BASE);

    lwprintf_printf("Run CPU1 first and while CPU1 is waitting for link up,then run CPU0 again!\r\n");
    my_rpmsg = rpmsg_lite_master_init((void *)RPMSG_LITE_SHMEM_BASE, SH_MEM_TOTAL_SIZE, RL_PLATFORM_RA8T2_M85_LINK_ID, RL_NO_FLAGS);
    lwprintf_printf("master init!\r\n");

    my_queue  = rpmsg_queue_create(my_rpmsg);
    my_ept    = rpmsg_lite_create_ept(my_rpmsg, LOCAL_EPT_ADDR, rpmsg_queue_rx_cb, my_queue);
    vTaskDelay(100);

    lwprintf_printf("\r\nSend the first message to the remoteproc\r\n");
    msg.DATA = 0;
    (void)rpmsg_lite_send(my_rpmsg, my_ept, REMOTE_EPT_ADDR, (char *)&msg, sizeof(THE_MESSAGE), RL_DONT_BLOCK);

    while (msg.DATA <= 100U)
    {
        (void)rpmsg_queue_recv(my_rpmsg, my_queue, (uint32_t *)&REMOTE_EPT_ADDR, (char *)&msg, sizeof(THE_MESSAGE), &len,RL_BLOCK);
        lwprintf_printf("Primary core received a msg\r\n");
        lwprintf_printf("Message: Size=%x, DATA = %i\r\n", len, msg.DATA);
        msg.DATA++;
        vTaskDelay(100);
        (void)rpmsg_lite_send(my_rpmsg, my_ept, REMOTE_EPT_ADDR, (char *)&msg, sizeof(THE_MESSAGE), RL_BLOCK);
    }

//    (void)rpmsg_lite_destroy_ept(my_rpmsg, my_ept);
//    my_ept = ((void *)0);
//    (void)rpmsg_queue_destroy(my_rpmsg, my_queue);
//    my_queue = ((void *)0);
//    (void)rpmsg_lite_deinit(my_rpmsg);

    lwprintf_printf("Ping pong done, deinitializing...\r\n");

    (void)rpmsg_lite_destroy_ept(my_rpmsg, my_ept);
    my_ept = ((void *)0);
    (void)rpmsg_queue_destroy(my_rpmsg, my_queue);
    my_queue = ((void *)0);
    (void)rpmsg_lite_deinit(my_rpmsg);

    lwprintf_printf("\n\nAll test procedures have been completed.\n");

}
