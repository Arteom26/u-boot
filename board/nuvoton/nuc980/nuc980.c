// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Copyright (c) 2021 Nuvoton Technology Corp.
 */

#include <common.h>
#include <dm.h>
#include <env.h>
#include <asm/io.h>
#include <asm/mach-types.h>

// Register defines
#define REG_HCLKEN      0xB0000210
#define REG_PCLKEN0     0xB0000218
#define REG_PCLKEN1     0xB000021C
#define REG_PWRON       0xB0000004
#define REG_SDIC_SIZE0  0xB0002010
#define REG_SDIC_SIZE1  0xB0002014
#define REG_CLKDIVCTL8  0xB0000240
#define REG_MFP_GPA_L	0xB0000070
#define REG_MFP_GPA_H	0xB0000074
#define REG_MFP_GPB_L	0xB0000078
#define REG_MFP_GPB_H	0xB000007C
#define REG_MFP_GPC_L	0xB0000080
#define REG_MFP_GPC_H	0xB0000084
#define REG_MFP_GPD_L	0xB0000088
#define REG_MFP_GPD_H	0xB000008C
#define REG_MFP_GPE_L	0xB0000090
#define REG_MFP_GPE_H	0xB0000094
#define REG_MFP_GPF_L	0xB0000098
#define REG_MFP_GPF_H	0xB000009C
#define REG_MFP_GPG_L	0xB00000A0
#define REG_MFP_GPG_H	0xB00000A4
#define REG_MFP_GPH_L	0xB00000A8
#define REG_MFP_GPH_H	0xB00000AC
#define REG_MFP_GPI_L	0xB00000B0
#define REG_MFP_GPI_H	0xB00000B4
#define SYS_RSTDEBCTL	0xB000010C
#define SYS_REGWPCTL    0xB00001FC

DECLARE_GLOBAL_DATA_PTR;

static void NUC980_UnLock(void)
{
	do {
		writel(0x59, SYS_REGWPCTL);
		writel(0x16, SYS_REGWPCTL);
		writel(0x88, SYS_REGWPCTL);
		//wait for write-protection disabled indicator raised
	} while(!(readl(SYS_REGWPCTL) & 1));
}

static void NUC980_Lock(void)
{
	writel(0x0, SYS_REGWPCTL);
}

/* Enable reset debounce and set debounce counter to 0xFFF */
static void enable_ResetDebounce(void)
{
	NUC980_UnLock();
	writel(0x80000FFF, SYS_RSTDEBCTL);
	NUC980_Lock();
}

static unsigned int sdram_size(unsigned int config)
{
	unsigned int size = 0;

	config &= 0x7;

	switch (config) {
	case 0:
		size = 0;
		break;
	case 1:
		size = 0x200000;
		break;
	case 2:
		size = 0x400000;
		break;
	case 3:
		size = 0x800000;
		break;
	case 4:
		size = 0x1000000;
		break;
	case 5:
		size = 0x2000000;
		break;
	case 6:
		size = 0x4000000;
		break;
	case 7:
		size = 0x8000000;
		break;
	}

	return(size);
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	enable_ResetDebounce();

	return 0;
}

int dram_init(void)
{
	gd->ram_size = sdram_size(readl(REG_SDIC_SIZE0)) + sdram_size(readl(REG_SDIC_SIZE1));

	return(0);
}

int last_stage_init(void)
{
	// board_set_console();

	return 0;
}
