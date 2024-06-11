/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014
 * Arteom Katkov
 */
#ifndef __NUC980_H__
#define __NUC980_H__

#define DEBUG
#define LOG_DEBUG

#define EXT_CLK	        12000000        /* 12 MHz crystal */

#define CONFIG_SYS_SDRAM_BASE   0
#define CONFIG_NR_DRAM_BANKS    2     /* there are 2 sdram banks for nuc980 */

// #define CONFIG_SPL_OF_CONTROL   1
// #define CONFIG_SPL_OF_PLATDATA  1
// #define CONFIG_SPL_OF_PLATDATA_PARENT   1

/* base address for uboot */
#define CONFIG_SYS_PHY_UBOOT_BASE       (CONFIG_SYS_SDRAM_BASE + 0xE00000)

#define CONFIG_SYS_NAND_MAX_CHIPS   1
#define CFG_SYS_NAND_BASE           0xB000D000
#define CFG_SYS_NAND_U_BOOT_DST     0xE00000
#define CFG_SYS_NAND_U_BOOT_START   0xE00000
#define CFG_SYS_NAND_U_BOOT_SIZE    (500 * 1024)

#define CFG_SYS_NAND_ECCBYTES       6
#define CFG_SYS_NAND_ECCSIZE        512
#define CFG_SYS_NAND_ECCPOS         {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,\
                                     0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,\
                                     0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,\
                                     0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D}

#endif
