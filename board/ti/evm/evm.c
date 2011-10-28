/*
 * (C) Copyright 2004-2008
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Manikandan Pillai <mani.pillai@ti.com>
 *
 * Derived from Beagle Board and 3430 SDP code by
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <khasim@ti.com>
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
 */
#include <common.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/arch/mem.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <asm/mach-types.h>
#include <fastboot.h>

#ifdef CONFIG_FLASHBOARD
#include "flashboard.h"
#else
#include "evm.h"
#endif

#ifdef	CONFIG_CMD_FASTBOOT
#ifdef	FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING
/* Initialize the name of fastboot flash name mappings */
fastboot_ptentry ptn[6] = {
	{
		.name   = "xloader",
		.start  = 0x0000000,
		.length = 0x0020000,
		/* Written into the first 4 0x20000 blocks
		   Use HW ECC */
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I |
		          FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_2 |
			  FASTBOOT_PTENTRY_FLAGS_REPEAT_4,
	},
		{
		.name   = "bootloader",
		.start  = 0x0080000,
		.length = 0x01C0000,
		/* Skip bad blocks on write
		   Use HW ECC */
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I |
		          FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_2,
	},
	{
		.name   = "environment",
		.start  = SMNAND_ENV_OFFSET,  /* set in config file */
		.length = 0x0040000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_ENV,
	},
	{
		.name   = "boot",
		/* Test with start close to bad block
		   The is dependent on the individual board.
		   Change to what is required */
		/* .start  = 0x0a00000, */
			/* The real start */
		.start  = 0x0280000,
		.length = 0x0500000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
		          FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_I,
	},
	{
		.name   = "system",
		.start  = 0x00780000,
		.length = 0x1F880000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
		          FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_I,
	},
};
#endif /* FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING */
#endif /* CONFIG_FASTBOOT */

static u8 omap3_evm_version;

u8 get_omap3_evm_rev(void)
{
	return omap3_evm_version;
}

static void omap3_evm_get_revision(void)
{
	unsigned int smsc_id;

	/* Ethernet PHY ID is stored at ID_REV register */
	smsc_id = readl(CONFIG_SMC911X_BASE + 0x50) & 0xFFFF0000;
	printf("Read back SMSC id 0x%x\n", smsc_id);

	switch (smsc_id) {
	/* SMSC9115 chipset */
	case 0x01150000:
		omap3_evm_version = OMAP3EVM_BOARD_GEN_1;
		break;
	/* SMSC 9220 chipset */
	case 0x92200000:
	default:
		omap3_evm_version = OMAP3EVM_BOARD_GEN_2;
       }
}

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init(); /* in SRAM or SDRAM, finish GPMC */
	/* board id for Linux */
	gd->bd->bi_arch_number = MACH_TYPE_OMAP3EVM;
	/* boot param addr */
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100);

#ifdef	CONFIG_CMD_FASTBOOT
#ifdef	FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING
	int i;

	for (i = 0; i < 6; i++)
		fastboot_flash_add_ptn (&ptn[i]);
#endif /* FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING */
#endif /* CONFIG_FASTBOOT */

	return 0;
}

/*
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 */
int misc_init_r(void)
{
#ifdef CONFIG_FLASHBOARD
	/*
	 * Configure drive strength for IO cells
	 */
	*(ulong *)(CONTROL_PROG_IO2) &= ~(PRG_I2C3_PULLUPRESX);
#endif

#ifdef CONFIG_DRIVER_OMAP34XX_I2C
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif

#if defined(CONFIG_CMD_NET)
	setup_net_chip();
#endif

	dieid_num_r();

	return 0;
}

/*
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers specific to the
 *		hardware. Many pins need to be moved from protect to primary
 *		mode.
 */
void set_muxconf_regs(void)
{
	MUX_EVM();
}

/*
 * Routine: setup_net_chip
 * Description: Setting up the configuration GPMC registers specific to the
 *		Ethernet hardware.
 */
static void setup_net_chip(void)
{
	struct gpio *gpio3_base = (struct gpio *)OMAP34XX_GPIO3_BASE;
	struct gpio *gpio1_base = (struct gpio *)OMAP34XX_GPIO1_BASE;
	struct ctrl *ctrl_base = (struct ctrl *)OMAP34XX_CTRL_BASE;

	/* Configure GPMC registers */
#ifdef CONFIG_FLASHBOARD
		writel(NET_GPMC_CONFIG1, &gpmc_cfg->cs[3].config1);
		writel(NET_GPMC_CONFIG2, &gpmc_cfg->cs[3].config2);
		writel(NET_GPMC_CONFIG3, &gpmc_cfg->cs[3].config3);
		writel(NET_GPMC_CONFIG4, &gpmc_cfg->cs[3].config4);
		writel(NET_GPMC_CONFIG5, &gpmc_cfg->cs[3].config5);
		writel(NET_GPMC_CONFIG6, &gpmc_cfg->cs[3].config6);
		writel(NET_GPMC_CONFIG7, &gpmc_cfg->cs[3].config7);
#else
		writel(NET_GPMC_CONFIG1, &gpmc_cfg->cs[5].config1);
		writel(NET_GPMC_CONFIG2, &gpmc_cfg->cs[5].config2);
		writel(NET_GPMC_CONFIG3, &gpmc_cfg->cs[5].config3);
		writel(NET_GPMC_CONFIG4, &gpmc_cfg->cs[5].config4);
		writel(NET_GPMC_CONFIG5, &gpmc_cfg->cs[5].config5);
		writel(NET_GPMC_CONFIG6, &gpmc_cfg->cs[5].config6);
		writel(NET_GPMC_CONFIG7, &gpmc_cfg->cs[5].config7);
#endif

	/* Enable off mode for NWE in PADCONF_GPMC_NWE register */
	writew(readw(&ctrl_base->gpmc_nwe) | 0x0E00, &ctrl_base->gpmc_nwe);
	/* Enable off mode for NOE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_noe) | 0x0E00, &ctrl_base->gpmc_noe);
	/* Enable off mode for ALE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_nadv_ale) | 0x0E00,
		&ctrl_base->gpmc_nadv_ale);

	/* determine omap3evm revision */
	omap3_evm_get_revision();

	if ( get_omap3_evm_rev() == OMAP3EVM_BOARD_GEN_1 ){
		/* Make GPIO 64 as output pin */
		writel(readl(&gpio3_base->oe) & ~(GPIO0), &gpio3_base->oe);

		/* Now send a pulse on the GPIO pin */
		writel(GPIO0, &gpio3_base->setdataout);
		udelay(1);
		writel(GPIO0, &gpio3_base->cleardataout);
		udelay(1);
		writel(GPIO0, &gpio3_base->setdataout);
	}else{
		/* Make GPIO 07 as output pin */
		writel(readl(&gpio1_base->oe) & ~(GPIO7), &gpio1_base->oe);

		/* Now send a pulse on the GPIO pin */
		writel(GPIO7, &gpio1_base->setdataout);
		udelay(1);
		writel(GPIO7, &gpio1_base->cleardataout);
		udelay(1);
		writel(GPIO7, &gpio1_base->setdataout);
	}

}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_SMC911X
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif
	return rc;
}
