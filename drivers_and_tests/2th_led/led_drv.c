/*
* linux-3.10.79
* arm-linux-gcc-4.5.1
*/


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <linux/types.h>
#include <linux/uaccess.h>

/*
* led resource :
*   led1 - GPJ2_0
*   led2 - GPJ2_1
*   led3 - GPJ2_2
*   led4 - GPJ2_3
*
* GPJ2CON - 0xE0200280 
*  
* GPJ2CON[0] [3:0] 
*      0000 = Input  
*      0001 = Output 
*      0010 = MSM_DATA[0] 
*      0011 = KP_COL[1] 
*      0100 = CF_DATA[0] 
*      0101 = MHL_D7 
*      0110 ~ 1110 = Reserved 
*      1111 = GPJ2_INT[0]
* ---
* GPJ2DAT - 0xE0200284
*/

#define LED_BASE_ADDR		0xE0200280

struct led_device 
{
	int major;
	char device_name[32];
	dev_t dev;
	struct cdev *cdev;
	struct class *led_class;
};


static struct led_device *led_dev;

static volatile unsigned long *gpjcon;
static volatile unsigned long *gpjdat;



static int led_dev_open (struct inode *inode, struct file *pfile)
{
	printk(KERN_INFO "open \n");
	
	/* set GPJ2-1,2,3 as output */
	*gpjcon |= (1 << 4 * 1) + (1 << 4 * 2) + (1 << 4 * 3);
	
	return 0;
}


static int led_dev_close (struct inode *inode, struct file *pfile)
{
	printk(KERN_INFO "close \n");
	
	//*gpjcon &= ~((1 << 4 * 1) + (1 << 4 * 2) + (1 << 4 * 3));
	
	return 0;
}


static ssize_t led_dev_write (struct file *pfile, const char __user *usrbuf, size_t len, loff_t *offset )
{
	int revbuf[8];
	int cmd, opt;

	printk(KERN_INFO "write \n");	
	if(	copy_from_user(revbuf,usrbuf,8))
	{
		return -EFAULT;
	}

	cmd = revbuf[0];
	opt = revbuf[1];
	//opt = opt & 0xf;

	printk(KERN_NOTICE "cmd : %d opt : %d \n", cmd, opt);

	if(cmd == 0)    // close
	{	
		*gpjdat |= (1 << opt);
	}
	else if(1 == cmd)  // open
	{
		*gpjdat &= ~(1 << opt);
	}
	else
	{
		// do nothing .
	}
	return 0;
}


const struct file_operations led_fops = 
{
	.owner = THIS_MODULE,
	.open = led_dev_open,
	.release = led_dev_close,
	.write = led_dev_write,
};



static int __init led_drv_init(void)
{
	int ret ;

	printk(KERN_INFO "init. \n");
	led_dev = kmalloc(sizeof(struct led_device), GFP_KERNEL);
	if(!led_dev)
	{
		return -ENOMEM;
	}

	strcpy(led_dev->device_name, "myled");

	if(led_dev->major)
	{
		led_dev->dev = MKDEV(led_dev->major,0);
		ret = register_chrdev_region(led_dev->dev,1,led_dev->device_name);
	}
	else
	{
		ret = alloc_chrdev_region(&led_dev->dev,1,1,led_dev->device_name);
		led_dev->major = MAJOR(led_dev->dev);
	}

	if(ret < 0)
	{
		printk(KERN_NOTICE "chrdev register fail .\n");
		kfree(led_dev);
		return -1;
	}

	led_dev->cdev = cdev_alloc();
	if(!led_dev->cdev)
	{
		printk(KERN_NOTICE "chrdev cdev alloc fail .\n");
		unregister_chrdev_region(led_dev->dev,1);
		kfree(led_dev);
		return -1;
	}
	cdev_init(led_dev->cdev, &led_fops);
	led_dev->cdev->owner = THIS_MODULE;
	led_dev->cdev->ops = &led_fops;
	cdev_add(led_dev->cdev,led_dev->dev,1);

	led_dev->led_class = class_create(THIS_MODULE , led_dev->device_name);

	device_create(led_dev->led_class,NULL,led_dev->dev,NULL,led_dev->device_name);

	gpjcon = (volatile unsigned long *)ioremap(LED_BASE_ADDR,0x10);
	gpjdat = gpjcon + 1;
	
	return 0;
}

static void __exit led_drv_exit(void)
{
	printk(KERN_INFO "exit. \n");
	iounmap(gpjcon);
	
	device_destroy(led_dev->led_class,led_dev->dev);
	class_destroy(led_dev->led_class);
	
	cdev_del(led_dev->cdev);
	unregister_chrdev_region(led_dev->dev,1);
	kfree(led_dev);
}


module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flinn <flinn682@foxmail.com>");
