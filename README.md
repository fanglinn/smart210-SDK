# smart210-SDK
smart210-SDK

# 编译

## u-boot

```
make smart210_config
make all
```



## kernel

```bash
make s5pv210_defconfig
make menuconfig
	System Type  ---> 
    	(0) S3C UART to use for low-level messages
    	S5PC110 Machines  --->
    		// remove all
    	S5PV210 Machines  --->
    		[*] SMDKV210
    ## 支持Nand
   	Device Drivers --->
	<y> Memory Technology Device (MTD) support --->
		<y> Caching block device access to MTD devices
		<y> NAND Device Support ---> 
			<*>   NAND Flash support for Samsung S3C SoCs
	## NFS
	[*] Networking support  --->
	     Networking options  --->
	     	<*> Packet socket                                                                     
			<*>   Packet: sockets monitoring interface                                           
  			<*> Unix domain sockets                                                               
  			<*>   UNIX: socket monitoring interface
  			[*] TCP/IP networking   
  			[*]   IP: multicasting                                                 
      		[*]   IP: advanced router                                 
  			[*]   IP: kernel level autoconfiguration
  			[*]     IP: DHCP support                
  			[*]     IP: BOOTP support               
  			[*]     IP: RARP support
	Device Drivers  --->
		[*] Network device support --->
        	[*] Ethernet driver support (NEW) --->
        		<*> DM9000 support  #只保留dm9000，去掉其他
    File systems --->
        [*] Network File Systems (NEW) --->
        	<*> NFS client support
        	[*] Root file system on NFS
	## JFFS2
	File systems  ---> 
		[*] Miscellaneous filesystems  --->
		<*>   Journalling Flash File System v2 (JFFS2) support
make uImage
```

## rootfs

```bash
mkfs.jffs2 -n -s 2048 -e 128KiB -d rootfs -o rootfs.jffs2	  
mkyaffs2image rootfs rootfs.yaffs2
```



# 分区

```bash
device nand0 <s5p-nand>, # parts = 4
 #: name                size            offset          mask_flags
 0: bootloader          0x00040000      0x00000000      0              # 256K
 1: params              0x00020000      0x00040000      0              # 128K
 2: kernel              0x00300000      0x00060000      0              # 3M
 3: rootfs              0x3fca0000      0x00360000      0              # ~
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

```bash
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/u-boot-all.bin
nand erase.part bootloader
nand write.yaffs 20000000 0 $filesize
```



### kernel

```bash
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/uImage
nand erase.part kernel
nand write.yaffs 20000000 60000 $filesize
```



### rootfs

```bash
rootfs:
fs-yaffs2:
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/rootfs.yaffs2
nand erase.part rootfs
nand write.yaffs 20000000 360000 139b9c0

fs-jffs2
nfs 20000000 192.168.10.136:/home/flinn/smart210-SDK/bin/rootfs.jffs2
nand erase.part rootfs
nand write.jffs2 20000000 360000 $filesize
set bootargs console=ttySAC0,115200 root=/dev/mtdblock3 rootfstype=jffs2
```



#### 