#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "hal_data.h"

/* ======================================= Generic config =======================================
 * Input/Output Stream Implementation. Note:
 * - Only one option can be selected.
 * - If CONSOLE_CFG_USE_CUSTOM is selected, The read and write functions need to be implemented.
 * - LCD only support output and using arm2d library
 * - USB/RPMSG/ have not been realized */
#ifndef CONSOLE_CFG_USE_RTT
#define CONSOLE_CFG_USE_RTT			0
#endif

#ifndef CONSOLE_CFG_USE_UART
#define CONSOLE_CFG_USE_UART		0
#endif

#ifndef CONSOLE_CFG_USE_USB
#define CONSOLE_CFG_USE_USB			0
#endif

#ifndef CONSOLE_CFG_USE_RPMSG
#define CONSOLE_CFG_USE_RPMSG		0
#endif

#ifndef CONSOLE_CFG_USE_LCD
#define CONSOLE_CFG_USE_LCD			0
#endif

#ifndef CONSOLE_CFG_USE_CUSTOM
#define CONSOLE_CFG_USE_CUSTOM		0
#endif

/* Receive cache size, only use in UART/USB/RPMSG */
#define CONSOLE_CFG_RX_BUF_SIZE		128

/* ==================================== RTT Specific Config  ==================================== */
#if CONSOLE_CFG_USE_RTT

/* SEGGER RTT terminal index */
#ifndef CONSOLE_CFG_RTT_INDEX
#define CONSOLE_CFG_RTT_INDEX		0
#endif

/* If this macro is 1, then CONSOLE_Init() will not call SEGGER_RTT_Init() */
#ifndef CONSOLE_CFG_RTT_DUAL_CORE
#define CONSOLE_CFG_RTT_DUAL_CORE	0
#endif

#endif

/* ==================================== UART Specific Config ==================================== */
#if CONSOLE_CFG_USE_UART
/* UART instance and callback, it should be the same as FSP Configuration */
#define CONSOLE_CFG_UART_INSTANCE	g_uart9
#define CONSOLE_CFG_UART_CALLBACK	UART9_Callback
#endif

/* ==================================== USB Specific Config  ==================================== */
#if CONSOLE_CFG_USE_USB
/* USB instance and callback, it should be the same as FSP Configuration */
#define CONSOLE_CFG_USB_INSTANCE	g_usb_basic
#define CONSOLE_CFG_USB_CALLBACK	USB_Callback
#endif

/* =================================== RPMSG Specific Config  =================================== */
#if CONSOLE_CFG_USE_RPMSG
/* IPC instance name, it should be the same as FSP Configuration */
#define CONSOLE_CFG_IPC0_INSTANCE	g_ipc0
#define CONSOLE_CFG_IPC1_INSTANCE	g_ipc1
#define CONSOLE_CFG_RPMSG_W_SIZE	128
#define CONSOLE_CFG_RPMSG_W_NUM		32
#define CONSOLE_CFG_RPMSG_R_SIZE	CONSOLE_CFG_RX_BUF_SIZE
#define CONSOLE_CFG_RPMSG_R_NUM		4
#endif

/* ==================================== Function Declaration ==================================== */
#if CONSOLE_CFG_USE_RTT || CONSOLE_CFG_USE_UART || CONSOLE_CFG_USE_LCD || CONSOLE_CFG_USE_RPMSG

void CONSOLE_Init(void);

#if CONSOLE_CFG_USE_RTT

#include "SEGGER_RTT/SEGGER_RTT.h"

#define CONSOLE_HasData				SEGGER_RTT_HasKey
#define CONSOLE_Read(buffer, size)	SEGGER_RTT_Read(CONSOLE_CFG_RTT_INDEX, buffer, size)

#else

int CONSOLE_HasData(void);
unsigned CONSOLE_Read(void* buffer, unsigned size);

#endif

#if CONSOLE_CFG_USE_RPMSG
/* Three extra function is need when using RPMSG, put them in IPC IRQ function */
void CONSOLE_IPCIrqHandle(void);
void CONSOLE_IPCMsgHandle(uint32_t msg);
void CONSOLE_Task(void);

typedef enum {
	RPMSG_LOCK_INPUT,
	RPMSG_LOCK_OUTPUT
} CONSOLE_RPMSGLockEnum;

/* When using an RTOS, implement these two functions */
extern void CONSOLE_LockGive(CONSOLE_RPMSGLockEnum elock);
extern void CONSOLE_LockTake(CONSOLE_RPMSGLockEnum elock);

#endif

#else

extern int CONSOLE_HasData(void);
extern void CONSOLE_Init(void);
extern void CONSOLE_PutChar(char ch);
extern unsigned CONSOLE_Read(void* buffer, unsigned size);

#endif /* #if CONSOLE_CFG_USE_RTT */

#endif
