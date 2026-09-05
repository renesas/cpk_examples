#include "lcd.h"

uint32_t LCD_Init(LCD_Device *device)
{
    LCD_PortInit(device);
    
    return 0;
}
