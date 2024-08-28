#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/of.h>

static struct of_device_id i2c_controller_id_tables[] = {
	{
	 .compatible = "brcm,bcm2711-i2c",
	  },
	{ }
};

static int i2c_controller_probe(struct platform_device *pdev)
{
	struct device_node *node = dev_of_node(&pdev->dev);
	struct property *pp;
	const __be32 *addrp;
	u64 phy_addr;
	u64 size;
	uint flags;
	struct resource res;
	struct pinctrl *pc;
	struct pinctrl_state *state;
	int i, count;
	const char *str;

	for_each_property_of_node(node, pp) {
		printk("property name:%s\n", pp->name);
	}

	printk("address count:%d\n", of_address_count(node));
	addrp = of_get_address(node, 0, &size, &flags);
	print_hex_dump(KERN_INFO, "reg value", DUMP_PREFIX_OFFSET, 16, 1, addrp,
		       of_n_addr_cells(node) * 4, false);
	of_address_to_resource(node, 0, &res);
	mylog("resource:%llx size:%llx\n", res.start, res.end - res.start + 1);
	phy_addr = of_translate_address(node, addrp);
	mylog("phy_addr:%llx\n", phy_addr);

	// really_probe call pinctrl_bind_pins when device match driver,set dev->pins
	// and select init or
	pc = pinctrl_get(&pdev->dev);
	if (!IS_ERR(pc)) {
		state = pinctrl_lookup_state(pc, "default");
		if (!IS_ERR(state)) {
			mylog("state == dev->pins.defaults:%d\n",
			      state == pdev->dev.pins->default_state);
		}

		count = of_property_count_strings(node, "pinctrl-names");
		for (i = 0; i < count; i++) {
			of_property_read_string_index(node, "pinctrl-names", i,
						      &str);
			state = pinctrl_lookup_state(pc, str);
			if (!IS_ERR(state)) {
				mylog("state name:%s\n", str);
			}
		}

		state = pinctrl_lookup_state(pc, "sleep");
		if (!IS_ERR(state)) {
			mylog("i2c select sleep pinctrl test\n");
			pinctrl_select_state(pc, state);
		}

		pinctrl_put(pc);
	}
	return 0;
}

static struct platform_driver i2c_control = {
	.driver = {.of_match_table = i2c_controller_id_tables,
		   .name = "myi2c_controller_driver" },
	.probe = i2c_controller_probe,
};

MODULE_DEVICE_TABLE(of, i2c_controller_id_tables);
static int __init myi2c_controller_bcm2711_init(void)
{
	mylog("\n");
	platform_driver_register(&i2c_control);
	return 0;
}

static void __exit myi2c_controller_bcm2711_exit(void)
{
	mylog("\n");
	platform_driver_unregister(&i2c_control);
}

module_init(myi2c_controller_bcm2711_init);
module_exit(myi2c_controller_bcm2711_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("wangshuai");
