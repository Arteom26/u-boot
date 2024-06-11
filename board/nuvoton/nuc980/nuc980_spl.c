/*
 * (C) Copyright 2006-2008
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
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
 */

#include <common.h>
#include <env.h>
#include <nand.h>
#include <asm/io.h>
#include <asm/spl.h>
#include <debug_uart.h>
#include <exports.h>
#include <spl.h>
#include <dm.h>

const char *spl_board_loader_name(u32 boot_device)
{
	switch (boot_device) {
		/* MMC Card Slot */
		case BOOT_DEVICE_MMC1:
			return "eMMC";
		/* SPI */
		case BOOT_DEVICE_SPI:
			return "SPI-NAND";
		default:
			return NULL;
	}
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_NAND;
}

void board_boot_order(u32 *spl_boot_list)
{
    // TODO: Dynamically configure...
    spl_boot_list[0] = BOOT_DEVICE_NAND;
    spl_boot_list[1] = BOOT_DEVICE_MMC1;
}

/*
 * The main entry for SPI NAND booting. It's necessary that SDRAM is already
 * configured and available since this code loads the main U-Boot image
 * from SPI NAND Flash into SDRAM and starts it from there.
 */
void board_init_f(unsigned long bootflag)
{
	spl_early_init();
	preloader_console_init();
	printascii("\nSPL load main U-Boot from SPI NAND Flash! \n");	
}

/* Lowlevel init isn't used on nuc980, so just provide a dummy one here */
void lowlevel_init(void) {}
