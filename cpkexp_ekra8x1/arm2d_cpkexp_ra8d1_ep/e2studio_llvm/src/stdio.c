#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "SEGGER_RTT/SEGGER_RTT.h"

void _exit(int code)
{
	(void)code;
        __asm("BKPT #0\n");
}

int segger_write(char ptr, FILE *file)
{
	(void)file;
	if (ptr == '\n')
		SEGGER_RTT_PutChar(0, '\r');
	SEGGER_RTT_PutChar(0, ptr);

	return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(segger_write,
		NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
