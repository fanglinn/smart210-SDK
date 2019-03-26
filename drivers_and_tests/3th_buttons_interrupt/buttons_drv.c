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
#include <linux/sched.h>

#include <asm/io.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <linux/irq.h>
#include <plat/gpio-cfg.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/wait.h>


/*
* GPIO OF KEY FOR SMART210
*  K1  - GPH2-0  - EXINT16
*  K2  - GPH2-1  - EXINT17
*  K3  - GPH2-2  - EXINT18
*  K4  - GPH2-3  - EXINT19
*  K5  - GPH3-0  - EXINT24 
*  K6  - GPH3-1  - EXINT25
*  K7  - GPH3-2  - EXINT26
*  K8  - GPH3-3  - EXINT27
*
*  LOW MEANS PRESSED
*
*/

struct buttons_device
{
	int major;
	char device_name[32];
	struct cdev *cdev;
	dev_t dev;
	struct class *buttons_class;
};

static struct buttons_device *btn_dev;


struct key_data
{
	unsigned int pins;
	unsigned int flags;
	unsigned char key_value;
	int irq;
	char *name;
};

static const struct key_data keys[] = 
{
	{S5PV210_GPH2(0), IRQF_TRIGGER_FALLING, 0x1, IRQ_EINT(16), "K1"},
	{S5PV210_GPH2(1), IRQF_TRIGGER_FALLING, 0x2, IRQ_EINT(17), "K2"},
	{S5PV210_GPH2(2), IRQF_TRIGGER_FALLING, 0x3, IRQ_EINT(18), "K3"},
	{S5PV210_GPH2(3), IRQF_TRIGGER_FALLING, 0x4, IRQ_EINT(19), "K4"},
	{S5PV210_GPH3(0), IRQF_TRIGGER_FALLING, 0x5, IRQ_EINT(24), "K5"},
	{S5PV210_GPH3(1), IRQF_TRIGGER_FALLING, 0x6, IRQ_EINT(25), "K6"},
	{S5PV210_GPH3(2), IRQF_TRIGGER_FALLING, 0x7, IRQ_EINT(26), "K7"},
	{S5PV210_GPH3(3), IRQF_TRIGGER_FALLING, 0x8, IRQ_EINT(27), "K8"},
};




static DECLARE_WAIT_QUEUE_HEAD(button_waitq);   

static int press_flag = 0;
static unsigned char key_val = 0;;

irqreturn_t btn_handler(int irq, void *dev_id)
{
	struct key_data *p = dev_id;

	wake_up_interruptible(&button_waitq);

	key_val = gpio_get_value(p->pins);
	if(!key_val)
	{
		key_val = p->key_value + 0x80;
	}
	
	press_flag = 1;
	
	return IRQ_RETVAL(IRQ_HANDLED);
}


static int btn_open (struct inode *inode, struct file *pfile)
{
	int i;
	printk(KERN_NOTICE "open .\n");
	
	for(i = 0; i < ARRAY_SIZE(keys); i++)
	{
		if(request_irq(keys[i].irq,btn_handler,keys[i].flags,keys[i].name,(void *)&keys[i]))
			break;
	}
	
	return 0;
}

static int btn_close (struct inode *inode, struct file *pfile)
{
	int i;

	printk(KERN_NOTICE "close .\n");
	
	for(i = ARRAY_SIZE(keys) - 1; i > 0; i--)
	{
		irq_free_desc(keys[i].irq);
	}
	
	return 0;
}

static ssize_t btn_read (struct file *pfile, char __user *usrbuf, size_t len, loff_t *off)
{
	wait_event_interruptible(button_waitq,press_flag);

	if(copy_to_user(usrbuf,&key_val,1))
		return -EFAULT;

	
	press_flag = 0;
	
	return 0;
}

static const struct file_operations btn_fops = 
{
	.owner = THIS_MODULE,
	.open = btn_open,
	.release = btn_close,
	.read = btn_read,
};

static int __init buttons_drv_init(void)
{
	int ret ;

	printk(KERN_NOTICE "init .\n");

	btn_dev = kmalloc(sizeof(struct buttons_device),GFP_KERNEL);
	if(!btn_dev)
	{
		printk(KERN_ERR "kmalloc error.\n");
		return -ENOMEM;
	}

	strcpy(btn_dev->device_name, "mykey");


	if(btn_dev->major)
	{
		btn_dev->dev = MKDEV(btn_dev->major,0);
		ret =register_chrdev_region(btn_dev->dev,1,btn_dev->device_name);
	}
	else
	{
		ret = alloc_chrdev_region(&btn_dev->dev,0,1,btn_dev->device_name);
		btn_dev->major = MAJOR(btn_dev->dev);
	}

	if(ret < 0)
	{
		printk(KERN_ERR "chrdev register error.\n");
		kfree(btn_dev);
		return -EFAULT;
	}

	btn_dev->cdev = cdev_alloc();
	if(!btn_dev->cdev)
	{
		unregister_chrdev_region(btn_dev->dev,1);
		kfree(btn_dev);
		return -EFAULT;
	}
	cdev_init(btn_dev->cdev,&btn_fops);
	btn_dev->cdev->owner = THIS_MODULE;
	btn_dev->cdev->ops = &btn_fops;
	cdev_add(btn_dev->cdev,btn_dev->dev,1);

	btn_dev->buttons_class = class_create(THIS_MODULE ,btn_dev->device_name);
	device_create(btn_dev->buttons_class,NULL,btn_dev->dev,NULL,btn_dev->device_name);
	
	return 0;
}

static void __exit buttons_drv_exit(void)
{
	printk(KERN_NOTICE "exit .\n");	

	device_destroy(btn_dev->buttons_class,btn_dev->dev);
	class_destroy(btn_dev->buttons_class);

	cdev_del(btn_dev->cdev);
	unregister_chrdev_region(btn_dev->dev,1);
	kfree(btn_dev);
}

module_init(buttons_drv_init);
module_exit(buttons_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flinn <flinn682@foxmail.com>");
