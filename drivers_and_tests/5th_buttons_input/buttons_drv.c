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

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>


/*
* GPIO OF KEY FOR SMART210
*  K1  - GPH2-0  - EXINT16   'l'
*  K2  - GPH2-1  - EXINT17   's'
*  K3  - GPH2-2  - EXINT18   
*  K4  - GPH2-3  - EXINT19   'p'
*  K5  - GPH3-0  - EXINT24   's'
*  K6  - GPH3-1  - EXINT25
*  K7  - GPH3-2  - EXINT26
*  K8  - GPH3-3  - EXINT27   '\n'
*
*  LOW MEANS PRESSED
*
*/


struct btn_data
{
	unsigned int pin;
	unsigned int flags;
	int irq;
	unsigned char key_val;
	char *name;
};

static struct input_dev *input;

static const struct btn_data btn_res[] = 
{	
	{S5PV210_GPH2(0), IRQF_TRIGGER_FALLING, IRQ_EINT(16),KEY_L, "K1"},
	{S5PV210_GPH2(1), IRQF_TRIGGER_FALLING, IRQ_EINT(17),KEY_S, "K2"},
	{S5PV210_GPH2(2), IRQF_TRIGGER_FALLING, IRQ_EINT(18),KEY_P, "K3"},
	{S5PV210_GPH2(3), IRQF_TRIGGER_FALLING, IRQ_EINT(19),KEY_W, "K4"},
	{S5PV210_GPH3(0), IRQF_TRIGGER_FALLING, IRQ_EINT(24),KEY_T, "K5"},
	{S5PV210_GPH3(1), IRQF_TRIGGER_FALLING, IRQ_EINT(25),KEY_D, "K6"},
	{S5PV210_GPH3(2), IRQF_TRIGGER_FALLING, IRQ_EINT(26),KEY_C, "K7"},
	{S5PV210_GPH3(3), IRQF_TRIGGER_FALLING, IRQ_EINT(27),KEY_ENTER, "K8"},	
};


irqreturn_t btn_handler(int irq, void *dev_id)
{
	unsigned int value;
	struct btn_data *pdata = (struct btn_data*)dev_id;

	value = gpio_get_value(pdata->pin);
	if(!value)
	{
		input_event(input, EV_KEY, pdata->key_val, 1);
		input_sync(input);
	}
	
	return IRQ_RETVAL(IRQ_HANDLED);
}

static int __init buttons_drv_init(void)
{
	int i, error; //,index;

	printk(KERN_NOTICE "init.\n");

	input = input_allocate_device();
	
	if (!input) {
			printk(KERN_ERR "failed to allocate state\n");
			input_free_device(input);
			return -ENOMEM;
	}

	input->name = "mykey";
	input->phys = "gpio-keys/input0";
	input->uniq = "20190326";

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0002;
	input->id.product = 0x0002;
	input->id.version = 0x0102;

	set_bit(EV_SYN,input->evbit);
	set_bit(EV_KEY,input->evbit);
	set_bit(EV_REP, input->evbit);

	for(i = 0; i < ARRAY_SIZE(btn_res); i++)
	{
		set_bit(btn_res[i].key_val,input->keybit);
	}
	
	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register input device, error: %d\n",
			error);
		input_free_device(input);
		return error;
	}

	
	for(i = 0; i < ARRAY_SIZE(btn_res); i++)
	{
		//index = gpio_to_irq(btn_res[i].pin);
		error = request_irq(btn_res[i].irq , btn_handler,btn_res[i].flags,btn_res[i].name,(void *)&btn_res[i]);
		if(error)
			break;
	}

	return 0;
}

static void __exit buttons_drv_exit(void)
{
	int i;

	printk(KERN_NOTICE "exit.\n");
	
	for (i = ARRAY_SIZE(btn_res) - 1 ; i > 0; i--)
	{
		free_irq(btn_res[i].irq ,(void *)&btn_res[i]);
	}
	
	input_unregister_device(input);
}

module_init(buttons_drv_init);
module_exit(buttons_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flinn <flinn682@foxmail.com>");
MODULE_DESCRIPTION(" buttons driver for smart210 base on input-subsystem");
