/*
 * (C) Copyright 2006 OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <nand.h>
#include <asm/arch/nand_reg.h>
#include <asm/io.h>

#define MP0_1CON  (*(volatile u32 *)0xE02002E0)
#define	MP0_3CON  (*(volatile u32 *)0xE0200320)
#define	MP0_6CON  (*(volatile u32 *)0xE0200380)

/* modied by shl */
static void s5pv210_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	struct s5pv210_nand *nand = (struct s5pv210_nand *)samsung_get_base_nand();
	debug("hwcontrol(): 0x%02x 0x%02x\n", cmd, ctrl);
	ulong IO_ADDR_W = (ulong)nand;
	if (ctrl & NAND_CTRL_CHANGE) {
		
		if (ctrl & NAND_CLE)		
			IO_ADDR_W = IO_ADDR_W | 0x8;	/* Command Register  */
		else if (ctrl & NAND_ALE)
			IO_ADDR_W = IO_ADDR_W | 0xC;	/* Address Register */
			
		chip->IO_ADDR_W = (void *)IO_ADDR_W;

		if (ctrl & NAND_NCE)	/* select */
			writel(readl(&nand->nfcont) & ~(1 << 1), &nand->nfcont);
		else					/* deselect */
			writel(readl(&nand->nfcont) | (1 << 1), &nand->nfcont);
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, chip->IO_ADDR_W);	
	else
		chip->IO_ADDR_W = &nand->nfdata;

}

static int s5pv210_dev_ready(struct mtd_info *mtd)
{
	struct s5pv210_nand *nand = (struct s5pv210_nand *)samsung_get_base_nand();
	debug("dev_ready\n");
	return readl(&nand->nfstat) & 0x01;
}

#ifdef CONFIG_S5PV210_NAND_HWECC
void s5pv210_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s5pv210_nand *nand = (struct s5pv210_nand *)samsung_get_base_nand();
	debug("s5pv210_nand_enable_hwecc(%p, %d)\n", mtd, mode);
	
	writel(readl(&nand->nfconf) | (0x3 << 23), &nand->nfconf);
	
	if (mode == NAND_ECC_READ)
	{
	}
	else if (mode == NAND_ECC_WRITE)
	{
		/* set 8/12/16bit Ecc direction to Encoding */
		writel(readl(&nand->nfecccont) | (0x1 << 16), &nand->nfecccont);
		/* clear 8/12/16bit ecc encode done */
		writel(readl(&nand->nfeccstat) | (0x1 << 25), &nand->nfeccstat);
	}
	
	/* Initialize main area ECC decoder/encoder */
	writel(readl(&nand->nfcont) | (0x1 << 5), &nand->nfcont);
	
	/* The ECC message size(For 512-byte message, you should set 511)
	* 8-bit ECC/512B */
	writel((511 << 16) | 0x3, &nand->nfeccconf);
	
	writel(readl(&nand->nfstat) | (0x1 << 4) | (0x1 << 5), &nand->nfstat);
	
	/* Initialize main area ECC decoder/ encoder */
	writel(readl(&nand->nfecccont) | (0x1 << 2), &nand->nfecccont);
	
	/* Unlock Main area ECC   */
	writel(readl(&nand->nfcont) & ~(0x1 << 7), &nand->nfcont);
}

/* modied by shl */
static int s5pv210_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				      u_char *ecc_code)
{
	struct s5pv210_nand *nand = (struct s5pv210_nand *)samsung_get_base_nand();
	u32 nfeccprgecc0 = 0, nfeccprgecc1 = 0, nfeccprgecc2 = 0, nfeccprgecc3 = 0;

	/* Lock Main area ECC */
	writel(readl(&nand->nfcont) | (1 << 7), &nand->nfcont);
	
	/* 读取13 Byte的Ecc Code */
	nfeccprgecc0 = readl(&nand->nfeccprgecc0);
	nfeccprgecc1 = readl(&nand->nfeccprgecc1);
	nfeccprgecc2 = readl(&nand->nfeccprgecc2);
	nfeccprgecc3 = readl(&nand->nfeccprgecc3);

	ecc_code[0] = nfeccprgecc0 & 0xff;
	ecc_code[1] = (nfeccprgecc0 >> 8) & 0xff;
	ecc_code[2] = (nfeccprgecc0 >> 16) & 0xff;
	ecc_code[3] = (nfeccprgecc0 >> 24) & 0xff;
	ecc_code[4] = nfeccprgecc1 & 0xff;
	ecc_code[5] = (nfeccprgecc1 >> 8) & 0xff;
	ecc_code[6] = (nfeccprgecc1 >> 16) & 0xff;
	ecc_code[7] = (nfeccprgecc1 >> 24) & 0xff;
	ecc_code[8] = nfeccprgecc2 & 0xff;
	ecc_code[9] = (nfeccprgecc2 >> 8) & 0xff;
	ecc_code[10] = (nfeccprgecc2 >> 16) & 0xff;
	ecc_code[11] = (nfeccprgecc2 >> 24) & 0xff;
	ecc_code[12] = nfeccprgecc3 & 0xff;
	
	debug("s5pv210_nand_calculate_hwecc(%p,):\n"
		"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
		"0x%02x 0x%02x 0x%02x\n", mtd , ecc_code[0], ecc_code[1], ecc_code[2], 
		ecc_code[3], ecc_code[4], ecc_code[5], ecc_code[6], ecc_code[7], 
		ecc_code[8], ecc_code[9], ecc_code[10], ecc_code[11], ecc_code[12]);

	return 0;
}

/* add by shl */
#define NF8_ReadPage_Adv(a,b,c) (((int(*)(u32, u32, u8*))(*((u32 *)0xD0037F90)))(a,b,c))
static int s5pv210_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{
	/* tiny210使用的NAND FLASH一个块64页 */
	return NF8_ReadPage_Adv(page / 64, page % 64, buf);
}

static int s5pv210_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	if (read_ecc[0] == calc_ecc[0] &&
	    read_ecc[1] == calc_ecc[1] &&
	    read_ecc[2] == calc_ecc[2])
		return 0;

	printf("s5pv210_nand_correct_data: not implemented\n");
	return -1;
}
#endif

/*
 * add by shl
 * nand_select_chip
 * @mtd: MTD device structure
 * @ctl: 0 to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void s5pv210_nand_select_chip(struct mtd_info *mtd, int ctl)
{
	struct nand_chip *chip = mtd->priv;

	switch (ctl) {
	case -1:	/* deselect the chip */
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:		/* Select the chip */
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;

	default:
		BUG();
	}
}

/* add by shl */
static struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 52,		/* 2048 / 512 * 13 */
	.eccpos = {	12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
				22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
				32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
				42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
				52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
				62, 63},
	/* 0和1用于保存坏块标记，12~63保存ecc，剩余2~11为free */
	.oobfree = {
			{.offset = 2,
			.length = 10}
		}
};

/* modied by shl */
int board_nand_init(struct nand_chip *nand)
{
	u32 cfg = 0;
	struct s5pv210_nand *nand_reg = (struct s5pv210_nand *)(struct s5pv210_nand *)samsung_get_base_nand();

	debug("board_nand_init()\n");

	/* initialize hardware */
	/* HCLK_PSYS=133MHz(7.5ns) */
	cfg =	(0x1 << 23) |	/* Disable 1-bit and 4-bit ECC */
			/* 下面3个时间参数稍微比计算出的值大些（我这里依次加1），否则读写不稳定 */
			(0x3 << 12) |	/* 7.5ns * 2 > 12ns tALS tCLS */
			(0x2 << 8) | 	/* (1+1) * 7.5ns > 12ns (tWP) */
			(0x1 << 4) | 	/* (0+1) * 7.5 > 5ns (tCLH/tALH) */
			(0x0 << 3) | 	/* SLC NAND Flash */
			(0x0 << 2) |	/* 2KBytes/Page */
			(0x1 << 1);		/* 5 address cycle */
	
	writel(cfg, &nand_reg->nfconf);
	
	writel((0x1 << 1) | (0x1 << 0), &nand_reg->nfcont);
	/* Disable chip select and Enable NAND Flash Controller */
	
	/* Config GPIO */
	MP0_1CON &= ~(0xFFFF << 8);
	MP0_1CON |= (0x3333 << 8);
	MP0_3CON = 0x22222222;
	MP0_6CON = 0x22222222;
	
	/* initialize nand_chip data structure */
	nand->IO_ADDR_R = (void *)&nand_reg->nfdata;
	nand->IO_ADDR_W = (void *)&nand_reg->nfdata;

	nand->select_chip = s5pv210_nand_select_chip;

	/* read_buf and write_buf are default */
	/* read_byte and write_byte are default */

	/* hwcontrol always must be implemented */
	nand->cmd_ctrl = s5pv210_hwcontrol;

	nand->dev_ready = s5pv210_dev_ready;

#ifdef CONFIG_S5PV210_NAND_HWECC
	nand->ecc.hwctl = s5pv210_nand_enable_hwecc;
	nand->ecc.calculate = s5pv210_nand_calculate_ecc;
	nand->ecc.correct = s5pv210_nand_correct_data;
	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.size = CONFIG_SYS_NAND_ECCSIZE;
	nand->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;
	nand->ecc.strength = 1;
	/* add by shl */
	nand->ecc.layout = &nand_oob_64;
	nand->ecc.read_page = s5pv210_nand_read_page_hwecc;
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif

#ifdef CONFIG_S3C2410_NAND_BBT
	nand->bbt_options |= NAND_BBT_USE_FLASH;
#endif

	debug("end of nand_init\n");
	
	return 0;
}

