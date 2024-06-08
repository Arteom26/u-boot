/*
 * Copyright (c) 2014	Nuvoton Technology Corp.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Description:   NUC980 UART driver
 */
#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <stdint.h>

#include <serial.h>
#include <watchdog.h>

// TODO: Move into a header?
#define WB_DATA_BITS_5    0x00
#define WB_DATA_BITS_6    0x01
#define WB_DATA_BITS_7    0x02
#define WB_DATA_BITS_8    0x03

#define WB_STOP_BITS_1    0x00
#define WB_STOP_BITS_2    0x04

#define WB_PARITY_NONE    0x00
#define WB_PARITY_ODD     0x00
#define WB_PARITY_EVEN    0x10

#define LEVEL_1_BYTE      0x00
#define LEVEL_4_BYTES     0x40
#define LEVEL_8_BYTES     0x80
#define LEVEL_14_BYTES    0xC0

#define TX_RX_FIFO_RESET    0x06
#define ENABLE_DLAB         0x80    // Enable Divider Latch Access
#define DISABLE_DLAB        0x7F
#define ENABLE_TIME_OUT     (0x80+0x20)
#define THR_EMPTY           0x20    // Transmit Holding Register Empty
#define RX_DATA_READY       0x01
#define RX_FIFO_EMPTY       0x4000

#define REG_COM_TX     (0x0)    /* (W) TX buffer */
#define REG_COM_RX     (0x0)    /* (R) RX buffer */
#define REG_COM_LSB    (0x0)    /* Divisor latch LSB */
#define REG_COM_MSB    (0x04)   /* Divisor latch MSB */
#define REG_COM_IER    (0x04)   /* Interrupt enable register */
#define REG_COM_IIR    (0x08)   /* (R) Interrupt ident. register */
#define REG_COM_FCR    (0x08)   /* (W) FIFO control register */
#define REG_COM_LCR    (0x0C)   /* Line control register */
#define REG_COM_MCR    (0x10)   /* Modem control register */
#define	REG_COM_LSR    (0x14)   /* (R) Line status register */
#define REG_COM_MSR    (0x18)   /* (R) Modem status register */
#define	REG_COM_TOR    (0x1C)   /* (R) Time out register */
#define REG_MFSEL      0xB000000C
#define UART0_BASE     0xB0070000

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
#define UART0_BA	0xB0070000 /* UART0 Control (High-Speed UART) */
#define UART0		((UART_TypeDef *)UART0_BA)
#define REG_PF_PUSEL	0xB0004170

DECLARE_GLOBAL_DATA_PTR;

#if (defined(CONFIG_DM_SERIAL) && !defined(CONFIG_SPL_BUILD)) || (defined(CONFIG_SPL_DM_SERIAL) && defined(CONFIG_SPL_BUILD))

struct nuc980_serial_plat {
	UART_TypeDef *reg;
	uint32_t uart_clk;		/* frequency of uart clock source */
};

static int nuc980_serial_probe(struct udevice *dev);
static int nuc980_serial_getc(struct udevice *dev);
static int nuc980_serial_setbrg(struct udevice *dev, int baudrate);
static int nuc980_serial_putc(struct udevice *dev, const char ch);

int nuc980_serial_getc (struct udevice *dev)
{
	struct nuc980_serial_plat *plat = dev_get_plat(dev);
	UART_TypeDef *uart = plat->reg;

	if (!(uart->FSR & (1 << 14))) {
		return (uart->x.RBR);
	}
	return -EAGAIN;
}

// TODO: Actually implement
int nuc980_serial_setbrg (struct udevice *dev, int baudrate)
{
	struct nuc980_serial_plat *plat = dev_get_plat(dev);
	UART_TypeDef *uart = plat->reg;

	/* UART0 line configuration for (115200,n,8,1) */
	uart->LCR |=0x07;
	uart->BAUD = 0x30000066;	/* 12MHz reference clock input, 115200 */
	uart->FCR |=0x02;		// Reset UART0 Rx FIFO

	return -EAGAIN;
}

int nuc980_serial_putc(struct udevice *dev, const char ch)
{
	struct nuc980_serial_plat *plat = dev_get_plat(dev);
	UART_TypeDef *uart = plat->reg;
	
	while ((uart->FSR & 0x800000)); //waits for TX_FULL bit is clear
	uart->x.THR = ch;
	if(ch == '\n') {
		while ((uart->FSR & 0x800000)); //waits for TX_FULL bit is clear
		uart->x.THR = '\r';
	}

	return 0;
}

#include <debug_uart.h>
int nuc980_serial_probe(struct udevice *dev)
{
	struct nuc980_serial_plat *plat = dev_get_plat(dev);

#if ((defined(CONFIG_SPL_OF_CONTROL) && defined(CONFIG_SPL_BUILD)) || (defined(CONFIG_OF_CONTROL) && !defined(CONFIG_SPL_BUILD)))
	const void* blob = gd->fdt_blob;
	int node = dev_of_offset(dev);
	printascii("Probing NUC980 serial driver....\n");
	printhex8(blob);
	printch('\n');
	printhex8(node);
	
	plat->reg = (UART_TypeDef* ) fdtdec_get_addr(blob, node, "reg");
	plat->uart_clk = fdtdec_get_int(blob, node, "input-click", -1);
#else
	plat->reg = UART0;// TODO: Read the property from the device tree
	plat->uart_clk = 12000000;// 12MHz reference clock
#endif
	__raw_writel(__raw_readl(REG_PCLKEN0) | 0x10000, REG_PCLKEN0);  // UART clk on
	__raw_writel(__raw_readl(REG_HCLKEN) | 0x800, REG_HCLKEN);  	// GPIO clk on
	__raw_writel(__raw_readl(REG_PF_PUSEL) | 0x400000, REG_PF_PUSEL);	// PF11 (UART0 Rx) pull up
	__raw_writel((__raw_readl(REG_MFP_GPF_H) & 0xfff00fff) | 0x11000, REG_MFP_GPF_H); // UART0 multi-function

	/* UART0 line configuration for (115200,n,8,1) */
	plat->reg->LCR |=0x07;
	plat->reg->BAUD = 0x30000066;	/* 12MHz reference clock input, 115200 */
	plat->reg->FCR |=0x02;		// Reset UART0 Rx FIFO

	return 0;
}

static const struct dm_serial_ops nuc980_serial_ops = {
	.getc = nuc980_serial_getc,
	.setbrg = nuc980_serial_setbrg,
	.putc = nuc980_serial_putc,
};

static const struct udevice_id nuc980_serial_ids[] = {
	{ .compatible = "nuvoton,nuc980-uart" },
	{ }
};

U_BOOT_DRIVER(nuvoton_nuc980_uart) = {
	.name	= "nuvoton_nuc980_uart",
	.id	= UCLASS_SERIAL,
	.of_match = nuc980_serial_ids,
	.plat_auto  = sizeof(struct nuc980_serial_plat),
	.probe = nuc980_serial_probe,
	.ops	= &nuc980_serial_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

#else

int nuc980_serial_init (void);
void nuc980_serial_putc (const char ch);
void nuc980_serial_puts (const char *s);
int nuc980_serial_getc (void);
int nuc980_serial_tstc (void);
void nuc980_serial_setbrg (void);

/*
 * Initialise the serial port with the given baudrate. The settings are always 8n1.
 */

u32 baudrate = CONFIG_BAUDRATE;
u32 ext_clk  = EXT_CLK;

int nuc980_serial_init (void)
{
	__raw_writel(__raw_readl(REG_PCLKEN0) | 0x10000, REG_PCLKEN0);  // UART clk on
	__raw_writel(__raw_readl(REG_HCLKEN) | 0x800, REG_HCLKEN);  	// GPIO clk on
	__raw_writel(__raw_readl(REG_PF_PUSEL) | 0x400000, REG_PF_PUSEL);	// PF11 (UART0 Rx) pull up
	__raw_writel((__raw_readl(REG_MFP_GPF_H) & 0xfff00fff) | 0x11000, REG_MFP_GPF_H); // UART0 multi-function

	/* UART0 line configuration for (115200,n,8,1) */
	UART0->LCR |=0x07;
	UART0->BAUD = 0x30000066;	/* 12MHz reference clock input, 115200 */
	UART0->FCR |=0x02;		// Reset UART0 Rx FIFO

	return 0;
}

void nuc980_serial_putc (const char ch)
{
	while ((UART0->FSR & 0x800000)); //waits for TX_FULL bit is clear
	UART0->x.THR = ch;
	if(ch == '\n') {
		while((UART0->FSR & 0x800000)); //waits for TX_FULL bit is clear
		UART0->x.THR = '\r';
	}
}

void nuc980_serial_puts (const char *s)
{
	while (*s) {
		nuc980_serial_putc (*s++);
	}
}

int nuc980_serial_getc (void)
{
	while (1) {
		if (!(UART0->FSR & (1 << 14))) {
			return (UART0->x.RBR);
		}
		// WATCHDOG_RESET();
	}
}

int nuc980_serial_tstc (void)
{
	return (!((__raw_readl(UART0_BASE + REG_COM_MSR) & RX_FIFO_EMPTY)>>14));
}

void nuc980_serial_setbrg (void)
{

	return;
}

static struct serial_device nuc980_serial_drv = {
	.name   = "nuc980_serial",
	.start  = nuc980_serial_init,
	.stop   = NULL,
	.setbrg = nuc980_serial_setbrg,
	.putc   = nuc980_serial_putc,
	.puts   = nuc980_serial_puts,
	.getc   = nuc980_serial_getc,
	.tstc   = nuc980_serial_tstc,
};

void nuc980_serial_initialize(void)
{
	serial_register(&nuc980_serial_drv);
}

__weak struct serial_device *default_serial_console(void) {
	return &nuc980_serial_drv;
}

#endif