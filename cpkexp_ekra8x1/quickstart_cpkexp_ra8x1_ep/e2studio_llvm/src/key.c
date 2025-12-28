#include "key.h"

static uint8_t s_key_release[KEY_02 + 1] = {1};

uint8_t key_scan(KeyEnum key)
{
    uint8_t keyval = KEY_OFF;
    bsp_io_level_t level;
    bsp_io_port_pin_t pin;

    if (key == KEY_01) {
        pin = BSP_IO_PORT_00_PIN_08;
    }
    else if (key == KEY_02) {
        pin = BSP_IO_PORT_00_PIN_06;
    }
    else {
        return KEY_OFF;
    }

    R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level);
    if ((s_key_release[key] == 1) && (level == BSP_IO_LEVEL_LOW)) {
        vTaskDelay(10);
        s_key_release[key] = 0;

        R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level);
        if (level == BSP_IO_LEVEL_LOW) {
            keyval = KEY_ON;
        }
    }
    else if (level == BSP_IO_LEVEL_HIGH) {
        s_key_release[key] = 1;
    }

    return keyval;
}


