#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <sound/soc.h>

static struct platform_device *smart210_wm8960_snd_device;

static int smart210_wm8960_startup(struct snd_pcm_substream *substream)
{
	return 0;
}


static void smart210_wm8960_shutdown(struct snd_pcm_substream *substream)
{
	return 0;
}


static int smart210_wm8960_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	return 0;
}


static struct snd_soc_ops smart210_wm8960_ops = {
	.startup = smart210_wm8960_startup,
	.shutdown = smart210_wm8960_shutdown,
	.hw_params = smart210_wm8960_hw_params,
};


static struct snd_soc_dai_link smart210_wm8960_dai_link = {
	.name = "smart210",
	.stream_name = "WM8960 HiFi",
	.codec_name = "wm8960-codec.0-001a",
	.codec_dai_name = "wm8960-hifi",
	.cpu_dai_name = "samsung-i2s.0",
	.ops = &smart210_wm8960_ops,
	.platform_name	= "samsung-audio",
};


static struct snd_soc_card snd_soc_smart210 = {
	.name = "smart210_wm8960",
	.owner = THIS_MODULE,
	.dai_link = &smart210_wm8960_dai_link,
	.num_links = 1,
};


static int smart210_wm8960_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s called.", __func__);

	smart210_wm8960_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smart210_wm8960_snd_device) {
		printk(KERN_ERR "smart210_wm8960 SoC Audio: "
		       "Unable to register\n");
		return -ENOMEM;
	}

	platform_set_drvdata(smart210_wm8960_snd_device,
			     &snd_soc_smart210);

	ret = platform_device_add(smart210_wm8960_snd_device);
	if (ret) {
		printk(KERN_ERR "smart210_wm8960 SoC Audio: Unable to add\n");
		platform_device_put(smart210_wm8960_snd_device);
	}

	return ret;

	return 0;	
}


static int smart210_wm8960_remove(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s called.", __func__);

	platform_device_unregister(smart210_wm8960_snd_device);

	return 0;
}


static struct platform_driver smart210_wm8960_drv = {
	.probe  = smart210_wm8960_probe,
	.remove = smart210_wm8960_remove,
	.driver = {
		.name = "smart210_wm8960",
		.owner = THIS_MODULE,
	},

};

static int smart210_wm8960_init(void)
{
	int ret = 0;
	pr_info("%s called.", __func__);

	ret = platform_driver_register(&smart210_wm8960_drv);
	if(ret){
		pr_err("smart210_wm8960_drv register err.\n");
		return ret;
	}
	
	return 0;
}

static void smart210_wm8960_exit(void)
{
	pr_info("%s called.", __func__);
	platform_driver_unregister(&smart210_wm8960_drv);
}


module_init(smart210_wm8960_init);
module_exit(smart210_wm8960_exit);

MODULE_DESCRIPTION("smart210 ALSA SoC audio driver");
MODULE_LICENSE("GPL");
