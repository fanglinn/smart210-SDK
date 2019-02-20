# u-boot-2014.04移植smart210记录



reference : smdkc100
gcc : arm-linux-gcc-4.5.1



## 1.建立單板

vim Makefile

```shell
CROSS_COMPILE ?= /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-
```



```
cp -r board/samsung/smdkc100/ board/samsung/smart210
mv board/samsung/smart210/smdkc100.c board/samsung/smart210/smart210.c
vim board/samsung/smart210/Makefile
	obj-y   := smart210.o onenand.o
	
cp include/configs/smdkc100.h include/configs/smart210.h
vim boards.cfg
	Active  arm         armv7    s5pc1xx     samsung       smart210    smart210     -       Flinn <flinn682@foxmail.com>
```

```
make smart210_config
make all
```



## 2.移植 u-boot-spl.bin

我们采用 SPL 方式，因此需要在单板配置文件 

### smart210.h 

vim include/configs/smart210.h 

```c
#define CONFIG_SPL
```

make all

报错：

```shell
arch/arm/lib/built-in.o: In function `_main':
/home/flinn/work/u-boot-2014.04/arch/arm/lib/crt0.S:74: undefined reference to `board_init_f'
make[1]: *** [spl/u-boot-spl] Error 1
make: *** [spl/u-boot-spl] Error 2
```

但是编译出错，我们前面分析得知在 u-boot-2014.04/arch/arm/lib/board.c 中定义了一个函数 board_init_f，这个函数进行了非常多的初始化操作，由于 u-boot-spl.bin 的最终目的只是把 BL2 从外部存储器（SD 卡、NAND）拷贝到 SDRAM，所以 u-boot-spl.bin 只需初始化时钟、SDRAM、NAND，然后调用一个拷贝函数，拷贝完成后直接跳转到 SDRAM 执行 BL2，就完事了，我们可以修改 crt0.S，不让其调用 board_init_f。



### crt0.S

vim arch/arm/lib/crt0.S

```asm
#if !defined(CONFIG_SPL_BUILD)
	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	sub	sp, sp, #GD_SIZE	/* allocate one GD above SP */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	mov	r9, sp		/* GD is above SP */
	mov	r0, #0
#endif

/* add by FLinn */
#ifdef CONFIG_SPL_BUILD
        bl      copy_bl2_to_ram
        ldr pc, =CONFIG_SYS_SDRAM_BASE
#else
        bl      board_init_f
#endif
```

copy_bl2_to_ram在smart210.c里面实现，用于从SD或者nand中拷贝bl2到DDR，完成拷贝后直接跳转到DDR的起始地址。

### smart210.c

vim board/samsung/smart210/smart210.c

```c
#ifndef CONFIG_SPL_BUILD        /* add by Flinn */
。。。
#else
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
```

由于里面使用了nand相关寄存器，需要添加nand_reg.h

### nand_reg.h

vim arch/arm/include/asm/arch-s5pc1xx/nand_reg.h

```c
#ifndef __ASM_ARM_ARCH_NAND_REG_H_
#define __ASM_ARM_ARCH_NAND_REG_H_

#ifndef __ASSEMBLY__

struct s5pv210_nand {
	u32	nfconf;
	u32	nfcont;
	u32	nfcmmd;
	u32	nfaddr;
	u32	nfdata;
	u32	nfmeccd0;
	u32	nfmeccd1;
	u32	nfseccd;
	u32 nfsblk;
	u32 nfeblk;
	u32	nfstat;
	u32 nfeccerr0;
	u32 nfeccerr1;
	u32 nfmecc0;
	u32 nfmecc1;
	u32 nfsecc;
	u32 nfmlcbitpt;
	u8 res0[0x1ffbc];
	u32 nfeccconf;
	u8 res1[0x1c];
	u32 nfecccont;
	u8 res2[0xc];
	u32 nfeccstat;
	u8 res3[0xc];
	u32 nfeccsecstat;
	u8 res4[0x4c];
	u32 nfeccprgecc0;
	u32 nfeccprgecc1;
	u32 nfeccprgecc2;
	u32 nfeccprgecc3;
	u32 nfeccprgecc4;
	u32 nfeccprgecc5;
	u32 nfeccprgecc6;
	u8 res5[0x14];
	u32 nfeccerl0;
	u32 nfeccerl1;
	u32 nfeccerl2;
	u32 nfeccerl3;
	u32 nfeccerl4;
	u32 nfeccerl5;
	u32 nfeccerl6;
	u32 nfeccerl7;
	u8 res6[0x10];
	u32 nfeccerp0;
	u32 nfeccerp1;
	u32 nfeccerp2;
	u32 nfeccerp3;
	u8 res7[0x10];
	u32 nfeccconecc0;
	u32 nfeccconecc1;
	u32 nfeccconecc2;
	u32 nfeccconecc3;
	u32 nfeccconecc4;
	u32 nfeccconecc5;
	u32 nfeccconecc6;
};

#endif

#endif
```

并在smart210.c包含

```c
#include <asm/arch/nand_reg.h>          /* add by Flinn */
```

編譯報錯：

```shell
board/samsung/smart210/built-in.o: In function `copy_bl2_to_ram':
/home/flinn/work/u-boot-2014.04/board/samsung/smart210/smart210.c:112: undefined reference to `samsung_get_base_nand'
make[1]: *** [spl/u-boot-spl] Error 1
make: *** [spl/u-boot-spl] Error 2
```

### cpu.h

vim arch/arm/include/asm/arch-s5pc1xx/cpu.h

```c
/* S5PV210 add by Flinn */
#define S5PV210_PRO_ID          0xE0000000
#define S5PV210_CLOCK_BASE      0xE0100000
#define S5PV210_GPIO_BASE       0xE0200000
#define S5PV210_PWMTIMER_BASE   0xE2500000
#define S5PV210_WATCHDOG_BASE   0xE2700000
#define S5PV210_UART_BASE       0xE2900000
#define S5PV210_SROMC_BASE      0xE8000000
#define S5PV210_MMC_BASE        0xEB000000
#define S5PV210_DMC0_BASE       0xF0000000
#define S5PV210_DMC1_BASE       0xF1400000
#define S5PV210_VIC0_BASE       0xF2000000
#define S5PV210_VIC1_BASE       0xF2100000
#define S5PV210_VIC2_BASE       0xF2200000
#define S5PV210_VIC3_BASE       0xF2300000
#define S5PV210_NAND_BASE       0xB0E00000
#define S5PV210_LCD_BASE        0xF8000000
。。。
#define SAMSUNG_BASE(device, base)                              \
static inline unsigned int samsung_get_base_##device(void)      \
{                                                               \
        return S5PV210_##base;          \
}
...
/* add by Flinn */
SAMSUNG_BASE(dmc0, DMC0_BASE)
SAMSUNG_BASE(dmc1, DMC1_BASE)
SAMSUNG_BASE(nand, NAND_BASE)
```

编译出错：

```shell
/home/flinn/work/u-boot-2014.04/tools/mkexynosspl  spl/u-boot-spl.bin spl/smart210-spl.bin
make[1]: /home/flinn/work/u-boot-2014.04/tools/mkexynosspl: Command not found
make[1]: *** [spl/smart210-spl.bin] Error 127
make: *** [spl/u-boot-spl] Error 2
```

### tools/mksmart210.c

vim tools/mksmart210.c

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IMG_SIZE        16 * 1024
#define HEADER_SIZE     16

int main (int argc, char *argv[])
{
        FILE                    *fp;
        unsigned                char *buffer;
        int                             bufferLen;
        int                             nbytes, fileLen;
        unsigned int    checksum, count;
        int                             i;

        if (argc != 3)
        {
                printf("Usage: %s <source file> <destination file>\n", argv[0]);
                return -1;
        }

        /* 分配16KByte的buffer,BL1最大为16KByte,并初始化为0 */
        buffer = calloc(1, IMG_SIZE);
        if (!buffer)
        {
                perror("Alloc buffer failed!");
                return -1;
        }

        /* 打开源bin文件 */
        fp = fopen(argv[1], "rb");
        if( fp == NULL)
        {
                perror("source file open error");
                free(buffer);
                return -1;
        }
        /* 获取源bin文件的长度 */
        fseek(fp, 0L, SEEK_END);
        fileLen = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        /* 源bin文件不得超过(16K-16)Byte */
        if (fileLen > (IMG_SIZE - HEADER_SIZE))
        {
                fprintf(stderr, "Source file is too big(> 16KByte)\n");
                free(buffer);
                fclose(fp);
        }

        /* 计算校验和 */
        i = 0;
        checksum = 0;
        while (fread(buffer + HEADER_SIZE + i, 1, 1, fp))
        {
                checksum += buffer[HEADER_SIZE + i++];
        }
        fclose(fp);


        /* 计算BL1的大小(BL1的大小包括BL1的头信息),并保存到buffer[0~3]中 */
        fileLen += HEADER_SIZE;
        memcpy(buffer, &fileLen, 4);

        // 将校验和保存在buffer[8~15]
        memcpy(buffer + 8, &checksum, 4);

        /* 打开目标文件 */
        fp = fopen(argv[2], "wb");
        if (fp == NULL)
        {
                perror("destination file open error");
                free(buffer);
                return -1;
        }
		// 将buffer拷贝到目标bin文件中
        nbytes  = fwrite(buffer, 1, fileLen, fp);
        if (nbytes != fileLen)
        {
                perror("destination file write error");
                free(buffer);
                fclose(fp);
                return -1;
        }

        free(buffer);
        fclose(fp);

        return 0;
}
```

编译gcc -o tools/mksmart210spl tools/mksmart210.c



### spl/Makefile

vim spl/Makefile

```shell
$(obj)/$(BOARD)-spl.bin: $(obj)/u-boot-spl.bin
        $(if $(wildcard $(objtree)/spl/board/samsung/$(BOARD)/tools/mk$(BOARD)spl),\
        $(objtree)/spl/board/samsung/$(BOARD)/tools/mk$(BOARD)spl,\
        $(objtree)/tools/mk$(BOARD)spl) $< $@
endif
```

编译OK

## 烧录

```shell
sudo dd iflag=sync oflag=sync if=spl/smart210-spl.bin of=/dev/sdc seek=1
sudo dd iflag=sync oflag=sync if=u-boot.bin of=/dev/sdc seek=32
```

沒有任何输出。

### vim write2sd.sh

```shell
#!/bin/bash
sudo dd iflag=sync oflag=sync if=spl/smart210-spl.bin of=/dev/sdc seek=1
sudo dd iflag=sync oflag=sync if=u-boot.bin of=/dev/sdc seek=32
sync
echo "done."
```

chmod +x write2sd.sh 



### lowlevel_init.S

vim board/samsung/smart210/lowlevel_init.S

```asm
        .globl lowlevel_init
lowlevel_init:
        mov     r9, lr

#ifdef CONFIG_SPL_BUILD
        bl clock_init                   /* clock init */
        bl ddr_init                     /* DDR init */

        /* add by Flinn, for uart */
        ldr r0, =0xE0200000     /* GPA0_CON */
        ldr     r1, =0x22222222
        str     r1, [r0]

#endif
        mov     pc, r9                /* return */

```

clock_init 和 ddr_init在smart210.c里面实现

vim board/samsung/smart210/smart210.c

```c
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
```

smart210.c里面添加头文件

```c
#include <asm/arch/clock.h>             /* add by Flinn */
#include <asm/arch/dmc.h>               /* add by Flinn */
```

### clock.h

vim arch/arm/include/asm/arch-s5pc1xx/clock.h

```c
/* add by Flinn */
struct s5pv210_clock {
        unsigned int    apll_lock;
        unsigned char   res1[0x04];
        unsigned int    mpll_lock;
        unsigned char   res2[0x04];
        unsigned int    epll_lock;
        unsigned char   res3[0x0C];
        unsigned int    vpll_lock;
        unsigned char   res4[0xdc];
        unsigned int    apll_con0;
        unsigned int    apll_con1;
        unsigned int    mpll_con;
        unsigned char   res5[0x04];
        unsigned int    epll_con0;
        unsigned int    epll_con1;
        unsigned char   res6[0x08];
        unsigned int    vpll_con;
        unsigned char   res7[0xdc];
        unsigned int    src0;
        unsigned int    src1;
        unsigned int    src2;
        unsigned int    src3;
        unsigned int    src4;
        unsigned int    src5;
        unsigned int    src6;
        unsigned char   res8[0x64];
        unsigned int    mask0;
        unsigned int    mask1;
        unsigned char   res9[0x78];
        unsigned int    div0;
        unsigned int    div1;
        unsigned int    div2;
        unsigned int    div3;
        unsigned int    div4;
        unsigned int    div5;
        unsigned int    div6;
        unsigned int    div7;
};

```

### dmc.h

vim arch/arm/include/asm/arch-s5pc1xx/dmc.h

```c
#ifndef __ASM_ARM_ARCH_DRAM_H_
#define __ASM_ARM_ARCH_DRAM_H_

#ifndef __ASSEMBLY__

struct s5pv210_dmc0 {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned char	res1[0x08];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus;
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
};

struct s5pv210_dmc1 {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned char	res1[0x08];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus;
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
};

#endif

#endif
```

编译OK

烧录后沒有任何输出：

### smart210.h 

vim include/configs/smart210.h

```c
#define CONFIG_SYS_SDRAM_BASE		0x20000000    /* modied by Flinn */

/* Text Base */
#define CONFIG_SYS_TEXT_BASE		0x20000000   /*modied by Flinn */

#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_SDRAM_BASE + PHYS_SDRAM_1_SIZE)
```

重新编译，烧录后，

```bash
U-Boot 2014.04 (Feb 19 2019 - 19:43:09) for SMDKC100

CPU:    S5PC110@1000MHz
Board:  SMDKC100
DRAM:  128 MiB
WARNING: Caches not enabled
```

卡死了。

原因是smart210沒有onenand



去掉onenand

vim include/configs/smart210.h

```c
#if 0
#define CONFIG_CMD_ONENAND
#endif
```

编译报错 ：

```bash
common/built-in.o: In function `env_relocate_spec':
/home/flinn/work/u-boot-2014.04/common/env_onenand.c:64: undefined reference to `onenand_mtd'
common/built-in.o: In function `saveenv':
/home/flinn/work/u-boot-2014.04/common/env_onenand.c:112: undefined reference to `onenand_mtd'
```

去掉CONFIG_ENV_IS_IN_ONENAND

vim include/configs/smart210.h

```c
#if 0
#define CONFIG_ENV_IS_IN_ONENAND        1
#endif
```

再编译：

```bash
common/cmd_nvedit.c:51:3: error: #error Define one of CONFIG_ENV_IS_IN_{EEPROM|FLASH|DATAFLASH|ONENAND|SPI_FLASH|NVRAM|MMC|FAT|REMOTE|UBI} or CONFIG_ENV_IS_NOWHERE
```

需要定义CONFIG_ENV_IS_IN_NAND

再编译

```bash
/home/flinn/work/u-boot-2014.04/common/env_nand.c:259: undefined reference to `nand_read_skip_bad'
/home/flinn/work/u-boot-2014.04/common/env_nand.c:273: undefined reference to `nand_info'
common/built-in.o: In function `do_env_save':
/home/flinn/work/u-boot-2014.04/common/cmd_nvedit.c:692: undefined reference to `saveenv'
```

需要对nand的支持

### s5pv210_nand.c

vim drivers/mtd/nand/s5pv210_nand.c

```c
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

```

vim drivers/mtd/nand/Makefile

```bash
obj-$(CONFIG_NAND_S5PV210) += s5pv210_nand.o
```

此外修改smart210.h

```c
#define CONFIG_CMD_NAND

#define CONFIG_SYS_MAX_NAND_DEVICE  1
#define CONFIG_SYS_NAND_BASE        0xB0E00000
#define CONFIG_NAND_S5PV210 
#define CONFIG_S5PV210_NAND_HWECC
#define CONFIG_SYS_NAND_ECCSIZE   512
#define CONFIG_SYS_NAND_ECCBYTES 13
```

编译下载后：

```bash
U-Boot 2014.04 (Feb 19 2019 - 19:54:24) for SMDKC100

CPU:    S5PC110@1000MHz
Board:  SMDKC100
DRAM:  128 MiB
WARNING: Caches not enabled
NAND:  1024 MiB
*** Warning - bad CRC, using default environment

In:    serial
Out:   serial
Err:   serial
Net:   smc911x: Invalid chip endian 0x0c070c07
No ethernet found.
Hit any key to stop autoboot:  0
Unknown command 'onenand' - try 'help'
Wrong Image Format for bootm command
ERROR: can't get kernel image!
SMDKC100 #
```

修改DRAM：

在smart210.h

```c
#define CONFIG_SYS_PROMPT               "smart210 # "
#define PHYS_SDRAM_1_SIZE       (512 << 20)     /* 0x8000000, 512 MB Bank #1 */
#define CONFIG_IDENT_STRING             " for SMART210"
```

输出：

```bash
U-Boot 2014.04 (Feb 19 2019 - 19:57:47) for SMART210

CPU:    S5PC110@1000MHz
Board:  SMDKC100
DRAM:  512 MiB
WARNING: Caches not enabled
NAND:  1024 MiB
*** Warning - bad CRC, using default environment

In:    serial
Out:   serial
Err:   serial
Net:   smc911x: Invalid chip endian 0x0c070c07
No ethernet found.
Hit any key to stop autoboot:  0
Unknown command 'onenand' - try 'help'
Wrong Image Format for bootm command
ERROR: can't get kernel image!
```

## DM9000

vim board/samsung/smart210/smart210.c

```c
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

/* add by shl  */
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
	/* masked by shl */
	//smc9115_pre_init();
 	dm9000_pre_init();
	gd->bd->bi_arch_number = MACH_TYPE_SMDKC100;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_SMC911X
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
    /*add by shl*/
#elif defined(CONFIG_DRIVER_DM9000)
	rc = dm9000_initialize(bis);
#endif	
	return rc;
}
```

vim include/configs/smart210.h

```c
#if 0
#define CONFIG_SMC911X         1       /* we have a SMC9115 on-board   */
#define CONFIG_SMC911X_16_BIT  1       /* SMC911X_16_BIT Mode          */
#define CONFIG_SMC911X_BASE    0x98800300      /* SMC911X Drive Base   */
#endif

#define CONFIG_ENV_SROM_BANK   1       /* Select SROM Bank-1 for Ethernet*/
#define CONFIG_DRIVER_DM9000
#define CONFIG_DM9000_NO_SROM
#define CONFIG_DM9000_BASE              0x88000000
#define DM9000_IO                               (CONFIG_DM9000_BASE)
#define DM9000_DATA                             (CONFIG_DM9000_BASE + 0x4)
#define CONFIG_CMD_PING
#define CONFIG_IPADDR                   192.168.1.120
#define CONFIG_SERVERIP                 192.168.1.101
#define CONFIG_ETHADDR                  1A:2A:3A:4A:5A:6A
#endif /* CONFIG_CMD_NET */

```

编译后：

```bash
U-Boot 2014.04 (Feb 19 2019 - 20:03:56) for SMART210

CPU:    S5PC110@1000MHz
Board:  SMDKC100
DRAM:  512 MiB
WARNING: Caches not enabled
NAND:  1024 MiB
*** Warning - bad CRC, using default environment

In:    serial
Out:   serial
Err:   serial
Net:   dm9000
Hit any key to stop autoboot:  0
Unknown command 'onenand' - try 'help'
Wrong Image Format for bootm command
ERROR: can't get kernel image!
smart210 #
smart210 # ping 192.168.1.102
dm9000 i/o: 0x88000000, id: 0x90000a46
DM9000: running in 16 bit mode
MAC: 1a:2a:3a:4a:5a:6a
operating at 100M full duplex mode
Using dm9000 device
ping failed; host 192.168.1.102 is not alive
smart210 #

smart210 # ping 192.168.1.104
dm9000 i/o: 0x88000000, id: 0x90000a46
DM9000: running in 16 bit mode
MAC: 1a:2a:3a:4a:5a:6a
operating at 100M full duplex mode
Using dm9000 device
host 192.168.1.104 is alive
smart210 #
```



## 支持mtdpart



vim include/configs/smart210.h

```c
#define MTDIDS_DEFAULT          "nand0=s5p-nand"
#define MTDPARTS_DEFAULT        "mtdparts=s5p-nand:256k(bootloader)"\
                                ",128k@0x40000(params)"\
                                ",3m@0x60000(kernel)"\
                                ",-(rootfs)"

```

输出

```bash
smart210 # mtdpart

device nand0 <s5p-nand>, # parts = 4
 #: name                size            offset          mask_flags
 0: bootloader          0x00040000      0x00000000      0
 1: params              0x00020000      0x00040000      0
 2: kernel              0x00300000      0x00260000      0
 3: rootfs              0x3faa0000      0x00560000      0

active partition: nand0,0 - (bootloader) 0x00040000 @ 0x00000000

defaults:
mtdids  : nand0=s5p-nand
mtdparts: mtdparts=s5p-nand:256k(bootloader),128k@0x40000(params),3m@0x260000(kernel),-(rootfs)
```

## NAND启动



```shell
nand erase.chip
```

使用 tftpboot 下载smart210-spl.bin 到 DDR 的起始地址 0x20000000

```shell
tftp 20000000 smart210-spl.bin
```

烧写 smart210-spl.bin 到 NAND 的 0 地址:

```shell
nand write 20000000 0 $filesize
```

使用 tftpboot 下载 u-boot.bin 到 DDR 的起始地址 0x20000000

```shell
tftp 20000000 u-boot.bin
```

烧写 u-boot.bin 到 NAND FLASH 的 0x4000(16K) 地址（0x0~0x3FFF 预留给 smart210-spl.bin）

```shell
nand write 20000000 4000 $filesize
```

拨动拨码开关，从 NAND 启动开发板,可以观察到u-boot正常启动了

```bash
U-Boot 2014.04 (Feb 20 2019 - 10:14:59) for SMART210

CPU:    S5PC110@1000MHz
Board:  SMDKC100
DRAM:  512 MiB
WARNING: Caches not enabled
NAND:  1024 MiB
*** Warning - bad CRC, using default environment

In:    serial
Out:   serial
Err:   serial
Net:   dm9000
Hit any key to stop autoboot:  0
Unknown command 'onenand' - try 'help'
Wrong Image Format for bootm command
ERROR: can't get kernel image!
```





