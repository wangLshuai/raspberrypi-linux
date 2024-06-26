#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

int process_platform_device(struct device *device, void *data)
{
	struct platform_device *pdev;
	pdev = to_platform_device(device);
	printk("pdev->name:%s\n", pdev->name);

	return 0;
}
static int __init of_test_init(void)
{
	struct device_node *np;
	struct device_node *child;
	struct property *pp;
	int len, i;
	char value[128];
	char property_str[128];
	struct device *dev;
	struct platform_device *pdev;
	printk("of_test_init\n");
	np = of_find_node_by_path("/");
	printk("root of node full name:%s\n", np->full_name);
	printk("child node of /:\n");
	for_each_child_of_node(np, child) {
		printk("of node full_name:%s name:%s\n", child->full_name,
		       child->name);
	}

	np = of_find_node_by_name(NULL, "leds");
	if (np != NULL) {
		printk("find leds,foreach property\n");
		for_each_property_of_node(np, pp) {
			len = pp->length;
			snprintf(property_str, 128, "property name:%s len:%d  ",
				 pp->name, len);
			for (i = 0; i < len && i < 64; i++) {
				snprintf(value + i * 2, 128 - i * 2, "%02x",
					 ((char *)pp->value)[i]);
			}
			printk("%s %s\n", property_str, value);
		}
	}

	bus_for_each_dev(&platform_bus_type, NULL, NULL,
			 process_platform_device);

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "leds");

	if (dev != NULL) {
		printk("find leds  platform device\n");
		pdev = to_platform_device(dev);
		len = pdev->num_resources;
		printk("led platform device resource num:%d\n", len);
		if (dev->driver != NULL) {
			printk("match name dev->driver->name:%s\n",
			       dev->driver->name);
		}
	}
	return 0;
}

static void __exit of_test_exit(void)
{
}

module_init(of_test_init);
module_exit(of_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangshuai");