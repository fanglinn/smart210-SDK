# smart210-SDK
smart210-SDK



## u-boot

### 编译：

```bash
make smart210_config
make all
```

### 烧录到SD卡：

```shell
#!/bin/bash
sudo dd iflag=sync oflag=sync if=spl/smart210-spl.bin of=/dev/sdc seek=1
sudo dd iflag=sync oflag=sync if=u-boot.bin of=/dev/sdc seek=32
sync
echo "done."
```

### 烧录到NAND

```bash
nand erase.part bootloader
tftp 20000000 smart210-spl.bin
nand write 20000000 0 $filesize
tftp 20000000 u-boot.bin
nand write 20000000 4000 $filesize
```



### 修改分区：

vim include/configs/smart210.h

```c
#define MTDPARTS_DEFAULT        "mtdparts=s5p-nand:256k(bootloader)"\
                                ",128k(device_tree)"\
                                ",128k(params)"\
                                ",5m(kernel)"\
                                ",-(rootfs)"

#define CONFIG_ENV_SIZE                 (128 << 10)     /* 128KiB, 0x20000 */
#define CONFIG_ENV_ADDR                 (384 << 10)     /* 384KiB(u-boot + device_tree), 0x60000 */
#define CONFIG_ENV_OFFSET               (384 << 10)     /* 384KiB(u-boot + device_tree), 0x60000 */
```

修改启动参数：

```
#define CONFIG_BOOTCOMMAND	"nand read.jffs2 0x30007FC0 0x80000 0x500000;  bootm 0x30007FC0 "
```

set bootargs noinitrd root=/dev/mtdblock4 rw init=/linuxrc console=ttySAC0,115200

## kernel

### 编译：

```bash
make s5pv210_defconfig
make uImage
```

### 烧录

```bash
nand erase.part kernel
tftp 20000000 uImage
nand write.yaffs  20000000 80000 $filesize
nand write  20000000 80000 $filesize
or
nand erase.part kernel;tftp 20000000 uImage;nand write  20000000 80000 $filesize
```



### 修改分区

对应与bootloader

vim arch/arm/mach-s5pv210/mach-smdkv210.c

```c
/* nand info (add by Flinn) */
static struct mtd_partition smdk_default_nand_part[] = {
        [0] = {
                .name   = "bootloader",
                .size   = SZ_256K,
                .offset = 0,
        },
        [1] = {
                .name   = "device_tree",
                .offset = MTDPART_OFS_APPEND,
                .size   = SZ_128K,
        },
        [2] = {
                .name   = "params",
                .offset = MTDPART_OFS_APPEND,
                .size   = SZ_128K,
        },
        [3] = {
                .name   = "kernel",
                .offset = MTDPART_OFS_APPEND,
                .size   = SZ_1M + SZ_4M,
        },

        [4] = {
                .name   = "rootfs",
                .offset = MTDPART_OFS_APPEND,
                .size   = MTDPART_SIZ_FULL,
        }
};
```





## rootfs

注意：

busybox不要采用高版本，使用busybox-1.7.0为宜。

### 烧录

```bash
tftp 20000000 rootfs.yaffs2
nand erase.part rootfs
nand write.yaffs 20000000 0x580000 $filesize
```

jffs2

```bash
tftp 20000000 rootfs.jffs2
nand erase.part rootfs
nand write.jffs2 20000000 580000 $filesize
set bootargs console=ttySAC0,115200 root=/dev/mtdblock4 rootfstype=jffs2
```



### nfs

```bash
set bootargs noinitrd root=/dev/nfs nfsroot=192.168.1.104:/home/flinn/work/rootfs/fs_mini_mdev  ip=192.168.1.123:192.168.1.104:192.168.1.1:255.255.255.0::eth0:off init=/linuxrc console=ttySAC0，115200
```

