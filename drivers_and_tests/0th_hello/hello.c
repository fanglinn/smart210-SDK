#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>



static int __init hello_init(void)
{
	printk(KERN_NOTICE "hello init.\n");
	return 0;
}


static void __exit hello_exit(void)
{
	printk(KERN_NOTICE "hello exit.\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_AUTHOR("Flinn <Flinn682@foxmail.com");
MODULE_LICENSE("GPL");
