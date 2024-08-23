
#include <linux/init.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

struct led_device {
	unsigned gpiod;
	struct led_classdev led;
};

struct leds_priv {
	int led_num;
	struct led_device leds[];
};

static void led_set(struct led_classdev *led_cdev,
		    enum led_brightness brightness)
{
	struct led_device *led = container_of(led_cdev, struct led_device, led);

	// static unsigned long last_print_time = 0;

	// if (time_after(jiffies, last_print_time + msecs_to_jiffies(2000))) {
	// 	mylog("in_softirq:%d\n",in_softirq());
	//     dump_stack();
	//     last_print_time = jiffies;
	// }
	gpio_set_value_cansleep(led->gpiod, brightness);
}

static int led_set_blocking(struct led_classdev *led_cdev,
			    enum led_brightness brightness)
{
	led_set(led_cdev, brightness);
	return 0;
}
static int my_platform_led_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int count;
	int value;
	enum led_default_state default_state;
	struct device_node *node;
	struct leds_priv *led_priv;
	struct led_init_data init_data;
	int i;
	count = device_get_child_node_count(dev);

	if (count == 0) {
		mylog("no child node\n");
		return -ENODEV;
	}

	led_priv = devm_kzalloc(dev, struct_size(led_priv, leds, count),
				GFP_KERNEL);
	led_priv->led_num = count;

	i = 0;
	for_each_child_of_node(dev->of_node, node) {
		mylog("child node of gpio-leds name:%s\n", node->name);

		led_priv->leds[i].gpiod = of_get_named_gpio(node, "gpios", 0);
		default_state = led_init_default_state_get(&node->fwnode);
		if (default_state == LEDS_DEFSTATE_ON)
			value = 1;
		else if (default_state == LEDS_DEFSTATE_OFF)
			value = 0;
		else {
			value = gpio_get_value(led_priv->leds[i].gpiod);
		}
		gpio_direction_output(led_priv->leds[i].gpiod, value);
		led_priv->leds[i].led.brightness_set = led_set;
		led_priv->leds[i].led.brightness_set_blocking =
			led_set_blocking;

		// if (!of_property_read_string(node, "linux,default-trigger",
		// 			     &string)) {
		init_data.fwnode = &node->fwnode;
		devm_led_classdev_register_ext(dev, &led_priv->leds[i].led,
					       &init_data);
		// }
		i++;
	}

	return 0;
}

static int my_platform_led_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id of_my_platform_led_match[] = {
	{ .compatible = "gpio-leds" },
	{},
};

MODULE_DEVICE_TABLE(of, of_my_platform_led_match);

static struct platform_driver my_platform_led_drv = {
	.probe = my_platform_led_probe,
	.remove = my_platform_led_remove,
	.driver = {
		.name = "my_platform_led",
		.of_match_table = of_my_platform_led_match,
	},
};

static int __init my_platform_led_init(void)
{
	pr_info("***************************my_platform_led_init***************\n");
	platform_driver_register(&my_platform_led_drv);
	return 0;
}

static void __exit my_platform_led_exit(void)
{
	platform_driver_unregister(&my_platform_led_drv);
	pr_info("***************************my_platform_led_exit***************\n");
}

module_init(my_platform_led_init);
module_exit(my_platform_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangshuai");