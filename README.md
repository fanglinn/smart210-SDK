# smart210-SDK
smart210-SDK

# 编译

## u-boot

```
make smart210_config
make all
```



## kernel

```
make s5pv210_defconfig
make uImage
```

# 下载

## tftp

### u-boot

```bash
nand erase.chip
tftp 20000000 smart210-spl.bin
nand write 20000000 0 $filesize

tftp 20000000 u-boot.bin
nand write 20000000 4000 $filesize

```



### kernel

```bash
nand erase.part kernel
tftp 20000000 uImage
nand write  20000000 60000 $filesize
```

set bootcmd "nand read 20000000 60000 300000;bootm 20000000"

OR

```bash
tftp 20000000 uImage
bootm 20000000
```



### rootfs

```
nand erase.part rootfs
tftp 20000000 rootfs.jffs2
nand write  20000000 360000 $filesize
```



## nfs

### u-boot

### kernel

### rootfs

```bash
rootfs:
fs-yaffs2:
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/rootfs.yaffs2
nand erase.part rootfs
nand write.yaffs 20000000 0x560000 139b9c0

fs-jffs2
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/rootfs.jffs2
nand erase.part rootfs
nand write.jffs2 20000000 560000 $filesize
set bootargs console=ttySAC0,115200 root=/dev/mtdblock3 rootfstype=jffs2
```



#### 