KERN_DIR = /home/flinn/smart210-SDK/linux-3.10.79

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order Module.symvers

obj-m	+= led_drv.o
