#include "common_utils.h"
#include "key.h"
#include "key_thread.h"
#include "lcd.h"

#include "graphics/graphics.h"

#define KEY_SCAN_INTERVAL           200

void key_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (1)
    {
        if (key_scan(KEY_01) == KEY_ON) {
            xEventGroupSetBits(g_event_group, KEY_EVENT_01_PRESS);
        }
        else if (key_scan(KEY_02) == KEY_ON) {
            xEventGroupSetBits(g_event_group, KEY_EVENT_02_PRESS);
        }

        vTaskDelay(KEY_SCAN_INTERVAL);
    }
}
