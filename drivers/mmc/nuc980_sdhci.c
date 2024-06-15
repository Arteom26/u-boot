/*
 * (C) Copyright 2018
 * Nuvoton Technology Corp. <www.nuvoton.com>
 *
 * Configuation settings for the NUC980 EV Board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/delay.h>
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <mmc.h>
#include <dm.h>

#define FMI_BA		0xB0019000 /* Flash Memory Card Interface */
#define SDH_BA		0xB0018000 /* SD Host */

/* DMAC Control Registers */
#define REG_DMACCSR             (SDH_BA+0x400)  /* DMAC Control and Status Register */
#define DMACEN           0x1
#define DMAC_SW_RST      0x2
//#define REG_DMACSAR1            (SDH_BA+0x404)  /* DMAC Transfer Starting Address Register 1 */
#define REG_DMACSAR2            (SDH_BA+0x408)  /* DMAC Transfer Starting Address Register 2 */
#define REG_DMACBCR             (SDH_BA+0x40C)  /* DMAC Transfer Byte Count Register */
#define REG_DMACIER             (SDH_BA+0x410)  /* DMAC Interrupt Enable Register */
#define REG_DMACISR             (SDH_BA+0x414)  /* DMAC Interrupt Status Register */

/* Flash Memory Card Interface Registers */
#define REG_SDH_GCTL            (SDH_BA+0x800)   /* Global Control and Status Register */
#define GCTL_RST      0x1
#define SD_EN         0x2
#define EMMC_EN       0x2

#define REG_GINTEN              (SDH_BA+0x804)   /* Global Interrupt Control Register */
#define REG_GINTSTS             (SDH_BA+0x808)   /* Global Interrupt Status Register */

/* Secure Digit Registers */
#define REG_SDCSR               (SDH_BA+0x820)   /* SD control and status register */
#define CO_EN           0x01
#define RI_EN           0x02
#define DI_EN           0x04
#define DO_EN           0x08
#define R2_EN           0x10
#define CLK74_OE        0x20
#define CLK8_OE         0x40
#define CLK_KEEP0       0x80
#define DBW             0x8000
#define REG_SDARG               (SDH_BA+0x824)   /* SD command argument register */
#define REG_SDIER               (SDH_BA+0x828)   /* SD interrupt enable register */
#define REG_SDISR               (SDH_BA+0x82C)   /* SD interrupt status register */
#define BLKD_IF         0x01
#define CRC_IF          0x02
#define CRC_7           0x04
#define CRC_16          0x08
#define SDDAT0          0x80
#define RITO_IF         0x1000
#define DITO_IF         0x2000
#define REG_SDRSP0              (SDH_BA+0x830)   /* SD receive response token register 0 */
#define REG_SDRSP1              (SDH_BA+0x834)   /* SD receive response token register 1 */
#define REG_SDBLEN              (SDH_BA+0x838)   /* SD block length register */
#define REG_SDTMOUT             (SDH_BA+0x83C)   /* SD Response/Data-in timeout register */

#define REG_CLKDIVCTL3             0xB000022C
#define REG_CLKDIVCTL9             0xB0000244
#define REG_HCLKEN 				   0xB0000210

#define MMC_CLK                 300000000  /* UCLKout */

// copy from linux/mmc/core.h
#define mmc_resp_type(cmd)      ((cmd)->resp_type & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))

#define REG_FMIDMACTL		(FMI_BA+0x400)	/* FMI DMA control register */
#define REG_FMIDMASA		(FMI_BA+0x408)	/* FMI DMA transfer starting address register */
#define REG_FMIDMABCNT		(FMI_BA+0x40C)	/* FMI DMA transfer byte count register */
#define REG_FMIDMAINTEN		(FMI_BA+0x410)	/* FMI DMA interrupt enable register */
#define REG_FMIDMAINTSTS	(FMI_BA+0x414)	/* FMI DMA interrupt status register */

#define REG_FMICTL		(FMI_BA+0x800)	/* FMI control and status register */
#define REG_FMIINTEN		(FMI_BA+0x804)	/* FMI interrupt enable register */
#define REG_FMIINTSTS		(FMI_BA+0x808)	/* FMI interrupt status register */
#define REG_EMMCCTL		(FMI_BA+0x820)	/* eMMC control register */
#define REG_EMMCCMD		(FMI_BA+0x824)	/* eMMC command argument register */
#define REG_EMMCINTEN		(FMI_BA+0x828)	/* eMMC interrupt enable register */
#define REG_EMMCINTSTS		(FMI_BA+0x82C)	/* eMMC interrupt status register */
#define REG_EMMCRESP0		(FMI_BA+0x830)	/* eMMC receiving response register 0 */
#define REG_EMMCRESP1		(FMI_BA+0x834)	/* eMMC receiving response register 1 */
#define REG_EMMCBLEN		(FMI_BA+0x838)	/* eMMC block length register */
#define REG_EMMCTOUT		(FMI_BA+0x83C)	/* eMMC Response/Data-in time-out register */

struct nuc980_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	bool isEMMC;// If true setup to use eMMC otherwise use SD
};

static int nuc980_sd_check_ready_busy(void)
{
	int cnt = 10;
	while(cnt-- > 0) {
		writel(readl(REG_SDCSR) | CLK8_OE, REG_SDCSR);
		while(readl(REG_SDCSR) & CLK8_OE);
		if(readl(REG_SDISR) & SDDAT0)
			break;
	}

	return cnt ? 0 : -1;
}

static int nuc980_emmc_check_ready_busy(void)
{
	int cnt = 10;
	while(cnt-- > 0) {
		writel(readl(REG_EMMCCTL) | CLK8_OE, REG_EMMCCTL);
		while(readl(REG_EMMCCTL) & CLK8_OE);
		if(readl(REG_EMMCINTSTS) & SDDAT0)
			break;
	}

	return cnt ? 0 : -1;
}

static int nuc980_sd_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	unsigned char *c;
	int i, j, tmp[5];
	unsigned int block_length, blocks;
	unsigned int sdcsr = readl(REG_SDCSR) & DBW;      // keep bus width. config other fields in this function

	if((readl(REG_SDCSR) & 0xF) != 0)
		if(nuc980_sd_check_ready_busy() < 0) {
			if ((readl(REG_SDISR) & SDDAT0) == 0)
				return(-ETIMEDOUT);
		}

	switch (cmd->resp_type)
	{
		case MMC_RSP_R1:
		case MMC_RSP_R1b:
			sdcsr |= RI_EN;
			break;

		case MMC_RSP_R2:
			sdcsr |= R2_EN;
			break;

		case MMC_RSP_R3:
			sdcsr |= RI_EN;
			break;

		default:
			break;
	}
	// Enable read input timeout if MMC command is not none
	if(mmc_resp_type(cmd) != MMC_RSP_NONE) {
		writel(RITO_IF, REG_SDISR);
		writel(0xFFFFFF, REG_SDTMOUT);
	}

	sdcsr |= 0x09010000; // Set SDNWR and BLK_CNT to 1, update later
	sdcsr |= (cmd->cmdidx << 8) | CO_EN;
	if (data) {
		block_length = data->blocksize;
		blocks = data->blocks;

		// printf("size %d len %d\n", block_length, blocks);
		writel(block_length - 1, REG_SDBLEN);

		if (block_length <= 0x200) {
			if (blocks < 256)
				sdcsr = (sdcsr & ~0xFF0000) | (blocks << 16);
			else
				printf("NUC980 SD Max block transfer is 255!!\n");
		}
		// printf("sdcsr 0x%x \n", sdcsr);

		if (data->flags == MMC_DATA_READ) {
			writel(DITO_IF, REG_SDISR);
			writel(0xFFFFFF, REG_SDTMOUT);
			sdcsr |= DI_EN;
			writel((unsigned int)data->dest, REG_DMACSAR2);

		} else if (data->flags == MMC_DATA_WRITE) {
			sdcsr |= DO_EN;
			writel((unsigned int)data->src, REG_DMACSAR2);
		}
		else {
			printf("Unknown MMC Data command...\n");
		}
	} else {
		block_length = 0;
		blocks = 0;
	}

	writel(cmd->cmdarg, REG_SDARG);
	// printf("arg: %x\n", cmd->cmdarg);
	writel(sdcsr, REG_SDCSR);

	// printf("%x\n",readl(REG_SDCSR));
	while (readl(REG_SDCSR) & CO_EN); //wait 'til command out complete

	// printf("MMC Response type: %x\n", cmd->resp_type);
	if(mmc_resp_type(cmd) != MMC_RSP_NONE) {
		if(mmc_resp_type(cmd) == MMC_RSP_R2) {
			while (readl(REG_SDCSR) & R2_EN);
			c = (unsigned char *)SDH_BA;
			for (i = 0, j = 0; j < 5; i += 4, j++)
				tmp[j] = (*(c + i) << 24) |
				         (*(c + i + 1) << 16) |
				         (*(c + i + 2) << 8) |
				         (*(c + i + 3));
			for (i = 0; i < 4; i++)
				cmd->response[i] = ((tmp[i] & 0x00ffffff) << 8) |
				                   ((tmp[i + 1] & 0xff000000) >> 24);
		} else {
			while (1) {
				if (readl(REG_SDISR) & RITO_IF) {
					writel(RITO_IF, REG_SDISR);
					writel(0, REG_SDTMOUT);

					printf("Error! Timed Out!");
					return(-ETIMEDOUT);
				}
				if (!(readl(REG_SDCSR) & RI_EN))
					break;
			}
			// printf("=>%x %x %x %x\n", sdcsr, readl(REG_SDBLEN), readl(REG_SDRSP0), readl(REG_SDRSP1));
			// printf("[xxxx]REG_SDISR = 0x%x\n",readl(REG_SDISR));
			cmd->response[0] = (readl(REG_SDRSP0) << 8) | (readl(REG_SDRSP1) & 0xff);
			cmd->response[1] = cmd->response[2] = cmd->response[3] = 0;
		}
	}
	if ((readl(REG_SDISR) & CRC_7) == 0) {
		writel(CRC_IF, REG_SDISR);
		if (mmc_resp_type(cmd) & MMC_RSP_CRC) {
			printf("xxxx retrun err!\n");
			return(-EILSEQ);
		}

	}


	if (data) {
		if (data->flags & MMC_DATA_READ) {
			// printf("**** %x %x %x %x\n", readl(REG_DMACCSR), readl(REG_DMACSAR2), readl(REG_DMACBCR), readl(REG_DMACISR));
			//writel(readl(REG_SDCSR) | DI_EN, REG_SDCSR);

			while(!(readl(REG_SDISR) & BLKD_IF));
			writel(BLKD_IF, REG_SDISR);

		} else if (data->flags & MMC_DATA_WRITE) {
			// printf("**** %x %x %x %x\n", readl(REG_DMACCSR), readl(REG_DMACSAR2), readl(REG_DMACBCR), readl(REG_DMACISR));
			while(!(readl(REG_SDISR) & BLKD_IF));
			writel(BLKD_IF, REG_SDISR);

			writel(readl(REG_SDCSR) | CLK_KEEP0, REG_SDCSR);

			while (!(readl(REG_SDISR) & SDDAT0));
			writel(readl(REG_SDCSR) & ~CLK_KEEP0, REG_SDCSR);
		}
	}

	return(0);
}

static int nuc980_emmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	unsigned char *c;
	int i, j, tmp[5];
	unsigned int block_length, blocks;
	unsigned int emmcctl = readl(REG_EMMCCTL) & DBW;      // keep bus width. config other fields in this function

	/*
	printf("[%s]REG_FMICTL = 0x%x\n",__FUNCTION__,readl(REG_FMICTL));
	       printf("[%s]REG_EMMCCTL = 0x%x\n",__FUNCTION__,readl(REG_EMMCCTL));
	       printf("[%s]REG_EMMCINTEN = 0x%x\n",__FUNCTION__,readl(REG_EMMCINTEN));
	       printf("[%s]REG_EMMCINTSTS = 0x%x\n",__FUNCTION__,readl(REG_EMMCINTSTS));
	       printf("[%s]cmd->cmdidx = 0x%x\n",__FUNCTION__,cmd->cmdidx);
	       printf("[%s]REG_EMMCRESP0 = 0x%x\n",__FUNCTION__,readl(REG_EMMCRESP0));
	       printf("[%s]REG_EMMCRESP1 = 0x%x\n",__FUNCTION__,readl(REG_EMMCRESP1));
	printf("========================================================\n");
	*/

	if((readl(REG_EMMCCTL) & 0xF) != 0)
		if(nuc980_emmc_check_ready_busy() < 0)
			return(-ETIMEDOUT);


	if(mmc_resp_type(cmd) != MMC_RSP_NONE) {
		if(mmc_resp_type(cmd) == MMC_RSP_R2) {
			emmcctl |= R2_EN;
		} else {
			emmcctl |= RI_EN;
		}

		writel(RITO_IF, REG_EMMCINTSTS);
		writel(0xFFFF, REG_EMMCTOUT);
	}

	emmcctl |= 0x09010000;                    // Set SDNWR and BLK_CNT to 1, update later
	emmcctl |= (cmd->cmdidx << 8) | CO_EN;

	if (data) {
		block_length = data->blocksize;
		blocks = data->blocks;

		//printf("size %d len %d\n", block_length, blocks);
		writel(block_length - 1, REG_EMMCBLEN);

		if (block_length <= 0x200) {
			if (blocks < 256)
				emmcctl = (emmcctl & ~0xFF0000) | (blocks << 16);
			else
				printf("NUC980 eMMC Max block transfer is 255!!\n");
		}
		//printf("emmcctl 0x%x \n", emmcctl);

		if (data->flags == MMC_DATA_READ) {
			emmcctl |= DI_EN;
			writel((unsigned int)data->dest, REG_FMIDMASA);

		} else if (data->flags == MMC_DATA_WRITE) {
			emmcctl |= DO_EN;
			writel((unsigned int)data->src, REG_FMIDMASA);

		}

	} else {
		block_length = 0;
		blocks = 0;
	}


	writel(cmd->cmdarg, REG_EMMCCMD);
	//printf("arg: %x\n", cmd->cmdarg);
	writel(emmcctl, REG_EMMCCTL);
	udelay(300);
	if(cmd->cmdarg==55)
		mdelay(1);

	while (readl(REG_EMMCCTL) & CO_EN); //wait 'til command out complete

	if(mmc_resp_type(cmd) != MMC_RSP_NONE) {
		if(mmc_resp_type(cmd) == MMC_RSP_R2) {
			while (readl(REG_EMMCCTL) & R2_EN);
			c = (unsigned char *)FMI_BA;
			for (i = 0, j = 0; j < 5; i += 4, j++)
				tmp[j] = (*(c + i) << 24) |
				         (*(c + i + 1) << 16) |
				         (*(c + i + 2) << 8) |
				         (*(c + i + 3));
			for (i = 0; i < 4; i++)
				cmd->response[i] = ((tmp[i] & 0x00ffffff) << 8) |
				                   ((tmp[i + 1] & 0xff000000) >> 24);
		} else {
			while (1) {
				if (readl(REG_EMMCINTSTS) & RITO_IF) {
					writel(RITO_IF, REG_EMMCINTSTS);
					writel(0, REG_EMMCTOUT);
					return(-ETIMEDOUT);
				}
				if (!(readl(REG_EMMCCTL) & RI_EN))
					break;
			}
			//printf("=>%x %x %x %x\n", emmcctl, readl(REG_EMMCBLEN), readl(REG_EMMCRESP0), readl(REG_EMMCRESP1));
			//printf("[xxxx]REG_EMMCINTSTS = 0x%x\n",readl(REG_EMMCINTSTS));
			cmd->response[0] = (readl(REG_EMMCRESP0) << 8) | (readl(REG_EMMCRESP1) & 0xff);
			cmd->response[1] = cmd->response[2] = cmd->response[3] = 0;
		}
	}
	if ((readl(REG_EMMCINTSTS) & CRC_7) == 0) {
		writel(CRC_IF, REG_EMMCINTSTS);
		if (mmc_resp_type(cmd) & MMC_RSP_CRC) {
			printf("xxxx retrun err!\n");
			return(-EILSEQ);
		}

	}


	if (data) {
		if (data->flags & MMC_DATA_READ) {
			//printf("R\n");
			//printf("**** %x %x %x %x\n", readl(REG_DMACCSR), readl(REG_DMACSAR2), readl(REG_DMACBCR), readl(REG_DMACISR));
			//writel(readl(REG_EMMCCTL) | DI_EN, REG_EMMCCTL);


			while(!(readl(REG_EMMCINTSTS) & BLKD_IF));

			writel(BLKD_IF, REG_EMMCINTSTS);
			//while(1);

		} else if (data->flags & MMC_DATA_WRITE) {
			while(!(readl(REG_EMMCINTSTS) & BLKD_IF));
			writel(BLKD_IF, REG_EMMCINTSTS);

			writel(readl(REG_EMMCCTL) | CLK_KEEP0, REG_EMMCCTL);

			while (!(readl(REG_EMMCINTSTS) & SDDAT0));
			writel(readl(REG_EMMCCTL) & ~CLK_KEEP0, REG_EMMCCTL);

		}
	}

	return(0);

}

static int nuc980_sdhci_send_command(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct nuc980_sdhci_plat *plat = dev_get_plat(dev);

	return (!plat->isEMMC) ? nuc980_sd_send_cmd(&plat->mmc, cmd, data) : nuc980_emmc_send_cmd(&plat->mmc, cmd, data);
}

static void nuc980_sd_set_clock(struct mmc *mmc, uint clock)
{
	uint    rate;
	unsigned long clk;

	if (clock < (*(mmc->cfg)).f_min)
		clock = (*(mmc->cfg)).f_min;
	if (clock > (*(mmc->cfg)).f_max)
		clock = (*(mmc->cfg)).f_max;

	//printf("clock = %d\n",clock);
	if (clock >= 2000000) {
		writel((readl(REG_CLKDIVCTL9) & ~0x18) | 0x18, REG_CLKDIVCTL9); //Set SDH clock source from UCLKOut
		clk = 300000000;
	} else {
		writel((readl(REG_CLKDIVCTL9) & ~0x18), REG_CLKDIVCTL9); //Set SDH clock source from HXT
		clk = 12000000;
	}

	rate = clk / clock;
	//printf("MMC_CLK = %d, clock=%d,rate=0x%x\n",MMC_CLK,clock,rate);

	// choose slower clock if system clock cannot divisible by wanted clock
	if (clk % clock != 0)
		rate++;

	writel((readl(REG_CLKDIVCTL9) & ~0xFF00) | ((rate-1) << 8), REG_CLKDIVCTL9); //Set SDH clock divider(SDH_N)

	return;
}

static void nuc980_emmc_set_clock(struct mmc *mmc, uint clock)
{
	uint    rate;
	unsigned long clk;

	if (clock < (*(mmc->cfg)).f_min)
		clock = (*(mmc->cfg)).f_min;
	if (clock > (*(mmc->cfg)).f_max)
		clock = (*(mmc->cfg)).f_max;

	//printf("clock = %d\n",clock);
	if (clock >= 2000000) {
		writel((readl(REG_CLKDIVCTL3) & ~0x18) | 0x18, REG_CLKDIVCTL3); //Set eMMC clock source from UCLKOut
		clk = 300000000;
	} else {
		writel((readl(REG_CLKDIVCTL3) & ~0x18), REG_CLKDIVCTL3); //Set eMMC clock source from HXT
		clk = 12000000;
	}
	rate = clk / clock;
	//printf("clk = %d, clock=%d,rate=0x%x\n",clk,clock,rate);

	// choose slower clock if system clock cannot divisible by wanted clock
	if (clk % clock != 0)
		rate++;

	writel((readl(REG_CLKDIVCTL3) & ~0xFF00) | ((rate-1) << 8), REG_CLKDIVCTL3); //Set eMMC clock divider(eMMC_N)

	return;
}

static int nuc980_sdhci_set_ios(struct udevice *dev)
{
	struct nuc980_sdhci_plat *plat = dev_get_plat(dev);

	if (!plat->isEMMC) // SD port 0
	{
		if(plat->mmc.clock)
			nuc980_sd_set_clock(&plat->mmc, plat->mmc.clock);

		if (plat->mmc.bus_width == 4)
			writel(readl(REG_SDCSR) | DBW, REG_SDCSR);
		else
			writel(readl(REG_SDCSR) & ~DBW, REG_SDCSR);
	}
	else // eMMC
	{
		if(plat->mmc.clock)
			nuc980_emmc_set_clock(&plat->mmc, plat->mmc.clock);

		if (plat->mmc.bus_width == 4)
			writel(readl(REG_EMMCCTL) | DBW, REG_EMMCCTL);
		else
			writel(readl(REG_EMMCCTL) & ~DBW, REG_EMMCCTL);
	}

	return 0;
}

static int nuc980_sdhci_get_cd(struct udevice *dev)
{
	struct nuc980_sdhci_plat *plat = dev_get_plat(dev);
	u8 cd;

	if (!plat->isEMMC) { // SD port 0
		if (((readl(REG_SDIER) & 0x40000000) >> 30) == 0)  // for DAT3 mode
			cd = ((readl(REG_SDISR) & (1 << 16)) == (1 << 16)) ? 1 : 0;
		else
			cd = ((readl(REG_SDISR) & (1 << 16)) == (1 << 16)) ? 0 : 1;
	} 
	else { // eMMC
		if (((readl(REG_EMMCINTEN) & 0x40000000) >> 30) == 0)  // for DAT3 mode
			cd = ((readl(REG_EMMCINTSTS) & (1 << 16)) == (1 << 16)) ? 1 : 0;
		else
			cd = ((readl(REG_EMMCINTSTS) & (1 << 16)) == (1 << 16)) ? 0 : 1;
	}

	return cd;
}

static int nuc980_sd_init(void)
{
	int volatile i;

	writel(GCTL_RST, REG_SDH_GCTL);
	writel(DMAC_SW_RST, REG_DMACCSR);
	for(i = 0; i < 100; i++);        // Need few clock delay 'til SW_RST auto cleared.
	writel(SD_EN, REG_SDH_GCTL);
	writel(DMACEN, REG_DMACCSR);
	writel(readl(REG_SDIER) | 0x40000000, REG_SDIER); // Use GPIO for card detection

	return(0);
}

static int nuc980_emmc_init(void)
{
	int volatile i;

	writel(readl(REG_FMICTL) | GCTL_RST, REG_FMICTL);
	writel(DMAC_SW_RST, REG_FMIDMACTL);
	for(i = 0; i < 100; i++);        // Need few clock delay 'til SW_RST auto cleared.
	writel(EMMC_EN, REG_FMICTL);
	writel(DMACEN, REG_FMIDMACTL);

	return(0);
}

static int muc980_sdhci_bind(struct udevice *dev)
{
	struct nuc980_sdhci_plat *plat = dev_get_plat(dev);

	return mmc_bind(dev, &plat->mmc, &plat->cfg);
}

static int nuc980_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct nuc980_sdhci_plat *plat = dev_get_plat(dev);

	ofnode node = dev_ofnode(dev);
	u32 priv = 0;
	ofnode_read_u32(node, "mode", &priv);
	plat->isEMMC = (priv != 0) ? true : false;

	__raw_writel(__raw_readl(REG_HCLKEN) | 0x40000000, REG_HCLKEN);  // SDHCI clk on
	plat->mmc.cfg = &plat->cfg;
	plat->cfg.host_caps = MMC_MODE_4BIT | MMC_MODE_HS;
	plat->cfg.voltages =  MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_30_31 | MMC_VDD_29_30 | MMC_VDD_28_29 | MMC_VDD_27_28;
	plat->cfg.b_max = 255;
	upriv->mmc = &plat->mmc;

	if (!plat->isEMMC) { // SD
		writel((readl(REG_CLKDIVCTL9) & ~0x18), REG_CLKDIVCTL9); //Set SDH clock source from XIN
		writel((readl(REG_CLKDIVCTL9) & ~0xFF00) | (0x3b << 8), REG_CLKDIVCTL9); //Set SDH clock divider => 200 KHz

		writel(readl(REG_SDCSR) | CLK74_OE, REG_SDCSR);
		while(readl(REG_SDCSR) & CLK74_OE);

		plat->cfg.name = "NUC980 SD";
		plat->cfg.f_min = 200000;
		plat->cfg.f_max = 20000000;

		nuc980_sd_init();
	} 
	else { //eMMC
		writel((readl(REG_CLKDIVCTL9) & ~0x18), REG_CLKDIVCTL9); //Set SDH clock source from XIN
		writel((readl(REG_CLKDIVCTL9) & ~0xFF00) | (0x3b << 8), REG_CLKDIVCTL9); //Set SDH clock divider => 200 KHz

		writel(readl(REG_SDCSR) | CLK74_OE, REG_SDCSR);
		while(readl(REG_SDCSR) & CLK74_OE);

		plat->cfg.name = "NUC980 eMMC";
		plat->cfg.f_min = 200000;
		plat->cfg.f_max = 2000000;

		nuc980_emmc_init();
	}

	printf("NUC980 SDHCI Configured\n");
	return 0;
}

const struct dm_mmc_ops nuc980_sdhci_ops = {
	.send_cmd	= nuc980_sdhci_send_command,
	.set_ios	= nuc980_sdhci_set_ios,
	.get_cd		= nuc980_sdhci_get_cd,
};

static const struct udevice_id nuc980_sdhci_ids[] = {
	{ .compatible = "nuvoton,nuc980-sdh" },
	{ }
};

U_BOOT_DRIVER(nuc980_sdhci_drv) = {
	.name           = "nuc980_sdhci",
	.id             = UCLASS_MMC,
	.of_match       = nuc980_sdhci_ids,
	.ops            = &nuc980_sdhci_ops,
	.probe          = nuc980_sdhci_probe,
	.bind			= muc980_sdhci_bind,
	.plat_auto      = sizeof(struct nuc980_sdhci_plat),
};
