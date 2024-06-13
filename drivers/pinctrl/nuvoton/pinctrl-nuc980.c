#include <dm.h>
#include <errno.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dm/pinctrl.h>

#define GCR_BA		0xB0000000 /* Global Control */

#define REG_MFP_GPA_L	(GCR_BA+0x070)	/* GPIOA Low Byte Multiple Function Control Register */
#define REG_MFP_GPA_H	(GCR_BA+0x074)	/* GPIOA High Byte Multiple Function Control Register */
#define REG_MFP_GPB_L	(GCR_BA+0x078)	/* GPIOB Low Byte Multiple Function Control Register */
#define REG_MFP_GPB_H	(GCR_BA+0x07C)	/* GPIOB High Byte Multiple Function Control Register */
#define REG_MFP_GPC_L	(GCR_BA+0x080)	/* GPIOC Low Byte Multiple Function Control Register */
#define REG_MFP_GPC_H	(GCR_BA+0x084)	/* GPIOC High Byte Multiple Function Control Register */
#define REG_MFP_GPD_L	(GCR_BA+0x088)	/* GPIOD Low Byte Multiple Function Control Register */
#define REG_MFP_GPD_H	(GCR_BA+0x08C)	/* GPIOD High Byte Multiple Function Control Register */
#define REG_MFP_GPE_L	(GCR_BA+0x090)	/* GPIOE Low Byte Multiple Function Control Register */
#define REG_MFP_GPE_H	(GCR_BA+0x094)	/* GPIOE High Byte Multiple Function Control Register */
#define REG_MFP_GPF_L	(GCR_BA+0x098)	/* GPIOF Low Byte Multiple Function Control Register */
#define REG_MFP_GPF_H	(GCR_BA+0x09C)	/* GPIOF High Byte Multiple Function Control Register */
#define REG_MFP_GPG_L	(GCR_BA+0x0A0)	/* GPIOG Low Byte Multiple Function Control Register */
#define REG_MFP_GPG_H	(GCR_BA+0x0A4)	/* GPIOG High Byte Multiple Function Control Register */
#define REG_MFP_GPH_L	(GCR_BA+0x0A8)	/* GPIOH Low Byte Multiple Function Control Register */
#define REG_MFP_GPH_H	(GCR_BA+0x0AC)	/* GPIOH High Byte Multiple Function Control Register */
#define REG_MFP_GPI_L	(GCR_BA+0x0B0)	/* GPIOI Low Byte Multiple Function Control Register */
#define REG_MFP_GPI_H	(GCR_BA+0x0B4)	/* GPIOI High Byte Multiple Function Control Register */
#define REG_MFP_GPJ_L	(GCR_BA+0x0B8)	/* GPIOJ Low Byte Multiple Function Control Register */
#define REG_DDR_DS_CR	(GCR_BA+0x0E0)	/* DDR I/O Driving Strength Control Register */
#define REG_PORDISCR    (GCR_BA+0x100)	/* Power-On-Reset Disable Control Register */
#define REG_ICEDBGCR    (GCR_BA+0x104)	/* ICE Debug Interface Control Register */
#define REG_WRPRTR	(GCR_BA+0x1FC)	/* Register Write-Protection Control Register */

#define REG_HCLKEN	0xB0000210

/**
 * struct nuc980_pmx_pin - describes an NUC980 pin multi-function
 * @bank: the bank of the pin (0 for PA, 1 for PB...)
 * @pin: pin number (0 ~ 0xf)
 * @func: multi-function pin setting value
 * @conf: reserved for GPIO mode
 */
struct nuc980_pmx_pin {
	uint32_t	bank;
	uint32_t	pin;
	uint32_t	func;
	unsigned long	conf;
};

/**
 * struct nuc980_pmx_func - describes NUC980 pinmux functions
 * @name: the name of this specific function
 * @groups: corresponding pin groups
 * @ngroups: the number of groups
 */
struct nuc980_pmx_func {
	const char	*name;
	const char	**groups;
	unsigned	ngroups;
};

/**
 * struct nuc980_pin_group - describes an NUC980 pin group
 * @name: the name of this specific pin group
 * @pins_conf: the mux mode for each pin in this group. The size of this
 *  array is the same as pins.
 * @pins: an array of discrete physical pins used in this group, taken
 *  from the driver-local pin enumeration space
 * @npins: the number of pins in this group array, i.e. the number of
 *  elements in .pins so we can iterate over that array
 */
struct nuc980_pin_group {
	const char	*name;
	struct nuc980_pmx_pin	*pins_conf;
	unsigned int	*pins;
	unsigned	npins;
};

struct nuc980_pinctrl_priv {
	int		nbanks;

	struct nuc980_pmx_func *functions;
	int		nfunctions;

	struct nuc980_pin_group *groups;
	int		ngroups;
};

#define MAX_NB_GPIO_PER_BANK 16

#define NUC980_PIN(a, b) { .number = a, .name = b }
struct nuc980_pin_desc {
	unsigned int number;
	const char *name;
};

// The numbering is not related to actual layout.
const struct nuc980_pin_desc nuc980_pins[] = {
	NUC980_PIN(0x00, "PA0"),
	NUC980_PIN(0x01, "PA1"),
	NUC980_PIN(0x02, "PA2"),
	NUC980_PIN(0x03, "PA3"),
	NUC980_PIN(0x04, "PA4"),
	NUC980_PIN(0x05, "PA5"),
	NUC980_PIN(0x06, "PA6"),
	NUC980_PIN(0x07, "PA7"),
	NUC980_PIN(0x08, "PA8"),
	NUC980_PIN(0x09, "PA9"),
	NUC980_PIN(0x0A, "PA10"),
	NUC980_PIN(0x0B, "PA11"),
	NUC980_PIN(0x0C, "PA12"),
	NUC980_PIN(0x0D, "PA13"),
	NUC980_PIN(0x0E, "PA14"),
	NUC980_PIN(0x0F, "PA15"),
	NUC980_PIN(0x10, "PB0"),
	NUC980_PIN(0x11, "PB1"),
	NUC980_PIN(0x12, "PB2"),
	NUC980_PIN(0x13, "PB3"),
	NUC980_PIN(0x14, "PB4"),
	NUC980_PIN(0x15, "PB5"),
	NUC980_PIN(0x16, "PB6"),
	NUC980_PIN(0x17, "PB7"),
	NUC980_PIN(0x18, "PB8"),
	NUC980_PIN(0x19, "PB9"),
	NUC980_PIN(0x1A, "PB10"),
	NUC980_PIN(0x1B, "PB11"),
	NUC980_PIN(0x1C, "PB12"),
	NUC980_PIN(0x1D, "PB13"),
	NUC980_PIN(0x1E, "PB14"),
	NUC980_PIN(0x1F, "PB15"),
	NUC980_PIN(0x20, "PC0"),
	NUC980_PIN(0x21, "PC1"),
	NUC980_PIN(0x22, "PC2"),
	NUC980_PIN(0x23, "PC3"),
	NUC980_PIN(0x24, "PC4"),
	NUC980_PIN(0x25, "PC5"),
	NUC980_PIN(0x26, "PC6"),
	NUC980_PIN(0x27, "PC7"),
	NUC980_PIN(0x28, "PC8"),
	NUC980_PIN(0x29, "PC9"),
	NUC980_PIN(0x2A, "PC10"),
	NUC980_PIN(0x2B, "PC11"),
	NUC980_PIN(0x2C, "PC12"),
	NUC980_PIN(0x2D, "PC13"),
	NUC980_PIN(0x2E, "PC14"),
	NUC980_PIN(0x2F, "PC15"),
	NUC980_PIN(0x30, "PD0"),
	NUC980_PIN(0x31, "PD1"),
	NUC980_PIN(0x32, "PD2"),
	NUC980_PIN(0x33, "PD3"),
	NUC980_PIN(0x34, "PD4"),
	NUC980_PIN(0x35, "PD5"),
	NUC980_PIN(0x36, "PD6"),
	NUC980_PIN(0x37, "PD7"),
	NUC980_PIN(0x38, "PD8"),
	NUC980_PIN(0x39, "PD9"),
	NUC980_PIN(0x3A, "PD10"),
	NUC980_PIN(0x3B, "PD11"),
	NUC980_PIN(0x3C, "PD12"),
	NUC980_PIN(0x3D, "PD13"),
	NUC980_PIN(0x3E, "PD14"),
	NUC980_PIN(0x3F, "PD15"),
	NUC980_PIN(0x40, "PE0"),
	NUC980_PIN(0x41, "PE1"),
	NUC980_PIN(0x42, "PE2"),
	NUC980_PIN(0x43, "PE3"),
	NUC980_PIN(0x44, "PE4"),
	NUC980_PIN(0x45, "PE5"),
	NUC980_PIN(0x46, "PE6"),
	NUC980_PIN(0x47, "PE7"),
	NUC980_PIN(0x48, "PE8"),
	NUC980_PIN(0x49, "PE9"),
	NUC980_PIN(0x4A, "PE10"),
	NUC980_PIN(0x4B, "PE11"),
	NUC980_PIN(0x4C, "PE12"),
	NUC980_PIN(0x4D, "PE13"),
	NUC980_PIN(0x4E, "PE14"),
	NUC980_PIN(0x4F, "PE15"),
	NUC980_PIN(0x50, "PF0"),
	NUC980_PIN(0x51, "PF1"),
	NUC980_PIN(0x52, "PF2"),
	NUC980_PIN(0x53, "PF3"),
	NUC980_PIN(0x54, "PF4"),
	NUC980_PIN(0x55, "PF5"),
	NUC980_PIN(0x56, "PF6"),
	NUC980_PIN(0x57, "PF7"),
	NUC980_PIN(0x58, "PF8"),
	NUC980_PIN(0x59, "PF9"),
	NUC980_PIN(0x5A, "PF10"),
	NUC980_PIN(0x5B, "PF11"),
	NUC980_PIN(0x5C, "PF12"),
	NUC980_PIN(0x5D, "PF13"),
	NUC980_PIN(0x5E, "PF14"),
	NUC980_PIN(0x5F, "PF15"),
	NUC980_PIN(0x60, "PG0"),
	NUC980_PIN(0x61, "PG1"),
	NUC980_PIN(0x62, "PG2"),
	NUC980_PIN(0x63, "PG3"),
	NUC980_PIN(0x64, "PG4"),
	NUC980_PIN(0x65, "PG5"),
	NUC980_PIN(0x66, "PG6"),
	NUC980_PIN(0x67, "PG7"),
	NUC980_PIN(0x68, "PG8"),
	NUC980_PIN(0x69, "PG9"),
	NUC980_PIN(0x6A, "PG10"),
	NUC980_PIN(0x6B, "PG11"),
	NUC980_PIN(0x6C, "PG12"),
	NUC980_PIN(0x6D, "PG13"),
	NUC980_PIN(0x6E, "PG14"),
	NUC980_PIN(0x6F, "PG15"),
};

/**
 * @brief Get the number of devices/pin configurations in the device tree
 * 
 * @param priv 
 * @param parent 
 */
static void nuc980_pinctrl_child_count(struct nuc980_pinctrl_priv *priv, ofnode parent)
{
	ofnode child;
	ofnode_for_each_subnode(child, parent) {
		priv->nfunctions++;// Number of device configurations
		priv->ngroups += ofnode_get_child_count(child); // Number of different pin configurations for a device
	}
	debug("Total functions: %d, total groups: %d\n", priv->nfunctions, priv->ngroups);
}

/**
 * @brief Parse pin group functions
 * 
 * @param node 
 * @param grp 
 * @param priv 
 * @param index 
 * @return int 
 */
static int nuc980_pinctrl_parse_groups(ofnode node, struct nuc980_pin_group *grp, struct nuc980_pinctrl_priv *priv, u32 index)
{
	struct nuc980_pmx_pin *pin;
	int size;
	const __be32 *list;
	debug("Group(%d): %s\n", index, ofnode_get_name(node));

	/* Initialise group */
	grp->name = ofnode_get_name(node);

	/*
	 * the binding format is nuvoton,pins = <bank pin pin-function>,
	 * do sanity check and calculate pins number
	*/
	list = ofnode_get_property(node, "nuvoton,pins", &size);
	/* we do not check return since it's safe node passed down */
	size /= sizeof(*list);
	if (!size || size % 4) {
		printf("nuc980_pinctrl: wrong setting!\n");
		return EINVAL;
	}

	grp->npins = size / 4;
	pin = grp->pins_conf = malloc(grp->npins * sizeof(struct nuc980_pmx_pin));
	grp->pins = malloc(grp->npins * sizeof(unsigned int));
	if (!grp->pins_conf || !grp->pins)
		return ENOMEM;

	for (u32 i = 0, j = 0;i < size;i += 4, j++) {
		pin->bank = be32_to_cpu(*list++);
		pin->pin = be32_to_cpu(*list++);
		grp->pins[j] = pin->bank * MAX_NB_GPIO_PER_BANK + pin->pin;
		pin->func = be32_to_cpu(*list++);
		pin->conf = be32_to_cpu(*list++);

		debug("P%c.%d: func=%d\n", pin->bank+'A', pin->pin, pin->func);
		pin++;
	}

	return 0;
}

/**
 * @brief Parse info for each of the different pin configurations
 * 
 * @param node Parent node
 * @param priv Pointer to private device struct
 * @param index Index of current device being parsed
 * @return int 
 */
static int nuc980_pinctrl_parse_functions(ofnode node, struct nuc980_pinctrl_priv *priv, u32 index) 
{
	struct nuc980_pmx_func *func;
	struct nuc980_pin_group *grp;

	func = &priv->functions[index];

	/* Initialize function */
	func->name = ofnode_get_name(node);
	func->ngroups = ofnode_get_child_count(node);
	if(func->ngroups <= 0) {
		printf("nuc980_pinctrl: no groups defined\n");
		return EINVAL;
	}

	func->groups = malloc(func->ngroups * sizeof(char *));
	if(!func->groups) 
		return ENOMEM;

	ofnode child;
	static u32 grp_index = 0;
	u32 i = 0;
	ofnode_for_each_subnode(child, node) {
		func->groups[i] = ofnode_get_name(child);
		grp = &priv->groups[grp_index++];
		u32 ret = nuc980_pinctrl_parse_groups(child, grp, priv, i++);
		if (ret)
			return ret;
	}

	return 0;
}

static int nuc980_get_pins_count(struct udevice *dev) 
{
	return ARRAY_SIZE(nuc980_pins);
}

static const char *nuc980_get_pin_name(struct udevice *dev, unsigned selector) 
{
	return nuc980_pins[selector].name;
}

static int nuc980_get_groups_count(struct udevice *dev)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->ngroups;
}

static const char *nuc980_get_group_name(struct udevice *dev, unsigned selector)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->groups[selector].name;
}

static int nuc980_get_functions_count(struct udevice *dev)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->nfunctions;
}

static const char *nuc980_get_function_name(struct udevice *dev, unsigned selector)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->functions[selector].name;
}

static int pin_check_config(struct nuc980_pinctrl_priv *info, const char *name, int index, const struct nuc980_pmx_pin *pin)
{
	/* check if it's a valid config */
	if (pin->bank >= info->nbanks) {
		debug("%s: pin conf %d bank_id %d >= nbanks %d\n",
		        name, index, pin->bank, info->nbanks);
		return -EINVAL;
	}

	if (pin->pin >= MAX_NB_GPIO_PER_BANK) {
		debug("%s: pin conf %d pin_bank_id %d >= %d\n",
		        name, index, pin->pin, MAX_NB_GPIO_PER_BANK);
		return -EINVAL;
	}

	if (pin->func > 0xf) {
		debug("%s: invalid pin function setting %d!\n", name, pin->func);
		return -EINVAL;
	}
	return 0;
}

/*
 * selector = data.nux.func, which is entry number in nuc980_functions,
 * and group = data.mux.group, which is entry number in nuc980_pmx_func
 * group is not used since some function use different setting between
 * different ports. for example UART....
 */
int nuc980_pmx_set_mux(struct udevice *dev, unsigned selector, unsigned group)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);
	const struct nuc980_pmx_pin *pins_conf = priv->groups[group].pins_conf;
	const struct nuc980_pmx_pin *pin;
	uint32_t npins = priv->groups[group].npins;
	unsigned int i, ret;
	unsigned int reg, offset;

	/* first check that all the pins of the group are valid with a valid
	 * paramter */
	for (i = 0; i < npins; i++) {
		pin = &pins_conf[i];
		ret = pin_check_config(priv, priv->groups[group].name, i, pin);
		if (ret) {
			return ret;
		}
	}

	for (i = 0; i < npins; i++) {
		pin = &pins_conf[i];
		offset = (pin->bank * 8) + ((pin->pin > 7) ? 4 : 0);
		reg = __raw_readl(REG_MFP_GPA_L + offset);

		reg = (reg & ~(0xF << ((pin->pin & 0x7) * 4))) | (pin->func << ((pin->pin & 0x7) * 4));

		__raw_writel(reg, REG_MFP_GPA_L + offset);
	}

	return 0;
}

static int nuc980_set_state(struct udevice *dev, struct udevice *config)
{
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);

	for (u32 i = 0; i < priv->ngroups; i++) {
		if(strcmp(ofnode_get_name(dev_ofnode(config)), priv->groups[i].name) == 0) {
			debug("Configuring for device: %s\n", ofnode_get_name(dev_ofnode(config)));
			return nuc980_pmx_set_mux(dev, 0, i);
		}	
	}

	return 0;
}

static int nuc980_pinctrl_probe(struct udevice *dev)
{
	debug("Probing NUC980 Pinctrl driver....\n");
	struct nuc980_pinctrl_priv *priv = dev_get_priv(dev);
	
	priv->nbanks = 7; /* PA ~ PG */
	nuc980_pinctrl_child_count(priv, dev_ofnode(dev));
	priv->functions = malloc(priv->nfunctions * sizeof(struct nuc980_pmx_func));
	if(!priv->functions)
		return ENOMEM;

	priv->groups = malloc(priv->ngroups * sizeof(struct nuc980_pin_group));
	if(!priv->groups)
		return ENOMEM;

	debug("Function: %p, Groups: %p\n", priv->functions, priv->groups);
	ofnode child;
	u32 i = 0;
	ofnode_for_each_subnode(child, dev_ofnode(dev)) {
		int ret = nuc980_pinctrl_parse_functions(child, priv, i++);
		if (ret) {
			printf("Failed to parse function\n");
			return ret;
		}
	}

	__raw_writel(__raw_readl(REG_HCLKEN) | 0x800, REG_HCLKEN); // GPIO clks on
	debug("Configured NUC980 Pinctrl Driver\n");

	return 0;
}

static struct pinctrl_ops nuc980_pinctrl_ops = {
    .get_pins_count = nuc980_get_pins_count,
	.get_pin_name = nuc980_get_pin_name,
    .get_groups_count = nuc980_get_groups_count,
	.get_group_name = nuc980_get_group_name,
	.get_functions_count = nuc980_get_functions_count,
	.get_function_name = nuc980_get_function_name,
	.set_state	= nuc980_set_state,
	.pinmux_set = nuc980_pmx_set_mux,
	.pinmux_group_set = NULL,
#if CONFIG_IS_ENABLED(PINCONF)
	.pinconf_num_params = ARRAY_SIZE(npcm7xx_conf_params),
	.pinconf_params = npcm7xx_conf_params,
	.pinconf_set = npcm7xx_pinconf_set,
	.pinconf_group_set = npcm7xx_pinconf_set,
#endif
};

static const struct udevice_id nuc980_pinctrl_ids[] = {
	{ .compatible = "nuvoton,nuc980-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_nuc980) = {
	.name = "nuvoton_nuc980_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = nuc980_pinctrl_ids,
	.priv_auto = sizeof(struct nuc980_pinctrl_priv),
	.ops = &nuc980_pinctrl_ops,
	.probe = nuc980_pinctrl_probe,
};
