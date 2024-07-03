#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>

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
	int len, i, num;
	char value[128];
	char property_str[128];
	struct device *dev;
	struct platform_device *pdev;
	const __be32 *addrp;
	u64 size;
	unsigned int flags;
	u64 taddr;
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
		printk("platform device resource num:%d\n", len);
		if (dev->driver != NULL) {
			printk("match driver name dev->driver->name:%s\n",
			       dev->driver->name);
		}
	}

	np = of_find_node_by_name(NULL, "gpiomem");
	pp = of_find_property(np, "reg", &len);
	if (pp) {
		printk("reg property len:%d\n", len);
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 1, 16,
			       pp->value, len, 1);
	}
	if (np) {
		printk("find %s device node\n", np->name);
		num = of_address_count(np);
		printk("resource count:%d\n", num);
		for (i = 0; i < num; i++) {
			printk("resource %d\n", i);
			// of_address_to_resource(np, i, &res);
			// printk("start:%llx end:%llx\n", res.start, res.end);
			addrp = __of_get_address(np, i, -1, &size, &flags);
			if (flags & IORESOURCE_MEM) {
				taddr = of_translate_address(np, addrp);
				printk("taddr:%llx\n", taddr);
			} else if (flags & IORESOURCE_IO)
				printk("IORESOURCE_IO\n");
			else
				return -EINVAL;
		}
	}

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "fe200000.gpiomem");
	if (dev != NULL) {
		pdev = to_platform_device(dev);
		printk("find %s  platform device\n", pdev->name);
		len = pdev->num_resources;
		printk("led platform device resource num:%d\n", len);
		if (dev->driver != NULL) {
			printk("match name dev->driver->name:%s\n",
			       dev->driver->name);
		}
		for (i = 0; i < len; i++) {
			printk("resource %d res->name:%s res->start:%llx ,res->end:%llx\n",
			       i, pdev->resource[i].name,pdev->resource[i].start,
			       pdev->resource[i].end);
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