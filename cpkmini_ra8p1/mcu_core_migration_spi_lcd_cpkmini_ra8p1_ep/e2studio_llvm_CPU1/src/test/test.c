#include <stdio.h>

#include "test.h"
#include "utils/log.h"

uint32_t TestAll(void *lcd_device)
{
	(void)lcd_device;

#if TEST_EN_LCD
	extern uint32_t TestLCD(void *lcd_device);
	LOG_PutsEndl("================== TEST_EN_LCD Start ==================");
	TestLCD(lcd_device);
	LOG_PutsEndl("==================  TEST_EN_LCD End  ==================");
#endif

	return 0;
}
