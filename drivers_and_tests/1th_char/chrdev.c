#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

static int chrdev_major = 0;
static int chrdev_minor = 0;
static char *dev_name = "chrdev";
static dev_t chr_dev;
static struct cdev *cdev;

static int chrdev_open (struct inode *inode, struct file *file)
{
	printk(KERN_NOTICE "chrdev open .\n");
	return 0;
}

static int chrdev_close (struct inode *inode, struct file *file)
{
	printk(KERN_NOTICE "chrdev close.\n");
	return 0;
}

static struct file_operations chrdev_fops = 
{
	.owner = THIS_MODULE,
	.open = chrdev_open,
	.release = chrdev_close,
};

static int __init chrdev_init(void)
{
	int ret;
	
	printk(KERN_NOTICE "chrdev init.\n");
	if(chrdev_major)
	{
		chr_dev = MKDEV(chrdev_major,chrdev_minor);
		ret = register_chrdev_region(chr_dev,1,dev_name);
	}
	else
	{
		ret = alloc_chrdev_region(&chr_dev,0,1,dev_name);
		chrdev_major = MAJOR(chr_dev);
	}
	if(ret < 0)
	{
		printk(KERN_ERR "chrdev register err .\n");
		return ret;
	}

	cdev = cdev_alloc();
	if(cdev == NULL)
	{
		printk(KERN_ERR "chrdev cdev alloc err .\n");
		unregister_chrdev_region(chr_dev,1);
		return -1;
	}

	cdev_init(cdev,&chrdev_fops);
	cdev->owner = THIS_MODULE;
	cdev->ops = &chrdev_fops;
	ret = cdev_add(cdev,chr_dev,1);
	if(ret < 0)
	{
		printk(KERN_NOTICE "chrdev cdev add fail .\n");
		cdev_del(cdev);
		unregister_chrdev_region(chr_dev,1);
		return ret;
	}

	
	return 0;
}


static void __exit chrdev_exit(void)
{
	printk(KERN_NOTICE "chrdev exit.\n");
	cdev_del(cdev);
	unregister_chrdev_region(chr_dev,1);
}



module_init(chrdev_init);
module_exit(chrdev_exit);
MODULE_AUTHOR("Flinn <Flinn682@foxmail.com");
MODULE_LICENSE("GPL");
