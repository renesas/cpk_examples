#include <stdarg.h>
#include <stdio.h>

#include "console.h"
#include "hal_data.h"

/* If using __CONSOLE_DEBUG, you must implement output stream by yourself.
 * You can't use C standard output */
#ifndef __CONSOLE_DEBUG
#define __CONSOLE_DEBUG		0
#endif

#if __CONSOLE_DEBUG
#include "utils/log.h"
#endif

/*********************************************************************************************************************
 * Different IO implement may need different extern function, variables or some struct. Defined here
 *********************************************************************************************************************/
#if CONSOLE_CFG_USE_RPMSG

/* First, decide who is the printer */
#if CONSOLE_CFG_USE_RTT || CONSOLE_CFG_USE_UART || CONSOLE_CFG_USE_USB || CONSOLE_CFG_USE_LCD || CONSOLE_CFG_USE_CUSTOM
#define I_AM_PRINTER	1
#else
#define I_AM_PRINTER	0
#endif

#if _RA_CORE == CPU0
#define IPC_INSTANCE	CONSOLE_CFG_IPC1_INSTANCE
#else
#define IPC_INSTANCE	CONSOLE_CFG_IPC0_INSTANCE
#endif

#define IPC_MSG_ACK				0xFFEE0000
#define IPC_MSG_BUSY			0xFFEE0001
#define IPC_MSG_QUERY			0xFFEE0002
#define IPC_MSG_READY			0xFFEE0003
#define IPC_MSG_RECEIVE			0xFFEE0004
#define IPC_MSG_SEND			0xFFEE0005
#define IPC_MSG_GET				0xFFEE0006

#define IO_LOCK_TAKE(io)		while (io->lock) { R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS); } io->lock = 1;
#define IO_LOCK_GIVE(io)		io->lock = 0

enum PrinterStatus {
	PRINTER_CONNECT,
	PRINTER_RECEIVE_DATA,
	PRINTER_SEND_DATA,
	PRINTER_UNCONNECT
};

enum RequesterStatus {
	REQUESTER_CONNECT,
	REQUESTER_RECEIVE_DATA,
	REQUESTER_SEND_DATA,
	REQUESTER_UNCONNECT
};

enum StatusMachineConnect {
	CONNECT_SM_DONE,
	CONNECT_SM_WAIT_ACK,
	CONNECT_SM_WAIT_OCACHE,
#if I_AM_PRINTER
	CONNECT_SM_WAIT_IRQ
#endif
};

enum StatusMachineSend {
	SEND_SM_DONE,
	SEND_SM_GET_ACK,
	SEND_SM_QUERY,
	SEND_SM_SEND_ADDR,
	SEND_SM_WAIT_ACK
};

struct InputPackage {
	uint32_t lock : 1;
	uint32_t length : 31;
	char data[CONSOLE_CFG_RPMSG_R_SIZE];
};

struct OutputPackage {
	uint32_t lock : 1;
	uint32_t length : 31;
	char data[CONSOLE_CFG_RPMSG_W_SIZE];
};

struct Printer {
	uint32_t should_handle : 1;
	uint32_t reserved : 31;
	enum PrinterStatus status;
	enum StatusMachineConnect sm_connect;
	enum StatusMachineSend sm_send;
};

struct Requester {
	uint32_t should_handle : 1;
	uint32_t reserved : 31;
	enum RequesterStatus status;
	enum StatusMachineConnect sm_connect;
	enum StatusMachineSend sm_send;
};

#if I_AM_PRINTER

struct ReceiveIndexQueue {
	uint32_t addr[CONSOLE_CFG_RPMSG_W_NUM];
	uint16_t size;
};

static uint16_t s_itail;
static uint16_t s_ihead;
static struct ReceiveIndexQueue s_rec_queue;
static struct Printer s_printer;

#if _RA_CORE == CPU0
/* CPU0 or CPU1 wants to access to another core's tcm, it needs different address
 * CPU0 want to send its dtcm address to CPU1, it must be added with this offset so that CPU1 can access */
#define TCM_OFFSET	0x08020000
static struct InputPackage s_icache[CONSOLE_CFG_RPMSG_R_NUM] __attribute__((section(".dtcm_noinit")));
static struct OutputPackage s_ocache[CONSOLE_CFG_RPMSG_W_NUM] __attribute__((section(".dtcm_noinit")));
#else
/* CPU1 want to send its stcm address to CPU0, it must be added with this offset so that CPU0 can access */
#define TCM_OFFSET	0x0A010000
static struct InputPackage s_icache[CONSOLE_CFG_RPMSG_R_NUM] __attribute__((section(".stcm_noinit")));
static struct OutputPackage s_ocache[CONSOLE_CFG_RPMSG_W_NUM] __attribute__((section(".stcm_noinit")));
#endif

#else

#if _RA_CORE == CPU0
/* CPU0 want to send CPU1's stcm address to CPU1, it must be subtracted this offset so that CPU1 can access */
#define TCM_OFFSET	0x0A010000
#else
/* CPU1 want to send CPU0's dtcm address to CPU0, it must be subtracted this offset so that CPU0 can access */
#define TCM_OFFSET	0x08020000
#endif

struct ReceiveIndexQueue {
	uint32_t addr[CONSOLE_CFG_RPMSG_R_NUM];
	uint16_t size;
};

static uint16_t s_otail;
static uint16_t s_ohead;
static struct ReceiveIndexQueue s_rec_queue;
static struct Requester s_requester;
static struct OutputPackage *s_ocache = NULL;

#endif /* #if I_AM_PRINTER */

static void smConnectInIPCMsg(uint32_t msg);
static void smSendInIPCMsg(uint32_t msg);

#if I_AM_PRINTER
static void smConnectInIPCIrq(void);
#endif

#elif CONSOLE_CFG_USE_LCD

extern void CONSOLE_LCD_Putchar(char ch);

#endif

#ifndef I_AM_PRINTER
#define I_AM_PRINTER 0
#endif

/*********************************************************************************************************************
 * Different complier require different functions to be rewritten. Defined here
 *********************************************************************************************************************/
#if defined(__ARMCOMPILER_VERSION)

int fputc(int ch, FILE *f)
{
	(void)f;
	
#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_PutChar(CONSOLE_CFG_RTT_INDEX, ch);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    ctrl->p_reg->TDR_BY = (uint8_t)ch;
    while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
    CONSOLE_LCD_Putchar((char)ch);
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif
		
	return 0;
}

/* These case don't have input, so always return 0 */
#if CONSOLE_CFG_USE_LCD
int fgetc(FILE *f)
{
	(void)f;

	return 0;
}
#else
int fgetc(FILE *f)
{
	(void)f;
	
	char c;

	while (CONSOLE_HasData() == 0) {
	#ifdef __OPTIMIZE__
		__nop();
	#endif
	}
	CONSOLE_Read(&c, 1);

	return c;
}
#endif

#elif defined(__clang_version__)

static int __fputc(char ch, FILE *f)
{
	(void)ch;
    (void)f;

#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_PutChar(CONSOLE_CFG_RTT_INDEX, ch);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    ctrl->p_reg->TDR_BY = ch;
    while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG && (I_AM_PRINTER == 0)
    bool notice = false;
    struct OutputPackage *o = &s_ocache[s_otail];

    CONSOLE_LockTake(RPMSG_LOCK_OUTPUT);
    IO_LOCK_TAKE(o);
    __DMB();
    o->data[o->length] = ch;
    o->length++;
    if ((o->length == (CONSOLE_CFG_RPMSG_W_SIZE - 1)) || (ch == '\n')) {
    	o->data[o->length] = '\0';
    	notice = true;
    	s_otail++;
    	if (s_otail == CONSOLE_CFG_RPMSG_W_NUM) {
    		s_otail = 0;
    	}
    	if (s_otail == s_ohead) {
    		s_ohead++;
    	}
    	if (s_ohead == CONSOLE_CFG_RPMSG_W_NUM) {
    		s_ohead = 0;
    	}
    }
    IO_LOCK_GIVE(o);
    CONSOLE_LockGive(RPMSG_LOCK_OUTPUT);

    if (notice) {
    	while (s_requester.status != REQUESTER_CONNECT) {
    		R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
    	}
    	/* Other case: requester is handling a send process, don't terninate it */
    	if (s_requester.sm_send == SEND_SM_DONE) {
			s_requester.status = REQUESTER_SEND_DATA;
			s_requester.sm_send = SEND_SM_WAIT_ACK;
			s_requester.should_handle = 1;
			R_IPC_EventGenerate(IPC_INSTANCE.p_ctrl, IPC_GENERATE_EVENT_IRQ0);
    	}
    }
#elif CONSOLE_CFG_USE_LCD
    CONSOLE_LCD_Putchar(ch);
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif

    return 0;
}

/* These case don't have input, so always return 0 */
#if CONSOLE_CFG_USE_LCD
static int __fgetc(FILE *f)
{
	(void)f;

	return 0;
}
#else
static int __fgetc(FILE *f)
{
	char c;

	(void)f;

	while (CONSOLE_HasData() == 0) {
	/* If use uart and compiler(ATfE 21.1.1) optimize,
	 * the compiler will generate incorrect assembly code here.
	 * So insert an __nop here to modify the behavior of compiler.
	 * See Snipaste_2025-12-23_17-47-24.png */
	#ifdef __OPTIMIZE__
		__nop();
	#endif
	}
	CONSOLE_Read(&c, 1);

	return c;
}
#endif

static FILE __stdio = FDEV_SETUP_STREAM(__fputc, __fgetc, NULL, _FDEV_SETUP_RW);
FILE * const stdin = &__stdio;
FILE * const stdout = &__stdio;
FILE * const stderr = &__stdio;

#else

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

int _close(int fd);
int _fstat(int fd, struct stat *st);
int _isatty(int fd);
int _lseek(int fd, int ptr, int dir);
int _read(int fd, char *pBuffer, int size);
int _write(int fd, char *pBuffer, int size);

__attribute__((weak)) int _close(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		return 0;
	}

	errno = EBADF;

	return -1;
}

__attribute__((weak)) int _fstat(int fd, struct stat *st)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		st->st_mode = S_IFCHR;
	}
	errno = EBADF;

	return 0;
}

__attribute__((weak)) int _isatty(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		return 1;
	}

	errno = EBADF;

	return 0;
}

__attribute__((weak)) int _lseek(int fd, int ptr, int dir)
{
	(void)fd;
	(void)ptr;
	(void)dir;

	errno = EBADF;

	return -1;
}

/* These case don't have input, so always return 0 */
#if CONSOLE_CFG_USE_LCD
int _read(int fd, char *pBuffer, int size)
{
	(void)fd;
	(void)pBuffer;
	(void)size;

	return -1;
}
#else
int _read(int fd, char *pBuffer, int size)
{
	unsigned r;

	unsigned have_read = 0;
	unsigned target = (unsigned)size;
	char *ptr = pBuffer;

	(void)fd;

	while (have_read < target) {
		while (CONSOLE_HasData() == 0) {
		#ifdef __OPTIMIZE__
			__NOP();
		#endif
		}
		r = CONSOLE_Read(ptr, target);
		have_read += r;
	}

	return size;
}
#endif

int _write(int fd, char *pBuffer, int size)
{
	(void)fd;

#if CONSOLE_CFG_USE_RTT
    SEGGER_RTT_Write(CONSOLE_CFG_RTT_INDEX, pBuffer, (unsigned)size);
#elif CONSOLE_CFG_USE_UART
    sci_b_uart_instance_ctrl_t *ctrl = (sci_b_uart_instance_ctrl_t *)CONSOLE_CFG_UART_INSTANCE.p_ctrl;
    for (int i = 0; i < size; i++) {
    		ctrl->p_reg->TDR_BY = (uint8_t)pBuffer[i];
    		while ((ctrl->p_reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0) {}
    }
#elif CONSOLE_CFG_USE_USB
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_RPMSG
    /* TODO Wait implementation */
#elif CONSOLE_CFG_USE_LCD
    for (int i = 0; i < size; i++) {
    	CONSOLE_LCD_Putchar(pBuffer[i]);
    }
#elif CONSOLE_CFG_USE_CUSTOM
    CONSOLE_PutChar(ch);
#endif

	return size;
}

#endif

/*********************************************************************************************************************
 * Different IO implement. Defined here
 *********************************************************************************************************************/
#if CONSOLE_CFG_USE_RTT

void CONSOLE_Init(void)
{
#if CONSOLE_CFG_RTT_DUAL_CORE == 0
	SEGGER_RTT_Init();
#endif

#if I_AM_PRINTER
	memset(&s_icache, 0, sizeof(s_icache));
	memset(&s_ocache, 0, sizeof(s_ocache));
	memset(&s_rec_queue, 0, sizeof(s_rec_queue));
	s_printer.should_handle = 0;
	s_printer.status = PRINTER_UNCONNECT;
	s_printer.sm_connect = CONNECT_SM_WAIT_IRQ;
#endif
}

#elif CONSOLE_CFG_USE_UART

static uint8_t s_rx_buf[CONSOLE_CFG_RX_BUF_SIZE];
static uint32_t s_head_index;
static uint32_t s_tail_index;
/**
 * @brief   Checks if at least one character for reading is available at rx buf
 * @return  0: no data, 1: data avaliable
 */
int CONSOLE_HasData(void)
{
    return s_head_index != s_tail_index ? 1 : 0;
}

/**
 * @brief	Init io peripheral
 * @return	0: Success, other: failed
 */
void CONSOLE_Init(void)
{
	memset(s_rx_buf, 0, CONSOLE_CFG_RX_BUF_SIZE);
	s_head_index = 0;
	s_tail_index = 0;
	R_SCI_B_UART_Open(CONSOLE_CFG_UART_INSTANCE.p_ctrl, CONSOLE_CFG_UART_INSTANCE.p_cfg);
#if I_AM_PRINTER
	memset(&s_icache, 0, sizeof(s_icache));
	memset(&s_ocache, 0, sizeof(s_ocache));
	memset(&s_rec_queue, 0, sizeof(s_rec_queue));
	s_printer.should_handle = 0;
	s_printer.status = PRINTER_UNCONNECT;
	s_printer.sm_connect = CONNECT_SM_WAIT_IRQ;
#endif
}

/**
 * @brief   Read data from rx buf
 * @param   buffer Data will be saved here
 * @param   size   Receive size
 * @return  Actual size read
 */
unsigned CONSOLE_Read(void* buffer, unsigned size)
{
    unsigned read_num;
    uint32_t rx_length, remain;

    uint8_t *p8 = (uint8_t *)buffer;

    if (s_head_index < s_tail_index) {
        rx_length = s_tail_index - s_head_index;
        read_num = rx_length > size ? size : rx_length;
        memcpy(p8, &s_rx_buf[s_head_index], read_num);
        s_tail_index -= read_num;
    }
    else if (s_head_index == s_tail_index) {
        read_num = 0;
    }
    else {
        rx_length = CONSOLE_CFG_RX_BUF_SIZE - (s_head_index - s_tail_index);
        read_num = rx_length > size ? size : rx_length;

        memcpy(p8, &s_rx_buf[s_head_index], CONSOLE_CFG_RX_BUF_SIZE - s_head_index);
        if (read_num < (CONSOLE_CFG_RX_BUF_SIZE - s_head_index)) {
            s_head_index += read_num;
            if (s_head_index == CONSOLE_CFG_RX_BUF_SIZE) {
                s_head_index = 0;
            }
        }
        else {
            remain = read_num - (CONSOLE_CFG_RX_BUF_SIZE - s_head_index);
            memcpy(&p8[CONSOLE_CFG_RX_BUF_SIZE - s_head_index], s_rx_buf, remain);
            s_head_index += remain;
        }
    }

    return read_num;
}

void CONSOLE_CFG_UART_CALLBACK(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR) {
        s_rx_buf[s_tail_index] = (uint8_t)p_args->data;
        s_tail_index++;
        if (s_tail_index == CONSOLE_CFG_RX_BUF_SIZE) {
            s_tail_index = 0;
        }
        if (s_tail_index == s_head_index) {
            s_head_index++;
            if (s_head_index == CONSOLE_CFG_RX_BUF_SIZE) {
                s_head_index = 0;
            }
        }
	#if CONSOLE_CFG_USE_RPMSG
        bool notice = false;
        struct InputPackage *ip = &s_icache[s_itail];

        ip->data[ip->length] = (char)p_args->data;
        ip->length++;
        if ((ip->length == (CONSOLE_CFG_RPMSG_R_SIZE - 1)) || (p_args->data == '\n')) {
        	notice = true;
        	ip->data[ip->length] = '\0';
        	s_itail++;
        	if (s_itail == CONSOLE_CFG_RPMSG_R_NUM) {
        		s_itail = 0;
        	}
        	if (s_itail == s_ihead) {
        		s_ihead++;
        	}
        	if (s_ihead == CONSOLE_CFG_RPMSG_R_NUM) {
        		s_ihead = 0;
        	}
        }

        if (notice && (s_printer.status == PRINTER_CONNECT)) {
        	s_printer.should_handle = 1;
        	s_printer.status = PRINTER_SEND_DATA;
        	s_printer.sm_send = SEND_SM_WAIT_ACK;
        	R_IPC_EventGenerate(IPC_INSTANCE.p_ctrl, IPC_GENERATE_EVENT_IRQ0);
        }
	#endif
    }
}

#elif CONSOLE_CFG_USE_LCD

extern void CONSOLE_LCD_Init(void);

int CONSOLE_HasData(void)
{
	return 0;
}

void CONSOLE_Init(void)
{
	CONSOLE_LCD_Init();
#if I_AM_PRINTER
	memset(&s_icache, 0, sizeof(s_icache));
	memset(&s_ocache, 0, sizeof(s_ocache));
	memset(&s_rec_queue, 0, sizeof(s_rec_queue));
	s_printer.should_handle = 0;
	s_printer.status = PRINTER_UNCONNECT;
	s_printer.sm_connect = CONNECT_SM_WAIT_IRQ;
#endif
}

unsigned CONSOLE_Read(void* buffer, unsigned size)
{
	(void)buffer;
	(void)size;

	return 0;
}

#elif CONSOLE_CFG_USE_RPMSG && (I_AM_PRINTER == 0)

int CONSOLE_HasData(void)
{
	if (s_requester.status != REQUESTER_CONNECT) {
		return 0;
	}

	return s_rec_queue.size ? 1 : 0;
}

void CONSOLE_Init(void)
{
	memset(&s_rec_queue, 0, sizeof(s_rec_queue));
	s_requester.status = REQUESTER_UNCONNECT;
	s_requester.sm_connect = CONNECT_SM_WAIT_ACK;
	s_requester.sm_send = SEND_SM_DONE;
	s_requester.should_handle = 1;
	R_IPC_EventGenerate(IPC_INSTANCE.p_ctrl, IPC_GENERATE_EVENT_IRQ0);
	while (s_requester.status != REQUESTER_CONNECT) {
		R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
	}
#if __CONSOLE_DEBUG
	LOG_D(__FUNCTION__, "REQUESTER_CONNECT");
#endif
}

unsigned CONSOLE_Read(void* buffer, unsigned size)
{
	uint16_t i;

	uint8_t *p8 = (uint8_t *)buffer;
	uint32_t actual_num = 0;
	struct InputPackage *ip = NULL;

	if (s_requester.status == REQUESTER_UNCONNECT) {
		return 0;
	}

	if ((buffer == NULL) || (size == 0)) {
		return 0;
	}

	if (s_rec_queue.size == 0) {
		return 0;
	}

	while (s_requester.status == REQUESTER_RECEIVE_DATA) {
		R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
	}

	CONSOLE_LockTake(RPMSG_LOCK_INPUT);
	ip = (struct InputPackage *)s_rec_queue.addr[0];
	IO_LOCK_TAKE(ip);
	while (size) {
		if (size >= ip->length) {
			memcpy(p8, ip->data, ip->length);
			ip->length = 0;
			size -= ip->length;
			actual_num += ip->length;

			s_rec_queue.size--;
			for (i = 0; i < s_rec_queue.size; i++) {
				s_rec_queue.addr[i] = s_rec_queue.addr[i+1];
			}
			if (s_rec_queue.size == 0) {
				size = 0;
			}
		}
		else {
			memcpy(p8, ip->data, size);
			__DSB();
			memcpy(ip->data, &ip->data[size], ip->length - size);
			ip->length -= size;

			actual_num += size;
			size = 0;
		}
	}
	IO_LOCK_GIVE(ip);
	CONSOLE_LockGive(RPMSG_LOCK_INPUT);

	return actual_num;
}

void CONSOLE_IPCIrqHandle(void)
{
	switch (s_requester.status) {
	case REQUESTER_CONNECT:
	#if __CONSOLE_DEBUG
		LOG_D(__FUNCTION__, "Get IRQ0");
	#endif
		s_requester.should_handle = 1;
		R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
		break;
	case REQUESTER_RECEIVE_DATA:
		break;
	case REQUESTER_SEND_DATA:
		break;
	case REQUESTER_UNCONNECT:
		break;
	default:
		break;
	}
}

void CONSOLE_IPCMsgHandle(uint32_t msg)
{
	uint16_t i;

	if (s_requester.should_handle == 0) {
	#if __CONSOLE_DEBUG
		LOG_I(__FUNCTION__, "Ignore msg: 0x%X", msg);
	#endif
		return;
	}

	switch (s_requester.status) {
	case REQUESTER_CONNECT:
		switch (msg) {
		case IPC_MSG_QUERY:
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_READY);
			break;
		case IPC_MSG_SEND:
			s_requester.status = REQUESTER_RECEIVE_DATA;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
			break;
		default:
			break;
		}
		break;
	case REQUESTER_RECEIVE_DATA:
		s_rec_queue.addr[s_rec_queue.size] = msg;
		s_rec_queue.size++;
		if (s_rec_queue.size == CONSOLE_CFG_RPMSG_R_NUM) {
			s_rec_queue.size--;
			for (i = 0; i < (CONSOLE_CFG_RPMSG_R_NUM - 1); i++) {
				s_rec_queue.addr[i] = s_rec_queue.addr[i - 1];
			}
		}
		s_requester.should_handle = 0;
		s_requester.status = REQUESTER_CONNECT;
		break;
	case REQUESTER_SEND_DATA:
		smSendInIPCMsg(msg);
		break;
	case REQUESTER_UNCONNECT:
		smConnectInIPCMsg(msg);
		break;
	default:
		break;
	}
}

void CONSOLE_LockGive(CONSOLE_RPMSGLockEnum elock) __attribute__((weak));
void CONSOLE_LockGive(CONSOLE_RPMSGLockEnum elock)
{
	(void)elock;

	__enable_irq();
}

void CONSOLE_LockTake(CONSOLE_RPMSGLockEnum elock) __attribute__((weak));
void CONSOLE_LockTake(CONSOLE_RPMSGLockEnum elock)
{
	(void)elock;

	__disable_irq();
}

static void smConnectInIPCMsg(uint32_t msg)
{
	switch (s_requester.sm_connect) {
	case CONNECT_SM_DONE:
		break;
	case CONNECT_SM_WAIT_ACK:
		if (msg == IPC_MSG_ACK) {
			s_requester.sm_connect = CONNECT_SM_WAIT_OCACHE;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
		}
		break;
	case CONNECT_SM_WAIT_OCACHE:
		s_requester.should_handle = 0;
		s_requester.sm_connect = CONNECT_SM_DONE;
		s_requester.status = REQUESTER_CONNECT;
		s_ocache = (struct OutputPackage *)msg;
	#if __CONSOLE_DEBUG
		LOG_D(__FUNCTION__, "Get s_ocache addr: 0x%X", msg);
	#endif
		break;
	default:
		break;
	}
}

static void smSendInIPCMsg(uint32_t msg)
{
	switch (s_requester.sm_send) {
	case SEND_SM_DONE:
		break;
	case SEND_SM_GET_ACK:
		if (msg == IPC_MSG_ACK) {
		#if __CONSOLE_DEBUG
			LOG_D(__FUNCTION__, "s_ohead = %u, s_otail = %u", s_ohead, s_otail);
			LOG_D(__FUNCTION__, "Send addr: 0x%p", &s_ocache[s_ohead]);
		#endif
			s_requester.sm_send = SEND_SM_SEND_ADDR;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, (uint32_t)&s_ocache[s_ohead] - TCM_OFFSET);
			s_ohead++;
			if (s_ohead == CONSOLE_CFG_RPMSG_W_NUM) {
				s_ohead = 0;
			}
		}
		else {
		#if __CONSOLE_DEBUG
			LOG_I(__FUNCTION__, "SEND_SM_GET_ACK: 0x%X", msg);
		#endif
		}
		break;
	case SEND_SM_QUERY:
		if (msg == IPC_MSG_READY) {
			s_requester.sm_send = SEND_SM_GET_ACK;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_SEND);
		}
		else {
		#if __CONSOLE_DEBUG
			LOG_I(__FUNCTION__, "SEND_SM_QUERY: 0x%X", msg);
		#endif
		}
		break;
	case SEND_SM_SEND_ADDR:
		if (msg == IPC_MSG_GET) {
			if (s_ohead == s_otail) {
				s_requester.should_handle = 0;
				s_requester.status = REQUESTER_CONNECT;
				s_requester.sm_send = SEND_SM_DONE;
			#if __CONSOLE_DEBUG
				LOG_D(__FUNCTION__, "End");
			#endif
			}
			else {
				s_requester.sm_send = SEND_SM_WAIT_ACK;
				R_IPC_EventGenerate(IPC_INSTANCE.p_ctrl, IPC_GENERATE_EVENT_IRQ0);
			#if __CONSOLE_DEBUG
				LOG_D(__FUNCTION__, "Continue");
			#endif
			}
		}
		else {
		#if __CONSOLE_DEBUG
			LOG_I(__FUNCTION__, "SEND_SM_SEND_ADDR: 0x%X", msg);
		#endif
		}
		break;
	case SEND_SM_WAIT_ACK:
		if (msg == IPC_MSG_ACK) {
			s_requester.sm_send = SEND_SM_QUERY;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_QUERY);
		}
		else {
		#if __CONSOLE_DEBUG
			LOG_I(__FUNCTION__, "SEND_SM_WAIT_ACK: 0x%X", msg);
		#endif
		}
		break;
	default:
		break;
	}
}

#else

int CONSOLE_HasData(void) __attribute__((weak));
int CONSOLE_HasData(void)
{
	return 0;
}

void CONSOLE_Init(void) __attribute__((weak));
void CONSOLE_Init(void)
{}

void CONSOLE_PutChar(char ch) __attribute__((weak));
void CONSOLE_PutChar(char ch)
{
	(void)ch;
}

unsigned CONSOLE_Read(void* buffer, unsigned size) __attribute__((weak));
unsigned CONSOLE_Read(void* buffer, unsigned size)
{
	(void)buffer;
	(void)size;

	return 0;
}

#endif

#if I_AM_PRINTER
void CONSOLE_IPCIrqHandle(void)
{
	switch (s_printer.status) {
	case PRINTER_CONNECT:
		s_printer.should_handle = 1;
		R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
		break;
	case PRINTER_RECEIVE_DATA:
		break;
	case PRINTER_SEND_DATA:
		break;
	case PRINTER_UNCONNECT:
		smConnectInIPCIrq();
		break;
	default:
		break;
	}
}

void CONSOLE_IPCMsgHandle(uint32_t msg)
{
	uint16_t i;

	if (s_printer.should_handle == 0) {
	#if __CONSOLE_DEBUG
		LOG_I(__FUNCTION__, "Ignore msg: 0x%X", msg);
	#endif
		return;
	}

	switch (s_printer.status) {
	case PRINTER_CONNECT:
		switch (msg) {
		case IPC_MSG_QUERY:
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_READY);
			break;
		case IPC_MSG_SEND:
			s_printer.status = PRINTER_RECEIVE_DATA;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
			break;
		default:
			break;
		}
		break;
	case PRINTER_RECEIVE_DATA:
		if (s_rec_queue.size == CONSOLE_CFG_RPMSG_W_NUM) {
			s_rec_queue.size--;
			for (i = 0; i < (CONSOLE_CFG_RPMSG_W_NUM - 1); i++) {
				s_rec_queue.addr[i] = s_rec_queue.addr[i+1];
			}
		}
		s_rec_queue.addr[s_rec_queue.size] = msg;
		s_rec_queue.size++;
		s_printer.should_handle = 0;
		s_printer.status = PRINTER_CONNECT;
		R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_GET);
	#if __CONSOLE_DEBUG
		LOG_D(__FUNCTION__, "Get addr: 0x%X", msg);
	#endif
		break;
	case PRINTER_SEND_DATA:
		smSendInIPCMsg(msg);
		break;
	case PRINTER_UNCONNECT:
		smConnectInIPCMsg(msg);
		break;
	default:
		break;
	}
}

void CONSOLE_Task(void)
{
	uint16_t i;
	uint32_t u32i;

	struct OutputPackage *op = NULL;

	if ((s_printer.status == PRINTER_RECEIVE_DATA) || (s_printer.status == PRINTER_UNCONNECT)) {
		return;
	}

	if (s_rec_queue.size) {
		__disable_irq();
	#if __CONSOLE_DEBUG
		LOG_D(__FUNCTION__, "s_rec_queue.size: %u", s_rec_queue.size);
	#endif
		for (i = 0; i < s_rec_queue.size; i++) {
			op = (struct OutputPackage *)s_rec_queue.addr[i];
			IO_LOCK_TAKE(op);
			__DMB();
			for (u32i = 0; u32i < op->length; u32i++) {
				__fputc(op->data[u32i], NULL);
			}
			memset(op, 0, sizeof(struct OutputPackage));
			__DMB();
			IO_LOCK_GIVE(op);
		}
		s_rec_queue.size = 0;
		__enable_irq();
	}
#if CONSOLE_CFG_USE_RTT
	char c;

	bool notice = false;
	struct InputPackage *ip = &s_icache[s_itail];

	if (SEGGER_RTT_HasKey() == 0) {
		return;
	}

	while (SEGGER_RTT_HasData(0)) {
		SEGGER_RTT_Read(0, &c, 1);
		ip->data[ip->length] = c;
		ip->length++;
		if ((ip->length == (CONSOLE_CFG_RPMSG_R_SIZE - 1)) || (c == '\n')) {
			notice = true;
			ip->data[ip->length] = '\0';
			s_itail++;
			if (s_itail == CONSOLE_CFG_RPMSG_R_NUM) {
				s_itail = 0;
			}
			if (s_itail == s_ihead) {
				s_ihead++;
			}
			if (s_ihead == CONSOLE_CFG_RPMSG_R_NUM) {
				s_ihead = 0;
			}
		}
	}

	if (notice && (s_printer.status == PRINTER_CONNECT)) {
		s_printer.should_handle = 1;
		s_printer.status = PRINTER_SEND_DATA;
		s_printer.sm_send = SEND_SM_WAIT_ACK;
		R_IPC_EventGenerate(IPC_INSTANCE.p_ctrl, IPC_GENERATE_EVENT_IRQ0);
	}
#endif
}

static void smConnectInIPCIrq(void)
{
	switch (s_printer.sm_connect) {
	case CONNECT_SM_DONE:
		break;
	case CONNECT_SM_WAIT_ACK:
		break;
	case CONNECT_SM_WAIT_IRQ:
		s_printer.should_handle = 1;
		s_printer.sm_connect = CONNECT_SM_WAIT_ACK;
		R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_ACK);
		break;
	default:
		break;
	}
}

static void smConnectInIPCMsg(uint32_t msg)
{
	switch (s_printer.sm_connect) {
	case CONNECT_SM_DONE:
		break;
	case CONNECT_SM_WAIT_ACK:
		if (msg == IPC_MSG_ACK) {
			s_printer.should_handle = 0;
			s_printer.sm_connect = CONNECT_SM_DONE;
			s_printer.status = PRINTER_CONNECT;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, (uint32_t)&s_ocache + TCM_OFFSET);
		#if __CONSOLE_DEBUG
			LOG_D(__FUNCTION__, "PRINTER_CONNECT");
		#endif
		}
		break;
	case CONNECT_SM_WAIT_IRQ:
		break;
	default:
		break;
	}
}

static void smSendInIPCMsg(uint32_t msg)
{
	switch (s_printer.sm_send) {
	case SEND_SM_DONE:
		break;
	case SEND_SM_GET_ACK:
		if (msg == IPC_MSG_ACK) {
			s_printer.should_handle = 0;
			s_printer.status = PRINTER_CONNECT;
			s_printer.sm_send = SEND_SM_DONE;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, (uint32_t)&s_icache[s_ihead] + TCM_OFFSET);

			s_ihead++;
			if (s_ihead == CONSOLE_CFG_RPMSG_R_NUM) {
				s_ihead = 0;
			}
		}
		break;
	case SEND_SM_QUERY:
		switch (msg) {
		case IPC_MSG_BUSY:
			break;
		case IPC_MSG_READY:
			s_printer.sm_send = SEND_SM_GET_ACK;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_SEND);
			break;
		default:
			break;
		}
		break;
	case SEND_SM_WAIT_ACK:
		if (msg == IPC_MSG_ACK) {
			s_printer.sm_send = SEND_SM_QUERY;
			R_IPC_MessageSend(IPC_INSTANCE.p_ctrl, IPC_MSG_QUERY);
		}
		break;
	default:
		break;
	}
}

#endif
