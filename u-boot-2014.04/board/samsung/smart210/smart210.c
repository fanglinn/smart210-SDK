/*
 *  Copyright (C) 2008-2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sromc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/nand_reg.h>
#include <asm/arch/clock.h>             /* add by Flinn */
#include <asm/arch/dmc.h>               /* add by Flinn */
#include <netdev.h>

#ifndef CONFIG_SPL_BUILD        /* add by Flinn */

DECLARE_GLOBAL_DATA_PTR;

/*
 * Miscellaneous platform dependent initialisations
 */

#if 0
static void smc9115_pre_init(void)
{
	u32 smc_bw_conf, smc_bc_conf;

	struct s5pc100_gpio *const gpio =
		(struct s5pc100_gpio *)samsung_get_base_gpio();

	/* gpio configuration GPK0CON */
	s5p_gpio_cfg_pin(&gpio->k0, CONFIG_ENV_SROM_BANK, GPIO_FUNC(2));

	/* Ethernet needs bus width of 16 bits */
	smc_bw_conf = SMC_DATA16_WIDTH(CONFIG_ENV_SROM_BANK);
	smc_bc_conf = SMC_BC_TACS(0x0) | SMC_BC_TCOS(0x4) | SMC_BC_TACC(0xe)
			| SMC_BC_TCOH(0x1) | SMC_BC_TAH(0x4)
			| SMC_BC_TACP(0x6) | SMC_BC_PMC(0x0);

	/* Select and configure the SROMC bank */
	s5p_config_sromc(CONFIG_ENV_SROM_BANK, smc_bw_conf, smc_bc_conf);
}
#endif 

/* add by Flinn  */
static void dm9000_pre_init(void)
{
	u32 smc_bw_conf, smc_bc_conf;
	
	/* Ethernet needs bus width of 16 bits */
	smc_bw_conf = SMC_DATA16_WIDTH(CONFIG_ENV_SROM_BANK)
		| SMC_BYTE_ADDR_MODE(CONFIG_ENV_SROM_BANK);
	smc_bc_conf = SMC_BC_TACS(0) | SMC_BC_TCOS(1) | SMC_BC_TACC(2)
		| SMC_BC_TCOH(1) | SMC_BC_TAH(0) | SMC_BC_TACP(0) | SMC_BC_PMC(0);

	/* Select and configure the SROMC bank */
	s5p_config_sromc(CONFIG_ENV_SROM_BANK, smc_bw_conf, smc_bc_conf);
}

int board_init(void)
{
	//smc9115_pre_init();
	dm9000_pre_init();

	gd->bd->bi_arch_number = MACH_TYPE_SMDKV210;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("Board:\tSMART210\n");
	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_SMC911X
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#elif defined(CONFIG_DRIVER_DM9000)
	rc = dm9000_initialize(bis);
#endif
	return rc;
}

#else

void clock_init(void)
{
        u32 val = 0;

        struct s5pv210_clock *const clock = (struct s5pv210_clock *)samsung_get_base_clock();

        writel(0xFFFF, &clock->apll_lock);
        writel(0xFFFF, &clock->mpll_lock);
        writel(0xFFFF, &clock->epll_lock);
        writel(0xFFFF, &clock->vpll_lock);

        writel((3  << 8) | (125 << 16) | (1 << 0) | (1 << 31), &clock->apll_con0);      /* FOUT_APLL = 1000MHz */
        writel((12 << 8) | (667 << 16) | (1 << 0) | (1 << 31), &clock->mpll_con);       /* FOUT_MPLL = 667MHz */
        writel((3  << 8) | (48  << 16) | (2 << 0) | (1 << 31), &clock->epll_con0);      /* FOUT_EPLL = 96MHz */
        writel((6  << 8) | (108 << 16) | (3 << 0) | (1 << 31), &clock->vpll_con);       /* FOUT_VPLL = 54MHz */

        while (!(readl(&clock->apll_con0) & (1 << 29)));
        while (!(readl(&clock->mpll_con) & (1 << 29)));
        while (!(readl(&clock->apll_con0) & (1 << 29)));
        while (!(readl(&clock->epll_con0) & (1 << 29)));
        while (!(readl(&clock->vpll_con) & (1 << 29)));

        writel((1 << 0) | (1 << 4) | (1 << 8) | (1 << 12), &clock->src0);

        val =   (0 << 0)  |     /* APLL_RATIO = 0, freq(ARMCLK) = MOUT_MSYS / (APLL_RATIO + 1) = 1000MHz */
                (4 << 4)  |     /* A2M_RATIO = 4, freq(A2M) = SCLKAPLL / (A2M_RATIO + 1) = 200MHz */
                (4 << 8)  |     /* HCLK_MSYS_RATIO = 4, freq(HCLK_MSYS) = ARMCLK / (HCLK_MSYS_RATIO + 1) = 200MHz */
                (1 << 12) |     /* PCLK_MSYS_RATIO = 1, freq(PCLK_MSYS) = HCLK_MSYS / (PCLK_MSYS_RATIO + 1) = 100MHz */
                (3 << 16) | /* HCLK_DSYS_RATIO = 3, freq(HCLK_DSYS) = MOUT_DSYS / (HCLK_DSYS_RATIO + 1) = 166MHz */
                (1 << 20) | /* PCLK_DSYS_RATIO = 1, freq(PCLK_DSYS) = HCLK_DSYS / (PCLK_DSYS_RATIO + 1) = 83MHz */
                (4 << 24) |     /* HCLK_PSYS_RATIO = 4, freq(HCLK_PSYS) = MOUT_PSYS / (HCLK_PSYS_RATIO + 1) = 133MHz */
                (1 << 28);      /* PCLK_PSYS_RATIO = 1, freq(PCLK_PSYS) = HCLK_PSYS / (PCLK_PSYS_RATIO + 1) = 66MHz */
        writel(val, &clock->div0);
}
void ddr_init(void)
{
        struct s5pv210_dmc0 *const dmc0 = (struct s5pv210_dmc0 *)samsung_get_base_dmc0();
        struct s5pv210_dmc1 *const dmc1 = (struct s5pv210_dmc1 *)samsung_get_base_dmc1();

        /* DMC0 */
        writel(0x00101000, &dmc0->phycontrol0);
        writel(0x00101002, &dmc0->phycontrol0);                 /* DLL on */
        writel(0x00000086, &dmc0->phycontrol1);
        writel(0x00101003, &dmc0->phycontrol0);                 /* DLL start */

        while ((readl(&dmc0->phystatus) & 0x7) != 0x7); /* wait DLL locked */

        writel(0x0FFF2350, &dmc0->concontrol);                  /* Auto Refresh Counter should be off */
        writel(0x00202430, &dmc0->memcontrol);                  /* Dynamic power down should be off */
        writel(0x20E01323, &dmc0->memconfig0);

        writel(0xFF000000, &dmc0->prechconfig);
        writel(0xFFFF00FF, &dmc0->pwrdnconfig);

        writel(0x00000618, &dmc0->timingaref);                  /* 7.8us * 200MHz = 1560 = 0x618  */
        writel(0x19233309, &dmc0->timingrow);
        writel(0x23240204, &dmc0->timingdata);
        writel(0x09C80232, &dmc0->timingpower);

        writel(0x07000000, &dmc0->directcmd);                   /* NOP */
        writel(0x01000000, &dmc0->directcmd);                   /* PALL */
        writel(0x00020000, &dmc0->directcmd);                   /* EMRS2 */
        writel(0x00030000, &dmc0->directcmd);                   /* EMRS3 */
        writel(0x00010400, &dmc0->directcmd);                   /* EMRS enable DLL */
        writel(0x00000542, &dmc0->directcmd);                   /* DLL reset */
        writel(0x01000000, &dmc0->directcmd);                   /* PALL */
        writel(0x05000000, &dmc0->directcmd);                   /* auto refresh */
        writel(0x05000000, &dmc0->directcmd);                   /* auto refresh */
        writel(0x00000442, &dmc0->directcmd);                   /* DLL unreset */
        writel(0x00010780, &dmc0->directcmd);                   /* OCD default */
        writel(0x00010400, &dmc0->directcmd);                   /* OCD exit */

        writel(0x0FF02030, &dmc0->concontrol);                  /* auto refresh on */
        writel(0xFFFF00FF, &dmc0->pwrdnconfig);
        writel(0x00202400, &dmc0->memcontrol);

    	/* DMC1 */
        writel(0x00101000, &dmc1->phycontrol0);
        writel(0x00101002, &dmc1->phycontrol0);                 /* DLL on */
        writel(0x00000086, &dmc1->phycontrol1);
        writel(0x00101003, &dmc1->phycontrol0);                 /* DLL start */

        while ((readl(&dmc1->phystatus) & 0x7) != 0x7); /* wait DLL locked */

        writel(0x0FFF2350, &dmc1->concontrol);                  /* Auto Refresh Counter should be off */
        writel(0x00202430, &dmc1->memcontrol);                  /* Dynamic power down should be off */
        writel(0x40E01323, &dmc1->memconfig0);

        writel(0xFF000000, &dmc1->prechconfig);
        writel(0xFFFF00FF, &dmc1->pwrdnconfig);

        writel(0x00000618, &dmc1->timingaref);                  /* 7.8us * 200MHz = 1560 = 0x618  */
        writel(0x19233309, &dmc1->timingrow);
        writel(0x23240204, &dmc1->timingdata);
        writel(0x09C80232, &dmc1->timingpower);

        writel(0x07000000, &dmc1->directcmd);                   /* NOP */
        writel(0x01000000, &dmc1->directcmd);                   /* PALL */
        writel(0x00020000, &dmc1->directcmd);                   /* EMRS2 */
        writel(0x00030000, &dmc1->directcmd);                   /* EMRS3 */
        writel(0x00010400, &dmc1->directcmd);                   /* EMRS enable DLL */
        writel(0x00000542, &dmc1->directcmd);                   /* DLL reset */
        writel(0x01000000, &dmc1->directcmd);                   /* PALL */
        writel(0x05000000, &dmc1->directcmd);                   /* auto refresh */
        writel(0x05000000, &dmc1->directcmd);                   /* auto refresh */
        writel(0x00000442, &dmc1->directcmd);                   /* DLL unreset */
        writel(0x00010780, &dmc1->directcmd);                   /* OCD default */
        writel(0x00010400, &dmc1->directcmd);                   /* OCD exit */

        writel(0x0FF02030, &dmc1->concontrol);                  /* auto refresh on */
        writel(0xFFFF00FF, &dmc1->pwrdnconfig);
        writel(0x00202400, &dmc1->memcontrol);
}

void copy_bl2_to_ram(void)
{
/*
** ch:  閫氶亾
** sb:  璧峰鍧?
** bs:  鍧楀ぇ灏?
** dst: 鐩殑鍦?
** i:   鏄惁鍒濆鍖?
*/
#define CopySDMMCtoMem(ch, sb, bs, dst, i) \
        (((u8(*)(int, u32, unsigned short, u32*, u8))\
        (*((u32 *)0xD0037F98)))(ch, sb, bs, dst, i))

#define MP0_1CON  (*(volatile u32 *)0xE02002E0)
#define MP0_3CON  (*(volatile u32 *)0xE0200320)
#define MP0_6CON  (*(volatile u32 *)0xE0200380) 

#define NF8_ReadPage_Adv(a,b,c) (((int(*)(u32, u32, u8*))(*((u32 *)0xD0037F90)))(a,b,c))

        u32 bl2Size = 250 * 1024;       // 250K

        u32 OM = *(volatile u32 *)(0xE0000004); // OM Register
        OM &= 0x1F;                                     // 鍙栦綆5浣?

        if (OM == 0x2)                          // NAND 2 KB, 5cycle 8-bit ECC
        {
                u32 cfg = 0;
                struct s5pv210_nand *nand_reg = (struct s5pv210_nand *)(struct s5pv210_nand *)samsung_get_base_nand();

                /* initialize hardware */
                /* HCLK_PSYS=133MHz(7.5ns) */
                cfg =   (0x1 << 23) |   /* Disable 1-bit and 4-bit ECC */

                                (0x3 << 12) |   /* 7.5ns * 2 > 12ns tALS tCLS */
                                (0x2 << 8) |    /* (1+1) * 7.5ns > 12ns (tWP) */
                                (0x1 << 4) |    /* (0+1) * 7.5 > 5ns (tCLH/tALH) */
                                (0x0 << 3) |    /* SLC NAND Flash */
                                (0x0 << 2) |    /* 2KBytes/Page */
                                (0x1 << 1);             /* 5 address cycle */

                writel(cfg, &nand_reg->nfconf);

                writel((0x1 << 1) | (0x1 << 0), &nand_reg->nfcont);
                /* Disable chip select and Enable NAND Flash Controller */

                /* Config GPIO */
                MP0_1CON &= ~(0xFFFF << 8);
                MP0_1CON |= (0x3333 << 8);
                MP0_3CON = 0x22222222;
                MP0_6CON = 0x22222222;

                int i = 0;
                int pages = bl2Size / 2048;             //
                int offset = 0x4000 / 2048;                     // u-boot.bin
                u8 *p = (u8 *)CONFIG_SYS_SDRAM_BASE;
                for (; i < pages; i++, p += 2048, offset += 1)
                        NF8_ReadPage_Adv(offset / 64, offset % 64, p);
        }
        else if (OM == 0xC)             // SD/MMC
        {
                u32 V210_SDMMC_BASE = *(volatile u32 *)(0xD0037488);    // V210_SDMMC_BASE
                u8 ch = 0;

                /* 7.9.1 SD/MMC REGISTER MAP */
                if (V210_SDMMC_BASE == 0xEB000000)              //
                        ch = 0;
                else if (V210_SDMMC_BASE == 0xEB200000) // 
                        ch = 2;
                CopySDMMCtoMem(ch, 32, bl2Size / 512, (u32 *)CONFIG_SYS_SDRAM_BASE, 0);
        }
}
#endif
