#include <common.h>
#include <dm.h>
#include <spi.h>
#include <clk.h>
#include <asm/gpio.h>
#include <linux/iopoll.h>

DECLARE_GLOBAL_DATA_PTR;

// Register defines
#define REG_HCLKEN      0xB0000210
#define REG_PCLKEN1 0xB000021C

#define PDMA_BA		0xB0008000
#define REG_PDMA_DSCT0_CTL      (PDMA_BA+0x000)
#define REG_PDMA_DSCT0_SA       (PDMA_BA+0x004)
#define REG_PDMA_DSCT0_DA       (PDMA_BA+0x008)
#define REG_PDMA_CHCTL          (PDMA_BA+0x400)
#define REG_PDMA_TDSTS          (PDMA_BA+0x424)
#define REG_PDMA_REQSEL0_3      (PDMA_BA+0x480)

#define SPI_GENERAL_TIMEOUT_MS  1000
#define PCLK_CLK         150000000        // From APB clock

/* spi register bit */
#define ENINT           (0x01 << 17)
#define TXNEG           (0x01 << 2)
#define RXNEG           (0x01 << 1)
#define LSB             (0x01 << 13)
#define SELECTLEV       (0x01 << 2)
#define SELECTPOL       (0x01 << 3)
#define SELECTSLAVE0    0x01
#define SELECTSLAVE1    0x02
#define SPIEN           0x01
#define SPIENSTS        (0x01 << 15)
#define TXRXRST         (0x01 << 23)
#define SPI_BUSY        0x00000001
#define SPI_SS_ACT      0x00000001
#define SPI_SS_HIGH     0x00000004
#define SSCR_AUTOSS_Msk (1 << 3)
#define SSCR_SS_LVL_Msk (1 << 2)
#define SPI_QUAD_EN     0x400000
#define SPI_DIR_2QM     0x100000

typedef struct {
    volatile unsigned int CTL;
    volatile unsigned int CLKDIV;
    volatile unsigned int SSCTL;
    volatile unsigned int PDMACTL;
    volatile unsigned int FIFOCTL;
    volatile unsigned int STATUS;
    volatile unsigned int TX;
    volatile unsigned int RX;
} QSPI_TypeDef;      

struct nuc980_qspi_priv {
    QSPI_TypeDef *reg;
    
    unsigned int num_cs;
	unsigned int lsb;
	unsigned int txneg;
	unsigned int rxneg;
	unsigned int divider;
	unsigned int sleep;
	unsigned int txbitlen;
	unsigned int clkpol;
	int bus_num;
	unsigned int hz;
};

/**
 * @brief Setup QSPI TX edge polarity
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_tx_edge(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->txneg)
        val |= TXNEG;
    else
        val &= ~TXNEG;
    
    priv->reg->CTL = val;
}

/**
 * @brief Setup QSPI RX edge polarity
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_rx_edge(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->rxneg)
        val |= RXNEG;
    else
        val &= ~RXNEG;
    
    priv->reg->CTL = val;
}

/**
 * @brief Select whether to send LSB or MSB first
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_send_first(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->lsb)
        val |= LSB;
    else
        val &= ~LSB;
    
    priv->reg->CTL = val;
}

/**
 * @brief Set the suspend inteval between successive transmits
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_set_sleep(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->sleep)
        val |= (priv->sleep << 4);
    
    priv->reg->CTL = val;
}

/**
 * @brief Set the QSPI TX bit length
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_setup_txbitlen(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->txbitlen)
        val |= (priv->txbitlen << 8);
    
    priv->reg->CTL = val;
}

/**
 * @brief Set QSPI Clock Polarity
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_set_clock_polarity(struct nuc980_qspi_priv *priv)
{
    u32 val = priv->reg->CTL;

    if(priv->clkpol)
        val |= SELECTPOL;
    else
        val &= ~SELECTPOL;
    
    priv->reg->CTL = val;
}

/**
 * @brief Set input clock divider for QSPI output clock
 * 
 * @param priv Pointer to private data struct
 */
static void nuc980_set_divider(struct nuc980_qspi_priv *priv)
{
    priv->reg->CLKDIV = priv->divider;
}

/**
 * @brief Activate QSPI Chip Select
 * @note TODO: Add support for CS1
 * 
 * @param priv Pointer to private data struct
 */
static void spi_cs_activate(struct nuc980_qspi_priv *priv)
{  
    priv->reg->SSCTL = (priv->reg->SSCTL | SELECTSLAVE0);
}

/**
 * @brief Deactivate QSPI Chip Select
 * @note TODO: Add support for CS1
 * 
 * @param priv Pointer to private data struct
 */
static void spi_cs_deactivate(struct nuc980_qspi_priv *priv)
{  
    priv->reg->SSCTL = (priv->reg->SSCTL & ~SELECTSLAVE0);
}

/**
 * @brief Claim the QSPI bus
 * @note Since this driver is seperate from normal SPI, do nothing TODO change?
 * 
 * @param dev Pointer to device struct
 * @return int 
 */
static int nuc980_qspi_claim_bus(struct udevice *dev)
{
    return 0;
}

/**
 * @brief Release the QSPI bus
 * @note Since this driver is seperate from normal SPI, do nothing TODO change?
 * 
 * @param dev Pointer to device struct
 * @return int 
 */
static int nuc980_qspi_release_bus(struct udevice *dev)
{
    return 0;
}

/**
 * @brief Set the QSPI TX wordlen
 * 
 * @param dev Pointer to device struct
 * @param wordlen 
 * @return int 
 */
static int nuc980_qspi_set_wordlen(struct udevice *dev, unsigned int wordlen)
{
    struct nuc980_qspi_priv *priv = dev_get_priv(dev);

    priv->txbitlen = wordlen;
    nuc980_setup_txbitlen(priv);

    return 0;
}

static int nuc980_qspi_xfer(struct udevice *dev, unsigned int bitlen, const void *dout, void *din, unsigned long flags)
{
    printf("Transferring w/ qspi: %08lX\n", flags);
    struct nuc980_qspi_priv *priv = dev_get_priv(dev);
    unsigned int len;
	unsigned int i;
	unsigned char *tx = (unsigned char *)dout;
	unsigned char *rx = din;

    if(bitlen == 0)
		goto out;

    if(bitlen % 8) {
		/* Errors always terminate an ongoing transfer */
		flags |= SPI_XFER_END;
		goto out;
	}
    len = bitlen / 8;

    if(flags & SPI_XFER_BEGIN) {
		spi_cs_activate(priv);
	}

    
    for (i = 0; i < len; i++) {
        if(tx) {
            while ((priv->reg->STATUS & 0x20000)); //TXFULL
            priv->reg->TX = *tx++;
        }

        if(rx) {
            while ((priv->reg->STATUS & 0x20000)); //TXFULL
            priv->reg->TX = 0;
            while ((priv->reg->STATUS & 0x100)); //RXEMPTY
            *rx++ = (unsigned char)priv->reg->RX;
        }
    }

    while (priv->reg->STATUS & SPI_BUSY);


out:
    if (flags & SPI_XFER_END) {
		/*
		 * Wait until the transfer is completely done before
		 * we deactivate CS.
		 */
		while (priv->reg->STATUS & SPI_BUSY);

		spi_cs_deactivate(priv);
	}

    return 0;
}

/**
 * @brief Set the QSPI Clock Speed
 * 
 * @param dev Pointer to device struct
 * @param hz Speed to update to
 * @return int 
 */
static int nuc980_qspi_set_speed(struct udevice *bus, uint hz)
{
    struct nuc980_qspi_priv *priv = dev_get_priv(bus);
    unsigned int div = PCLK_CLK / hz;

	if (div)
		div--;

	if(div == 0)
		div = 1;

	if(div > 0x1FF)
		div = 0x1FF;

	priv->reg->CLKDIV = div;

	return 0;
}

/**
 * @brief Setup the QSPI mode based on passed flags
 * @todo Configure pins based on passed in flags?
 * 
 * @param bus Pointer to device struct
 * @param mode Flags for device configuration
 * @return int 
 */
static int nuc980_qspi_set_mode(struct udevice *bus, uint mode)
{
    printf("Setting up mode: %08X\n", mode);
    struct nuc980_qspi_priv *priv = dev_get_priv(bus);

    // Clock polarity and clock phase sampling setup
    priv->clkpol = (mode & SPI_CPOL) >> 1;
    if(mode & SPI_CPHA)
    {
        priv->rxneg = 1;
        priv->txneg = 1;
    }
    else
    {
        priv->rxneg = 0;
        priv->txneg = 0;
    }
    nuc980_set_clock_polarity(priv);
    nuc980_rx_edge(priv);
    nuc980_tx_edge(priv);

    if(mode & SPI_LSB_FIRST)
        priv->lsb = 1;
    else
        priv->lsb = 0;
    nuc980_send_first(priv);

    if (mode & SPI_3WIRE) 
		priv->reg->CTL = (priv->reg->CTL | SPI_QUAD_EN); // Enable Quad mode
	else
		priv->reg->CTL = (priv->reg->CTL & ~SPI_QUAD_EN); // Disable Quad mode

    return 0;
}

static int nuc980_qspi_probe(struct udevice *dev) 
{
    ofnode node = dev_ofnode(dev);
    struct nuc980_qspi_priv *priv = dev_get_priv(dev);
    printf("Probing NUC980 QSPI Driver...\n");

    priv->reg = (QSPI_TypeDef *) ofnode_get_addr(node);
    printf("Reg location: %p\n", priv->reg);

    // Setup other qspi parameters
    ofnode_read_u32(node, "num_cs", &priv->num_cs);
    ofnode_read_u32(node, "lsb", &priv->lsb);
    ofnode_read_u32(node, "txneg", &priv->txneg);
    ofnode_read_u32(node, "rxneg", &priv->rxneg);
    ofnode_read_u32(node, "clkpol", &priv->clkpol);
    ofnode_read_u32(node, "divider", &priv->divider);
    ofnode_read_u32(node, "sleep", &priv->sleep);
    ofnode_read_u32(node, "txbitlen", &priv->txbitlen);
    ofnode_read_u32(node, "bus_num", &priv->bus_num);
    writel(readl(REG_PCLKEN1) | 0x10, REG_PCLKEN1); // QSPI0 clk enable

    // Configure QSPI registers
    nuc980_tx_edge(priv);
    nuc980_rx_edge(priv);
    nuc980_send_first(priv);
    nuc980_set_sleep(priv);
    nuc980_setup_txbitlen(priv);
    nuc980_set_clock_polarity(priv);
    nuc980_set_divider(priv);
    priv->reg->CTL = (priv->reg->CTL | SPIEN); /* enable SPI */

    return 0;
}

static const struct dm_spi_ops nuc980_qspi_ops = {
    .claim_bus          = nuc980_qspi_claim_bus,
    .release_bus        = nuc980_qspi_release_bus,
    .set_wordlen        = nuc980_qspi_set_wordlen,
	.xfer               = nuc980_qspi_xfer,
	.set_speed          = nuc980_qspi_set_speed,
	.set_mode           = nuc980_qspi_set_mode,
};

static const struct udevice_id nuc980_qspi_ids[] = {
	{ .compatible = "nuvoton,nuc980-qspi0"},
	{ }
};

U_BOOT_DRIVER(npcm_pspi) = {
	.name   = "nuc980_qspi",
	.id     = UCLASS_SPI,
	.of_match = nuc980_qspi_ids,
	.ops    = &nuc980_qspi_ops,
	.priv_auto = sizeof(struct nuc980_qspi_priv),
	.probe  = nuc980_qspi_probe,
};
