/* linux/arch/arm/mach-s5pv210/mach-smart210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/device.h>
#include <linux/dm9000.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>
#include <linux/platform_data/s3c-hsotg.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <video/platform_lcd.h>
#include <video/samsung_fimd.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <linux/platform_data/touchscreen-s3c2410.h>
#include <linux/platform_data/ata-samsung_cf.h>
#include <linux/platform_data/i2c-s3c2410.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/fb.h>
#include <plat/samsung-time.h>
#include <plat/backlight.h>
#include <plat/mfc.h>
#include <plat/clock.h>

/* add by Flinn */
#include <plat/nand-core.h>
#include <linux/platform_data/mtd-nand-s3c2410.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/mtd.h>

#include "common.h"

#include <linux/leds.h>

static struct gpio_led leds[] = 
{
	[0] = {
		.name = "led0",
		.default_trigger = "heartbeat",
		.gpio = S5PV210_GPJ2(0),
		.active_low = 1,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data smart210_leds_data = {
	.num_leds = ARRAY_SIZE(leds),
	.leds = leds,
};

static struct platform_device smart210_leds = 
{
	.name = "leds-gpio",
	.dev = {
		.platform_data = &smart210_leds_data,
	},
	.id = -1,
};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMART210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMART210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMART210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smart210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMART210_UCON_DEFAULT,
		.ulcon		= SMART210_ULCON_DEFAULT,
		.ufcon		= SMART210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMART210_UCON_DEFAULT,
		.ulcon		= SMART210_ULCON_DEFAULT,
		.ufcon		= SMART210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMART210_UCON_DEFAULT,
		.ulcon		= SMART210_ULCON_DEFAULT,
		.ufcon		= SMART210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMART210_UCON_DEFAULT,
		.ulcon		= SMART210_ULCON_DEFAULT,
		.ufcon		= SMART210_UFCON_DEFAULT,
	},
};

static struct s3c_ide_platdata smart210_ide_pdata __initdata = {
	.setup_gpio	= s5pv210_ide_setup_gpio,
};

static uint32_t smart210_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 3, KEY_1), KEY(0, 4, KEY_2), KEY(0, 5, KEY_3),
	KEY(0, 6, KEY_4), KEY(0, 7, KEY_5),
	KEY(1, 3, KEY_A), KEY(1, 4, KEY_B), KEY(1, 5, KEY_C),
	KEY(1, 6, KEY_D), KEY(1, 7, KEY_E)
};

static struct matrix_keymap_data smart210_keymap_data __initdata = {
	.keymap		= smart210_keymap,
	.keymap_size	= ARRAY_SIZE(smart210_keymap),
};

static struct samsung_keypad_platdata smart210_keypad_data __initdata = {
	.keymap_data	= &smart210_keymap_data,
	.rows		= 8,
	.cols		= 8,
};

static struct resource smart210_dm9000_resources[] = {
	[0] = DEFINE_RES_MEM(S5PV210_PA_SROM_BANK1, 4),
	[1] = DEFINE_RES_MEM(S5PV210_PA_SROM_BANK1 + 4, 4),
	[2] = DEFINE_RES_NAMED(IRQ_EINT(7), 1, NULL, IORESOURCE_IRQ \
				| IORESOURCE_IRQ_HIGHLEVEL),
};

static struct dm9000_plat_data smart210_dm9000_platdata = {
	.flags		= DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
	.dev_addr	= { 0x00, 0x09, 0xc0, 0xff, 0xec, 0x48 },
};

static struct platform_device smart210_dm9000 = {
	.name		= "dm9000",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smart210_dm9000_resources),
	.resource	= smart210_dm9000_resources,
	.dev		= {
		.platform_data	= &smart210_dm9000_platdata,
	},
};



static void smart210_lte480wv_set_power(struct plat_lcd_data *pd,
					unsigned int power)
{
	if (power) {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_HIGH, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif

		/* fire nRESET on power up */
		gpio_request_one(S5PV210_GPH0(6), GPIOF_OUT_INIT_HIGH, "GPH0");

		gpio_set_value(S5PV210_GPH0(6), 0);
		mdelay(10);

		gpio_set_value(S5PV210_GPH0(6), 1);
		mdelay(10);

		gpio_free(S5PV210_GPH0(6));
	} else {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_LOW, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif
	}
}

static struct plat_lcd_data smart210_lcd_lte480wv_data = {
	.set_power	= smart210_lte480wv_set_power,
};

static struct platform_device smart210_lcd_lte480wv = {
	.name			= "platform-lcd",
	.dev.parent		= &s3c_device_fb.dev,
	.dev.platform_data	= &smart210_lcd_lte480wv_data,
};

static struct s3c_fb_pd_win smart210_fb_win0 = {
	.max_bpp	= 32,
	.default_bpp	= 24,
	.xres		= 800,
	.yres		= 480,
};

static struct fb_videomode smart210_lcd_timing = {
	.left_margin	= 13,
	.right_margin	= 8,
	.upper_margin	= 7,
	.lower_margin	= 5,
	.hsync_len	= 3,
	.vsync_len	= 1,
	.xres		= 800,
	.yres		= 480,
};

static struct s3c_fb_platdata smart210_lcd0_pdata __initdata = {
	.win[0]		= &smart210_fb_win0,
	.vtiming	= &smart210_lcd_timing,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
	.setup_gpio	= s5pv210_fb_gpio_setup_24bpp,
};

/* USB OTG */
static struct s3c_hsotg_plat smart210_hsotg_pdata;

/* nand info (add by Flinn) */
static struct mtd_partition smdk_default_nand_part[] = {
	[0] = {
		.name	= "bootloader",
		.size	= SZ_256K,
		.offset	= 0,
	},
	[1] = {
                .name   = "device_tree",
                .offset = MTDPART_OFS_APPEND,
                .size   = SZ_128K,
        },
	[2] = {
		.name	= "params",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_128K,
	},
	[3] = {
		.name	= "kernel",
		.offset	= MTDPART_OFS_APPEND,
		.size	= SZ_1M + SZ_4M,
	},

	[4] = {
		.name	= "rootfs",
		.offset = MTDPART_OFS_APPEND,
		.size	= MTDPART_SIZ_FULL,
	}
};

static struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 52,		/* 2048 / 512 * 13 */
	.eccpos = {	12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
				22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
				32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
				42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
				52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
				62, 63},
	/* 0�?1用于保存坏块标记�?12~63保存ecc，剩�?2~11为free */
	.oobfree = {
			{.offset = 2,
			.length = 10}
		}
};

static struct s3c2410_nand_set smdk_nand_sets[] = {
	[0] = {
		.name		= "NAND",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(smdk_default_nand_part),
		.partitions	= smdk_default_nand_part,
		.disable_ecc = 1,
	},
};

static struct s3c2410_platform_nand smdk_nand_info = {
	.tacls		= 12,
	.twrph0		= 12,
	.twrph1		= 5,
	.nr_sets	= ARRAY_SIZE(smdk_nand_sets),
	.sets		= smdk_nand_sets,
};

static void s5pv210_nand_gpio_cfg(void)
{
	volatile unsigned long *mp01;
	volatile unsigned long *mp03;
	volatile unsigned long *mp06;
	
	mp01 = (volatile unsigned long *)ioremap(0xE02002E0, 4);
	mp03 = (volatile unsigned long *)ioremap(0xE0200320, 4);
	mp06 = (volatile unsigned long *)ioremap(0xE0200380, 4);
	
	*mp01 &= ~(0xFFFF << 8);
	*mp01 |= (0x3333 << 8);
	*mp03 = 0x22222222;
	*mp06 = 0x22222222;
	
	iounmap(mp01);
	iounmap(mp03);
	iounmap(mp06);
}


static struct platform_device *smart210_devices[] __initdata = {
	&s3c_device_adc,
	&s3c_device_cfcon,
	&s3c_device_fb,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c2,
	&s3c_device_rtc,
	&s3c_device_ts,
	&s3c_device_usb_hsotg,
	&s3c_device_wdt,
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc_md,
	&s5p_device_jpeg,
	&s5p_device_mfc,
	&s5p_device_mfc_l,
	&s5p_device_mfc_r,
	&s5pv210_device_ac97,
	&s5pv210_device_iis0,
	&s5pv210_device_spdif,
	&samsung_asoc_idma,
	&samsung_device_keypad,
	&smart210_dm9000,
	&smart210_lcd_lte480wv,
	&s3c_device_nand,	/* add by flinn for nand  */
	&smart210_leds,     /* add by flinn for heartbeat */
};

static void __init smart210_dm9000_init(void)
{
	unsigned int tmp;

	gpio_request(S5PV210_MP01(1), "nCS1");
	s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(2));
	gpio_free(S5PV210_MP01(1));

	tmp = (5 << S5P_SROM_BCX__TACC__SHIFT);
	__raw_writel(tmp, S5P_SROM_BC1);

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= (S5P_SROM_BW__CS_MASK << S5P_SROM_BW__NCS1__SHIFT);
	tmp |= (3 << S5P_SROM_BW__NCS1__SHIFT);
	__raw_writel(tmp, S5P_SROM_BW);
}

static struct i2c_board_info smart210_i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },     /* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("wm8960", 0x1a), },
};

static struct i2c_board_info smart210_i2c_devs1[] __initdata = {
	/* To Be Updated */
};

static struct i2c_board_info smart210_i2c_devs2[] __initdata = {
	/* To Be Updated */
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smart210_bl_gpio_info = {
	.no = S5PV210_GPD0(3),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smart210_bl_data = {
	.pwm_id = 3,
	.pwm_period_ns = 1000,
};

static void __init smart210_map_io(void)
{
	s5pv210_init_io(NULL, 0);
	s3c24xx_init_clocks(clk_xusbxti.rate);
	s3c24xx_init_uarts(smart210_uartcfgs, ARRAY_SIZE(smart210_uartcfgs));
	samsung_set_timer_source(SAMSUNG_PWM2, SAMSUNG_PWM4);
}

static void __init smart210_reserve(void)
{
	s5p_mfc_reserve_mem(0x43000000, 8 << 20, 0x51000000, 8 << 20);
}

static void __init smart210_machine_init(void)
{
	s3c_pm_init();

	smart210_dm9000_init();
	/* add by Flinn */
	s3c_nand_setname("s5pv210-nand");
	s3c_nand_set_platdata(&smdk_nand_info);
	s5pv210_nand_gpio_cfg();

	samsung_keypad_set_platdata(&smart210_keypad_data);
	s3c24xx_ts_set_platdata(NULL);

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, smart210_i2c_devs0,
			ARRAY_SIZE(smart210_i2c_devs0));
	i2c_register_board_info(1, smart210_i2c_devs1,
			ARRAY_SIZE(smart210_i2c_devs1));
	i2c_register_board_info(2, smart210_i2c_devs2,
			ARRAY_SIZE(smart210_i2c_devs2));

	s3c_ide_set_platdata(&smart210_ide_pdata);

	s3c_fb_set_platdata(&smart210_lcd0_pdata);

	samsung_bl_set(&smart210_bl_gpio_info, &smart210_bl_data);

	s3c_hsotg_set_platdata(&smart210_hsotg_pdata);

	platform_add_devices(smart210_devices, ARRAY_SIZE(smart210_devices));
}

MACHINE_START(SMART210, "SMART210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.atag_offset	= 0x100,
	.init_irq	= s5pv210_init_irq,
	.map_io		= smart210_map_io,
	.init_machine	= smart210_machine_init,
	.init_time	= samsung_timer_init,
	.restart	= s5pv210_restart,
	.reserve	= &smart210_reserve,
MACHINE_END
