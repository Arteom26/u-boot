#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <debug_uart.h>

DECLARE_GLOBAL_DATA_PTR;

typedef struct {
	union {
		volatile unsigned int RBR;	/*!< Offset: 0x0000   Receive Buffer Register		*/
		volatile unsigned int THR;	/*!< Offset: 0x0000   Transmit Holding Register		*/
	} x;
	volatile unsigned int IER;		/*!< Offset: 0x0004   Interrupt Enable Register		*/
	volatile unsigned int FCR;		/*!< Offset: 0x0008   FIFO Control Register		*/
	volatile unsigned int LCR;		/*!< Offset: 0x000C   Line Control Register		*/
	volatile unsigned int MCR;		/*!< Offset: 0x0010   Modem Control Register		*/
	volatile unsigned int MSR;		/*!< Offset: 0x0014   Modem Status Register		*/
	volatile unsigned int FSR;		/*!< Offset: 0x0018   FIFO Status Register		*/
	volatile unsigned int ISR;		/*!< Offset: 0x001C   Interrupt Status Register		*/
	volatile unsigned int TOR;		/*!< Offset: 0x0020   Time Out Register			*/
	volatile unsigned int BAUD;		/*!< Offset: 0x0024   Baud Rate Divisor Register	*/
	volatile unsigned int IRCR;		/*!< Offset: 0x0028   IrDA Control Register		*/
	volatile unsigned int ALTCON;		/*!< Offset: 0x002C   Alternate Control/Status Register	*/
	volatile unsigned int FUNSEL;		/*!< Offset: 0x0030   Function Select Register		*/
} UART_TypeDef;

#define GCR_BA		0xB0000000 /* Global Control */
#define REG_MFP_GPF_H	(GCR_BA+0x09C)  /* GPIOF High Byte Multiple Function Control Register */
#define REG_HCLKEN	0xB0000210
#define REG_PCLKEN0	0xB0000218
#define UART0		((UART_TypeDef *)CONFIG_DEBUG_UART_BASE)
#define REG_PF_PUSEL	0xB0004170

static inline void _debug_uart_init(void)
{
    __raw_writel(__raw_readl(REG_PCLKEN0) | 0x10000, REG_PCLKEN0);  // UART clk on
	__raw_writel(__raw_readl(REG_HCLKEN) | 0x800, REG_HCLKEN);  	// GPIO clk on
	__raw_writel(__raw_readl(REG_PF_PUSEL) | 0x400000, REG_PF_PUSEL);	// PF11 (UART0 Rx) pull up
	__raw_writel((__raw_readl(REG_MFP_GPF_H) & 0xfff00fff) | 0x11000, REG_MFP_GPF_H); // UART0 multi-function

	/* UART0 line configuration for (115200,n,8,1) */
	UART0->LCR |=0x07;
	UART0->BAUD = 0x30000000 | ((CONFIG_DEBUG_UART_CLOCK - (2 * CONFIG_BAUDRATE)) / CONFIG_BAUDRATE);	/* 12MHz reference clock input => baud = CLK / (BRD + 2) */
	UART0->FCR |=0x02;		// Reset UART0 Rx FIFO
}

static inline void _debug_uart_putc(int ch)
{
    while ((UART0->FSR & 0x800000)) {
        // waits for TX_FULL bit is clear
    }
	UART0->x.THR = ch;
}

DEBUG_UART_FUNCS;
