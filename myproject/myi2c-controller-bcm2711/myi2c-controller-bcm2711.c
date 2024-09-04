#include <linux/container_of.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
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
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>

#define I2C_CONTROL_REG 0x0
#define I2C_STATUS_REG 0x4
#define I2C_DLEN_REG 0x8
#define I2C_ADDR_REG 0xc // slave address
#define I2C_FIFO_REG 0x10 // data fifo

// i2c clock divider CDIV is always rounded down to aneven number
#define I2C_DIV_REG 0x14
#define I2C_DELAY_REG 0x18 // data delay
#define I2C_CLKT_REG 0x1c // clock stretch timeout
static struct of_device_id i2c_controller_id_tables[] = {
	{
		.compatible = "brcm,bcm2711-i2c",
	},
	{}
};

struct i2c_controller {
	struct i2c_adapter adap;
	struct clk_hw i2c_bus_hw_clk;
	void *reg;
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

static u32 i2c_controller_algo_support(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}
static struct i2c_algorithm algo = {
	.functionality = i2c_controller_algo_support,
	.master_xfer = i2c_controller_master_xfer,
	// .master_xfer_atomic = i2c_controller_master_xfer_atomic,
	// .master_xfer_msg = i2c_controller_master_xfer_msg,
	// .master_xfer_msg_atomic = i2c_controller_master_xfer_msg_atomic,
};

// __clk_register call ops->recalc_rate get clk_core->rate
// or change clk_core->rate after call ops->set_rate
static unsigned long recalc_get_rate(struct clk_hw *hw,
				     unsigned long parent_rate)
{
	struct i2c_controller *i2c_controller =
		container_of(hw, struct i2c_controller, i2c_bus_hw_clk);
	u32 div = ioread32(i2c_controller->reg + I2C_DIV_REG);
	mylog("parent rate:%lu div:0x%x rate:%lu\n", parent_rate, div,
	      DIV_ROUND_UP(parent_rate, div));
	return DIV_ROUND_UP(parent_rate, div);
}

// clk_set_rate_exclusive call round_rate,
// if result == core->rate, not call ops->set_rate to set hardware
long round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *parent_rate)
{
	mylog("parent_rate:%llu,rate:%lu\n", *parent_rate, rate);

	return rate;
}

int set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	struct i2c_controller *i2c_controller =
		container_of(hw, struct i2c_controller, i2c_bus_hw_clk);
	u32 div = parent_rate / rate;
	mylog("parent_rate:%lu,rate:%lu,div:%x\n", parent_rate, rate, div);
	iowrite32(div, i2c_controller->reg + I2C_DIV_REG);
	return 0;
}
static struct clk_ops clk_ops = {
	.recalc_rate = recalc_get_rate,
	.round_rate = round_rate,
	.set_rate = set_rate,
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
	int i, count, ret;
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

	mylog("+++++++++++++++++++++++++++++start probe i2c controller\n");
	int bus_clk_rate;
	struct i2c_controller *i2c_controller = devm_kzalloc(
		&pdev->dev, sizeof(struct i2c_controller), GFP_KERNEL);
	platform_set_drvdata(pdev, &i2c_controller->adap);
	snprintf(i2c_controller->adap.name, sizeof(i2c_controller->adap.name),
		 "bcm2835 (%s)", of_node_full_name(pdev->dev.of_node));
	i2c_controller->adap.owner = THIS_MODULE;
	i2c_controller->adap.class = I2C_CLASS_DEPRECATED;
	i2c_controller->adap.algo = &algo;
	i2c_controller->adap.dev.parent = &pdev->dev;
	i2c_controller->adap.dev.of_node = pdev->dev.of_node;

	struct resource *reg = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c_controller->reg = devm_ioremap_resource(&pdev->dev, reg);
	mylog("reg->start:%llx\n", reg->start);
	// get parent clock , clocks = <&clocks BCM2835_CLOCK_VPU> in /soc/i2c1: i2c@7e804000
	// clocks == of_clk_provider->node
	// clk_hw_create_clk(of_clk_provider->data->hws[BCM2835_CLOCK_VPU])
	// mailbox send message to  vpu get rate
	struct clk *clk = devm_clk_get(&pdev->dev, NULL);
	mylog("i2c controller parent clk name:%s rate:%lu\n",
	      __clk_get_name(clk), clk_get_rate(clk));

	struct clk_init_data init;
	init.ops = &clk_ops;
	init.name = "";
	init.flags = 0;
	init.num_parents = 1;
	init.parent_names = (const char *[]){ __clk_get_name(clk) };

	i2c_controller->i2c_bus_hw_clk.init = &init;

	clk_hw_register_clkdev(&i2c_controller->i2c_bus_hw_clk, "div",
			       dev_name(&pdev->dev)); // register clklookup
	devm_clk_register(&pdev->dev,
			  &i2c_controller->i2c_bus_hw_clk); // set core ops

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				   &bus_clk_rate);
	mylog("bus_clk_rate:%d\n", bus_clk_rate);
	if (ret < 0) {
		dev_warn(&pdev->dev,
			 "Could not read clock-frequency property\n");
		bus_clk_rate = I2C_MAX_STANDARD_MODE_FREQ;
	}

	mylog("clk_set_rate_exclusive\n");
	ret = clk_set_rate_exclusive(i2c_controller->i2c_bus_hw_clk.clk,
				     bus_clk_rate);
	if (ret < 0)
		return dev_err_probe(&pdev->dev, ret,
				     "Could not set clock frequency\n");
	mylog("current rate:%lu\n",
	      clk_get_rate(i2c_controller->i2c_bus_hw_clk.clk));
	// call parent ops->prepare and ops->enable if function no null
	ret = clk_prepare_enable(i2c_controller->i2c_bus_hw_clk.clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't prepare clock");
		return ret;
	}

	// add a i2c bus,and foreach child node to register i2c_client.
	ret = i2c_add_adapter(&i2c_controller->adap);
	// if (ret)
	// 	clk_unprepare(i2c_controller->i2c_bus_hw_clk.clk);
	return 0;
}

int i2c_controller_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adap = platform_get_drvdata(pdev);

	i2c_del_adapter(adap);
	return 0;
}
static struct platform_driver i2c_controller = {
	.driver = { .of_match_table = i2c_controller_id_tables,
		    .name = "myi2c_controller_driver" },
	.probe = i2c_controller_probe,
	.remove = i2c_controller_remove,
};

MODULE_DEVICE_TABLE(of, i2c_controller_id_tables);
static int __init myi2c_controller_bcm2711_init(void)
{
	mylog("\n");
	platform_driver_register(&i2c_controller);
	return 0;
}

static void __exit myi2c_controller_bcm2711_exit(void)
{
	mylog("\n");
	platform_driver_unregister(&i2c_controller);
}

module_init(myi2c_controller_bcm2711_init);
module_exit(myi2c_controller_bcm2711_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("wangshuai");
