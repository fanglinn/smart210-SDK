# linux-3.10.79移植到smart210





## 1.顶层Makefile

vim Makefile

```bash
ARCH            ?= arm
CROSS_COMPILE 	?= /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-
```

## 2.编译

```
make s5pv210_defconfig
make uImage
```



下载

```
tftp 20000000 uImage
bootm 20000000
```

输出:

```bash
smart210 # bootm 20000000
## Booting kernel from Legacy Image at 20000000 ...
   Image Name:   Linux-3.10.79
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    1157768 Bytes = 1.1 MiB
   Load Address: 20008000
   Entry Point:  20008000
   Verifying Checksum ... OK
   Loading Kernel Image ... OK

Starting kernel ...
```

这是因为串口没有选择对

```
make menuconfig
    System Type  ---> 
    	(0) S3C UART to use for low-level messages
    S5PC110 Machines  --->
    	// remove all
    S5PV210 Machines  --->
    	[*] SMDKV210
```

输出：

```bash
smart210 # bootm 20000000
## Booting kernel from Legacy Image at 20000000 ...
   Image Name:   Linux-3.10.79
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    1320280 Bytes = 1.3 MiB
   Load Address: 20008000
   Entry Point:  20008000
   Verifying Checksum ... OK
   Loading Kernel Image ... OK

Starting kernel ...

Uncompressing Linux... done, booting the kernel.

Error: unrecognized/unsupported machine ID (r1 = 0x00000722).

Available machine support:

ID (hex)        NAME
00000998        SMDKV210

Please check your kernel config and/or bootloader.
```

id不匹配，有两个方法：

①修改u-boot，id改为0x998

默认MACH_TYPE_SMDKC100=1826（0x722）， 应该改为MACH_TYPE_SMDKV210 （0x998）

vim board/samsung/smart210/smart210.c

```
int board_init(void)
{
    ...
    -gd->bd->bi_arch_number = MACH_TYPE_SMDKC100;
    +gd->bd->bi_arch_number = MACH_TYPE_SMDKV210 ;
    ...
}
```

重新编译即可

②修改环境变量

```bash
set machid 998
save
```

此时可以正常启动内核，输出：

```bash
## Booting kernel from Legacy Image at 20000000 ...
   Image Name:   Linux-3.10.79
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    1320280 Bytes = 1.3 MiB
   Load Address: 20008000
   Entry Point:  20008000
   Verifying Checksum ... OK
   Loading Kernel Image ... OK
Using machid 0x998 from environment

Starting kernel ...

Uncompressing Linux... done, booting the kernel.
Booting Linux on physical CPU 0x0
Linux version 3.10.79 (flinn@flinn) (gcc version 4.5.1 (ctng-1.8.1-FA) ) #1 PREEMPT Wed Feb 20 12:38:06 CST 2019
...
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
CPU: 0 PID: 1 Comm: swapper Not tainted 3.10.79 #1
```



## dm9000

在 arch/arm/mach-s5pv210/mach-smdkv210.c 中已经配置了 DM9000 的平台设备相关的数据，我们只
需要修改就行了。

在 arch/arm/mach-s5pv210/include/mach/map.h 中只定义了 S5PV210_PA_SROM_BANK5 的基地址，
smart210 开发板的 DM9000 接在 BANK1，因此我们需要在这个文件中定义 S5PV210_PA_SROM_BANK1 的基
地址 

```c
#define S5PV210_PA_SROM_BANK5           0xA8000000
#define S5PV210_PA_SROM_BANK1           0x88000000
```

然后修改 arch/arm/mach-s5pv210/mach-smdkv210.c 中的 DM9000 配置 

vim arch/arm/mach-s5pv210/mach-smdkv210.c 

```c
static struct resource smdkv210_dm9000_resources[] = {
        [0] = DEFINE_RES_MEM(S5PV210_PA_SROM_BANK1, 4),
        [1] = DEFINE_RES_MEM(S5PV210_PA_SROM_BANK1 + 4, 4),
        [2] = DEFINE_RES_NAMED(IRQ_EINT(10), 1, NULL, IORESOURCE_IRQ \
                                | IORESOURCE_IRQ_HIGHLEVEL),
};
```

此时不支持nand，所以配置nfs

```
[*] Networking support  --->
	     Networking options  --->
	     	<*> Packet socket                                                                     
			<*>   Packet: sockets monitoring interface                                           
  			<*> Unix domain sockets                                                               
  			<*>   UNIX: socket monitoring interface
  			[*] TCP/IP networking   
  			[*]   IP: multicasting                                                 
      		[*]   IP: advanced router                                
  			[*]     FIB TRIE statistics         
  			[*]     IP: policy routing
			[*]     IP: equal cost multipath      
  			[*]     IP: verbose route monitoring  
  			[*]   IP: kernel level autoconfiguration
  			[*]     IP: DHCP support                
  			[*]     IP: BOOTP support               
  			[*]     IP: RARP support
Device Drivers  --->
```

问题

```bash
dm9000 dm9000: read wrong id 0x01010101
```

修改dm9000驱动, 在复位后需要延时

vim drivers/net/ethernet/davicom/dm9000.c

```
static int dm9000_probe(struct platform_device *pdev)
{
    ...
    iow(db, DM9000_NCR, NCR_MAC_LBK | NCR_RST);
    udelay(150);
	...
}
```



## 根文件系统rootfs

cd busybox-1.22.1/

```
CROSS_COMPILE 	?= /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-
```

make menuconfig

```shell
Busybox Settings  ---> 

​	General Configuration  ---> 

​		[*] Enable options for full-blown desktop systems (NEW) // 取消

​	Build Options  --->    

​		(/home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-) Cross Compiler prefix

​	Installation Options ("make install" behavior)  --->

Linux Module Utilities  ---> 

 	[ ] Simplified modutils                                                           
	[*]   insmod                                              
	[*]   rmmod                                            
	[*]   lsmod                                          
	[ ]     Pretty output (NEW)                
	[*]   modprobe                                 
	[ ]     Blacklist support (NEW)      
	[*]   depmod    	
```

make && make install 

cd ..

mkdir ~/rootfs

cp busybox-1.22.1/_install/* rootfs/ -a 

cp busybox-1.22.1/examples/bootfloppy/etc/ rootfs/ -r 





拷贝虚拟机系统的 passwd、 group、 shadow 到根文件系统 

cp /etc/passwd etc/ 

cp /etc/group etc/ 

cp /etc/shadow etc/ 

把 passwd 文件中的第一行： root:x:0:0:root:/root:/bin/bash 中的/bin/bash，改成/bin/ash
root:x:0:0:root:/root:/bin/ash
因为文件系统的 bin 目录下没有 bash 这个命令，而是用 ash 代替 bash，所以在用用户名密
码登录的时候(如 telnet)，会出现“cannot run /bin/bash: No such file or directory”的错误 

### 创建必要的设备文件 

cd ~/rootfs

mkdir dev

sudo mknod dev/console c 5 1 

sudo mknod dev/null c 1 3 

mkdir mnt proc var tmp sys root lib 

sudo cp /home/flinn/tools/4.5.1/arm-none-linux-gnueabi/sys-root/lib/* lib/ -a

修改 rootfs/etc/inittab 文件 

vi etc/inittab 

```shell
# /etc/inittab
# This is run first except when booting in single-user mode.
::sysinit:/etc/init.d/rcS
# Note below that we prefix the shell commands with a "-" to indicate to the
# shell that it is supposed to be a login shell
# Start an "askfirst" shell on the console (whatever that may be)
::askfirst:-/bin/sh
# Start an "askfirst" shell on /dev/tty2
#tty2::askfirst:-/bin/sh个人 QQ： 809205580 技术交流群： 153530783 个人博客： http://blog.csdn.net/zjhsucceed_329
# Stuff to do before rebooting
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a –r
```

修改 rootfs/ etc/fstab 

vi etc/fstab 

```shell
#device mount-point type options dump fsck order
proc /proc proc defaults 0 0
tmpfs /tmp tmpfs defaults 0 0
sysfs /sys sysfs defaults 0 0
tmpfs /dev tmpfs defaults 0 0
```

修改 rootfs/etc/init.d/rcS 

vi etc/init.d/rcS 

```shell
#!/bin/sh
#This is the first script called by init process
mount -a
mkdir /dev/pts
mount -t devpts devpts /dev/pts
echo /sbin/mdev>/proc/sys/kernel/hotplug
mdev –s
```

修改 rootfs/etc/profile 

vi etc/profile 

```
#!/bin/sh
`hostname zjh`
HOSTNAME=`hostname`
USER=`id -un`
LOGNAME=$USER
HOME=$USER
PS1="[\u@$\h: \W]\# "
PATH=/bin:/sbin:/usr/bin:/usr/sbin
LD_LIBRARY_PATH=/lib:/usr/lib:$LD_LIBRARY_PATH个人 QQ： 809205580 技术交流群： 153530783 个人博客： http://blog.csdn.net/zjhsucceed_329
export PATH LD_LIBRARY_PATH HOSTNAME USER PS1 LOGNAME HOME
alias ll="ls -l"
echo "Processing /etc/profile... "
echo "Done"
```



## NFS

参考 Documentation/filesystems/nfs/nfsroot.txt 给 u-boot 设置传给内核的命令行参数。

```bash
set bootargs root=/dev/nfs nfsroot=192.168.10.136:/home/flinn/smart210-SDK/rootfs ip=192.168.10.123 
console=ttySAC0,115200 
```

nfsroot 指定了服务器的 IP 和根文件系统在服务器的路径， ip 指定了开发板中的 Linux 内核的 IP 

sudo vi /etc/exports 

```bash
/home/flinn/smart210-SDK/rootfs *(rw,sync,no_root_squash)
```

重启nfs

sudo /etc/init.d/nfs-kernel-server restart

挂载失败

```shell
VFS: Unable to mount root fs via NFS, trying floppy.
VFS: Cannot open root device "nfs" or unknown-block(2,0): error -6
Please append a correct "root=" boot option; here are the available partitions:
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(2,0)
CPU: 0 PID: 1 Comm: swapper Not tainted 3.10.79 #6
```



## Nand支持

三星提供的 NAND FLASH 驱动为 drivers/mtd/nand/s3c2410.c，只支持 S3C2410/S3C2440/S3C2412。
我们需要修改它，以支持 s5pv210。
在这个驱动中，根据 CPU 类型来决定如何操作硬件。里面定义了一个枚举类型 

vim  drivers/mtd/nand/s3c2410.c

```c
enum s3c_cpu_type {
        TYPE_S3C2410,
        TYPE_S3C2412,
        TYPE_S3C2440,
    	TYPE_S5PV210,
};
```

用来表示 Cpu 类型，我们需要在里面添加 TYPE_S5PV210。 

还定义了一个结构体变量 s3c24xx_driver_ids，里面列出了当前支持的平台设备，我们需要在里面添
加 s5pv210-nand 

```c
static struct platform_device_id s3c24xx_driver_ids[] = {
        {
                .name           = "s3c2410-nand",
                .driver_data    = TYPE_S3C2410,
        }, {
                .name           = "s3c2440-nand",
                .driver_data    = TYPE_S3C2440,
        }, {
                .name           = "s3c2412-nand",
                .driver_data    = TYPE_S3C2412,
        }, {
                .name           = "s3c6400-nand",
                .driver_data    = TYPE_S3C2412, /* compatible with 2412 */
        },{
                .name           = "s5pv210-nand",
                .driver_data    = TYPE_S5PV210, /* compatible with 210TYPE_S5PV210 */
        },
        { }
};
```

三星公用的 NAND 平台设备在 arch/arm/plat-samsung/devs.c 中定义 

vim arch/arm/plat-samsung/devs.c  

```c
#ifdef CONFIG_S3C_DEV_NAND
static struct resource s3c_nand_resource[] = {
        [0] = DEFINE_RES_MEM(S3C_PA_NAND, SZ_1M),
};

struct platform_device s3c_device_nand = {
        .name           = "s3c2410-nand",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(s3c_nand_resource),
        .resource       = s3c_nand_resource,
};
```

这里的 nand 设备名称默认为 s3c2410-nand，我们需要在 mach-smdkv210.c 中的
smdkv210_machine_init 函数中调用一个函数来设置这个 name 属性，把它设置为 s5pv210-nand，这
样驱动中就会得到 CPU 类型为 TYPE_S5PV210。 

s3c_nand_setname("s5pv210-nand"); 

首先在 arch/arm/plat-samsung/include/plat/regs-nand.h 针对 s5pv210 添加 NAND 寄存器索引 

vim arch/arm/plat-samsung/include/plat/regs-nand.h

```c
#define S5PV210_NFCONF  		S3C2410_NFREG(0x00)
#define S5PV210_NFCONT  		S3C2410_NFREG(0x04)
#define S5PV210_NFCMD   		S3C2410_NFREG(0x08)
#define S5PV210_NFADDR  		S3C2410_NFREG(0x0C)
#define S5PV210_NFDATA  		S3C2410_NFREG(0x10)
#define S5PV210_NFSTAT  		S3C2410_NFREG(0x28)

#define S5PV210_NFECC			S3C2410_NFREG(0x20000)
#define S5PV210_NFECCCONF  		S3C2410_NFREG(0x00) + (S5PV210_NFECC)
#define S5PV210_NFECCCONT  		S3C2410_NFREG(0x20) + (S5PV210_NFECC)
#define S5PV210_NFECCSTAT		S3C2410_NFREG(0x30) + (S5PV210_NFECC)
#define S5PV210_NFECCSECSTAT	S3C2410_NFREG(0x40) + (S5PV210_NFECC)
#define S5PV210_NFECCPRGECC0	S3C2410_NFREG(0x90) + (S5PV210_NFECC)
#define S5PV210_NFECCPRGECC1	S3C2410_NFREG(0x94) + (S5PV210_NFECC)
#define S5PV210_NFECCPRGECC2	S3C2410_NFREG(0x98) + (S5PV210_NFECC)
#define S5PV210_NFECCPRGECC3	S3C2410_NFREG(0x9C) + (S5PV210_NFECC)
#define S5PV210_NFECCERL0		S3C2410_NFREG(0xC0) + (S5PV210_NFECC)
#define S5PV210_NFECCERL1		S3C2410_NFREG(0xC4) + (S5PV210_NFECC)
#define S5PV210_NFECCERL2		S3C2410_NFREG(0xC8) + (S5PV210_NFECC)
#define S5PV210_NFECCERL3		S3C2410_NFREG(0xCC) + (S5PV210_NFECC)
#define S5PV210_NFECCERP0		S3C2410_NFREG(0xF0) + (S5PV210_NFECC)
#define S5PV210_NFECCERP1		S3C2410_NFREG(0xF4) + (S5PV210_NFECC)
```

vim  drivers/mtd/nand/s3c2410.c

```c
enum s3c_cpu_type {
	TYPE_S3C2410,
	TYPE_S3C2412,
	TYPE_S3C2440,
	/* add by Flinn */
	TYPE_S5PV210,
};
...
static int s3c2410_nand_setrate(struct s3c2410_nand_info *info)
{
    ...
        /* add by Flinn */
	case TYPE_S5PV210:
		mask = (0xF << 12) | (0xF << 8) | (0xF << 4);

		set = (tacls + 1) << 12;
		set |= (twrph0 - 1 + 1) << 8;
		set |= (twrph1 - 1 + 1) << 4;
		break;
}

static int s3c2410_nand_inithw(struct s3c2410_nand_info *info)
{
    ...
    unsigned long uninitialized_var(cfg);	/* add by Flinn */
    case TYPE_S5PV210:
		cfg = readl(info->regs + S5PV210_NFCONF);
		cfg &= ~(0x1 << 3);	/* SLC NAND Flash */
		cfg &= ~(0x1 << 2);	/* 2KBytes/Page */
		cfg |= (0x1 << 1);	/* 5 address cycle */
		writel(cfg, info->regs + S5PV210_NFCONF);
		/* Disable chip select and Enable NAND Flash Controller */
		writel((0x1 << 1) | (0x1 << 0), info->regs + S5PV210_NFCONT);
		break;
}
/* add by Flinn */
static void s5pv210_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{
	struct s3c2410_nand_info *info = s3c2410_nand_mtd_toinfo(mtd);

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		writeb(cmd, info->regs + S5PV210_NFCMD);
	else
		writeb(cmd, info->regs + S5PV210_NFADDR);
}

/* add by Flinn */
static int s5pv210_nand_devready(struct mtd_info *mtd)
{
	struct s3c2410_nand_info *info = s3c2410_nand_mtd_toinfo(mtd);
	return readb(info->regs + S5PV210_NFSTAT) & 0x1;
}

static void s3c2410_nand_init_chip(struct s3c2410_nand_info *info,
				   struct s3c2410_nand_mtd *nmtd,
				   struct s3c2410_nand_set *set)
{
    ...
    /* add by Flinn */
	case TYPE_S5PV210:
		chip->IO_ADDR_W = regs + S5PV210_NFDATA;
		info->sel_reg   = regs + S5PV210_NFCONT;
		info->sel_bit	= (1 << 1);
		chip->cmd_ctrl  = s5pv210_nand_hwcontrol;
		chip->dev_ready = s5pv210_nand_devready;
		break;
}

static struct platform_device_id s3c24xx_driver_ids[] =
{
    ...
        {	/* add by Flinn */
		.name		= "s5pv210-nand",
		.driver_data	= TYPE_S5PV210,
	},
}
```

我们要让 drivers/mtd/nand/s3c2410.c 被编译进内核，需要修改 drivers/mtd/nand/Kconfig 

vim  drivers/mtd/nand/Kconfig

```shell
config MTD_NAND_S3C2410
        tristate "NAND Flash support for Samsung S3C SoCs"
        depends on ARCH_S3C24XX || ARCH_S3C64XX || ARCH_S5PV210
        help
          This enables the NAND flash controller on the S3C24xx and S3C64xx
          SoCs

          No board specific support is done by this driver, each board
          must advertise a platform_device for the driver to attach.

```

在 drivers/mtd/nand/s3c2410.c 中通过名称“nand”获得时钟
info->clk = devm_clk_get(&pdev->dev, "nand");
s5pv210 的时钟定义在 arch/arm/mach-s5pv210/clock.c 中，这里面没有针对 nand 定义时钟，因此需
要添加，在 init_clocks_off 数组里面添加（参考 s5pv210 手册时钟章节） 

vim arch/arm/mach-s5pv210/clock.c 

```c
static struct clk init_clocks_off[] = 
{
    ...
    {
                .name           = "spdif",
                .parent         = &clk_p,
                .enable         = s5pv210_clk_ip3_ctrl,
                .ctrlbit        = (1 << 0),
        },{ // add by Flinn
                .name           = "nand",
                .parent         = &clk_hclk_psys.clk,
                .enable         = s5pv210_clk_ip1_ctrl,
                .ctrlbit        = (1 << 24),
        },
}
```

在 arch/arm/plat-samsung/devs.c 中定义的 nand 平台设备如下： 

```c
#ifdef CONFIG_S3C_DEV_NAND
static struct resource s3c_nand_resource[] = {
        [0] = DEFINE_RES_MEM(S3C_PA_NAND, SZ_1M),
};

struct platform_device s3c_device_nand = {
        .name           = "s3c2410-nand",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(s3c_nand_resource),
        .resource       = s3c_nand_resource,
};
```

这里用了一个宏 CONFIG_S3C_DEV_NAND，这个宏默认没有选中，因此需要修改
arch/arm/mach-s5pv210/Kconfig 

vim arch/arm/mach-s5pv210/Kconfig 

```c
config MACH_SMDKV210
	...
	select SAMSUNG_DEV_TS
    select S3C_DEV_NAND
```

在后面添加了 select S3C_DEV_NAND 



另外在 nand 平台设备中使用了 S3C_PA_NAND，这个在 s5pv210 中也没定义，需要在
arch/arm/mach-s5pv210/include/mach/map.h 中定义 

vim arch/arm/mach-s5pv210/include/mach/map.h

```c
/* add by Flinn */
#define S5PV210_PA_NAND                 0xB0E00000
#define S3C_PA_NAND                     S5PV210_PA_NAND
```

在 arch/arm/mach-s5pv210/mach-smdkv210.c 添加头文件 

vim arch/arm/mach-s5pv210/mach-smdkv210.c

```c
/* add by Flinn */
#include <plat/nand-core.h>
#include <linux/platform_data/mtd-nand-s3c2410.h>
#include <linux/mtd/partitions.h>
```

定义 nand 平台相关的数据 

```c
/* nand info (add by Flinn) */
static struct mtd_partition smdk_default_nand_part[] = {
	[0] = {
		.name	= "bootloader",
		.size	= SZ_256K,
		.offset	= 0,
	},
	[1] = {
		.name	= "params",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_128K,
	},
	[2] = {
		.name	= "log",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_2M,
	},
	[3] = {
		.name	= "kernel",
		.offset	= MTDPART_OFS_APPEND,
		.size	= SZ_1M + SZ_2M,
	},

	[4] = {
		.name	= "rootfs",
		.offset = MTDPART_OFS_APPEND,
		.size	= MTDPART_SIZ_FULL,
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
static struct platform_device *smdkv210_devices[] __initdata =
{
    ...
    &s3c_device_nand,	/* add by Flinn */
}
```

这里定义了 nand 的分区，要和 u-boot 中的分区一致，以及时序参数 

在 smdkv210_machine_init 函数中设置 nand 

```c
static void __init smdkv210_machine_init(void)
{
    ...
   	/* add by Flinn */
	s3c_nand_setname("s5pv210-nand");
	s3c_nand_set_platdata(&smdk_nand_info);
	s5pv210_nand_gpio_cfg();
}
```



配置内核支持 nand
Device Drivers --->
	<y> Memory Technology Device (MTD) support --->
		<y> Caching block device access to MTD devices
		<y> NAND Device Support ---> 

执行 make uImage 编译内核，下载到内存运行 

烧写内核到nand，并启动

nand erase.part kernel

tftp 20000000 uImage

nand write  20000000 260000 $filesize

启动参数bootcmd=nand read 20000000 260000 300000;bootm 20000000

或者

```shell
set bootcmd "nand read 20000000 260000 300000;bootm 20000000"
```



如果前面烧写了u-boot到nand，那么此时切换到nand启动，可以启动内核。

```shell
U-Boot 2014.04 (Feb 20 2019 - 10:43:40) for SMART210

CPU:    S5PC110@1000MHz
Board:  SMART210
DRAM:  512 MiB
WARNING: Caches not enabled
NAND:  1024 MiB
In:    serial
Out:   serial
Err:   serial
Net:   dm9000
Hit any key to stop autoboot:  0

NAND read: device 0 offset 0x260000, size 0x300000
 3145728 bytes read: OK
## Booting kernel from Legacy Image at 20000000 ...
   Image Name:   Linux-3.10.79
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    1954736 Bytes = 1.9 MiB
   Load Address: 20008000
   Entry Point:  20008000
   Verifying Checksum ... OK
   Loading Kernel Image ... OK
Using machid 0x998 from environment

Starting kernel ...

Uncompressing Linux... done, booting the kernel.
Booting Linux on physical CPU 0x0
Linux version 3.10.79 (flinn@flinn) (gcc version 4.5.1 (ctng-1.8.1-FA) ) #7 PREEMPT Wed Feb 20 16:02:27 CST 2019
...
```

