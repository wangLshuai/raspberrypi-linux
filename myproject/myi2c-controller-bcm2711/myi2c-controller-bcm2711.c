#include "linux/device.h"
#include "linux/gfp_types.h"
#include "linux/i2c.h"
#include "linux/irqnr.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/of.h>
#include <linux/of_irq.h>

static struct of_device_id i2c_controller_id_tables[] = {
	{
		.compatible = "brcm,bcm2711-i2c",
	},
	{}
};

struct i2c_control {
	struct i2c_adapter adap;
};

static int i2c_controller_master_xfer(struct i2c_adapter *adap,
				      struct i2c_msg *msgs, int num)
{
	return 0;
}
static int i2c_controller_master_xfer_atomic(struct i2c_adapter *adap,
					     struct i2c_msg *msgs, int num)
{
	return 0;
}

u32 i2c_controller_algo_support(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}
static struct i2c_algorithm algo = {
	.functionality = i2c_controller_algo_support,
	.master_xfer = i2c_controller_master_xfer,
	.master_xfer_atomic = i2c_controller_master_xfer_atomic,
	// .master_xfer_msg = i2c_controller_master_xfer_msg,
	// .master_xfer_msg_atomic = i2c_controller_master_xfer_msg_atomic,
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
	struct irq_desc *irq_desc;

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

	struct of_phandle_args args;
	u32 intsize;
	of_irq_parse_one(node, 0, &args);
	mylog("args:%s %d %d %d\n", args.np->name, args.args[0], args.args[1],
	      args.args[2]);
	struct device_node *interrupt = of_irq_find_parent(node);
	of_node_put(interrupt);
	mylog("interrupt:%s\n", interrupt->name);

	if (!of_property_read_u32(args.np, "#interrupt-cells", &intsize))
		mylog("intsize:%d", intsize);
	int irq = platform_get_irq(pdev, 0);
	irq_desc = irq_to_desc(irq);
	mylog("irq:%d hw_irq:%lu interrupt controller:%s\n", irq,
	      irq_desc->irq_data.hwirq, irq_desc->irq_data.domain->name);
	mylog("of_irq_get:%d\n", of_irq_get(node, 0));

	struct i2c_control *i2c_control = devm_kzalloc(
		&pdev->dev, sizeof(struct i2c_control), GFP_KERNEL);
	platform_set_drvdata(pdev, &i2c_control->adap);
	snprintf(i2c_control->adap.name, sizeof(i2c_control->adap.name),
		 "bcm2835 (%s)", of_node_full_name(pdev->dev.of_node));
	i2c_control->adap.owner = THIS_MODULE;
	i2c_control->adap.class = I2C_CLASS_DEPRECATED;
	i2c_control->adap.algo = &algo;
	i2c_control->adap.dev.parent = &pdev->dev;
	i2c_control->adap.dev.of_node = pdev->dev.of_node;

	// add a i2c bus,and foreach child node to register i2c_client.
	i2c_add_adapter(&i2c_control->adap);
	return 0;
}

int i2c_controller_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adap = platform_get_drvdata(pdev);

	i2c_del_adapter(adap);
	return 0;
}
static struct platform_driver i2c_control = {
	.driver = { .of_match_table = i2c_controller_id_tables,
		    .name = "myi2c_controller_driver" },
	.probe = i2c_controller_probe,
	.remove = i2c_controller_remove,
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
