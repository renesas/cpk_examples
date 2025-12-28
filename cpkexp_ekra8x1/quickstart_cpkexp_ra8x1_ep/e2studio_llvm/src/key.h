#ifndef KEY_H_
#define KEY_H_

#include "common_data.h"

typedef enum {
    KEY_01 = 0,
    KEY_02 = 1
} KeyEnum;

#define KEY_EVENT_01_PRESS  (0x01)
#define KEY_EVENT_02_PRESS  (0x02)

#define KEY_ON  1
#define KEY_OFF 0

uint8_t key_scan(KeyEnum key);

#endif
