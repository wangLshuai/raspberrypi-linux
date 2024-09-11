#include <linux/completion.h>
#include <linux/jiffies.h>
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
#define I2C_ADDR_REG 0xc // slave address
#define I2C_DLEN_REG 0x8
#define I2C_FIFO_REG 0x10 // data fifo

// i2c clock divider CDIV is always rounded down to aneven number
#define I2C_DIV_REG 0x14
#define I2C_DELAY_REG 0x18 // data delay
#define I2C_CLKT_REG 0x1c // clock stretch timeout

// status bit
#define S_CLKT_BIT BIT(9)
#define S_ERR_BIT BIT(8)
#define S_RXF_BIT BIT(7)
#define S_TRE_BIT BIT(6)
#define S_RXD_BIT BIT(5)
#define S_TXD_BIT BIT(4)
#define S_RXR_BIT BIT(3)
#define S_TXW_BIT BIT(2)
#define S_DONE_BIT BIT(1)
#define S_TA_BIT BIT(0)

// control bit
#define C_EN_BIT BIT(15)
#define C_INTR_BIT BIT(10)
#define C_INTT_BIT BIT(9)
#define C_INTD_BIT BIT(8)
#define C_START_BIT BIT(7)
#define C_CLEAR_BIT BIT(5)
#define C_READ_BIT BIT(0)

static struct of_device_id i2c_controller_id_tables[] = {
	{
		.compatible = "brcm,bcm2711-i2c",
	},
	{}
};

struct i2c_controller {
	struct i2c_adapter adap;
	struct clk_hw i2c_bus_hw_clk;
	int irq;
	struct completion rw_completion;
	struct completion done;
	void *reg;
};

static int transfer_msg(struct i2c_msg *msg, struct i2c_controller *i2c_ctrl)
{
	iowrite32(msg->addr, i2c_ctrl->reg + I2C_ADDR_REG);
	iowrite32(msg->len, i2c_ctrl->reg + I2C_DLEN_REG);
	printk("msg len:%d\n", msg->len);
	if (msg->flags & I2C_M_RD) {
		printk("start read %d bytes\n", msg->len);
		iowrite32(C_EN_BIT | C_START_BIT | C_READ_BIT | C_INTD_BIT |
				  C_INTR_BIT,
			  i2c_ctrl->reg + I2C_CONTROL_REG);
		int i = 0;
		while (i < msg->len) {
			u32 status = ioread32(i2c_ctrl->reg + I2C_STATUS_REG);
			if (status & S_RXD_BIT) {
				msg->buf[i] =
					ioread32(i2c_ctrl->reg + I2C_FIFO_REG);
				printk("read a byte:%x\n", msg->buf[i]);
				i++;
			} else {
				if (msg->len - i >= 12) {
					if (!wait_for_completion_timeout(
						    &i2c_ctrl->rw_completion,
						    msecs_to_jiffies(100))) {
						printk("timeout\n");
						break;
					}
				} else {
					if (!wait_for_completion_timeout(
						    &i2c_ctrl->done,
						    msecs_to_jiffies(100))) {
						printk("timeout\n");
						break;
					}
				}
			}
		}

	} else {
		iowrite32((~C_READ_BIT) &
				  (C_EN_BIT | C_CLEAR_BIT | C_START_BIT |
				   C_INTD_BIT | C_INTT_BIT),
			  i2c_ctrl->reg + I2C_CONTROL_REG);

		int i = 0;
		while (i < msg->len) {
			u32 status = ioread32(i2c_ctrl->reg + I2C_STATUS_REG);
			if (status & S_TXD_BIT) {
				printk("write a byte:%x\n", msg->buf[i]);
				iowrite32(msg->buf[i],
					  i2c_ctrl->reg + I2C_FIFO_REG);
				printk("writed a byte:%x\n", msg->buf[i]);
				i++;
			} else {
				if (!wait_for_completion_timeout(
					    &i2c_ctrl->rw_completion,
					    msecs_to_jiffies(100))) {
					printk("timeout\n");
					break;
				}
			}
		}

		printk("wait tx complete\n");
		u32 status = ioread32(i2c_ctrl->reg + I2C_STATUS_REG);
		if (status & S_TA_BIT) {
			if (!wait_for_completion_timeout(
				    &i2c_ctrl->done, msecs_to_jiffies(100))) {
				pr_err("wait tx complete timeout\n");
			}

		} else {
			init_completion(&i2c_ctrl->rw_completion);
		}
	}

	iowrite32(C_CLEAR_BIT, i2c_ctrl->reg + I2C_CONTROL_REG);
	return 0;
}
static int i2c_controller_master_xfer(struct i2c_adapter *adap,
				      struct i2c_msg *msgs, int num)
{
	struct i2c_controller *i2c_controller =
		container_of(adap, struct i2c_controller, adap);
	iowrite32(C_CLEAR_BIT, i2c_controller->reg + I2C_CONTROL_REG);
	for (int i = 0; i < num; i++) {
		transfer_msg(&msgs[i], i2c_controller);
	}
	return num;
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
	u32 div = DIV_ROUND_UP(*parent_rate, rate);
	if (div & 1)
		div--;
	return DIV_ROUND_UP(*parent_rate, div);
}

int set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	struct i2c_controller *i2c_controller =
		container_of(hw, struct i2c_controller, i2c_bus_hw_clk);
	u32 div = parent_rate / rate;
	div = max(2, div);
	div = min(0xffff, div);
	mylog("parent_rate:%lu,rate:%lu,div:%x\n", parent_rate, rate, div);
	iowrite32(div, i2c_controller->reg + I2C_DIV_REG);

	// a i2c bus clock cycle  = div parent clock cycles
	// high level or lowel level,half cycle = div/2 div parent clock cycles
	// change sda when sck low level
	// controller changer sda level after falling edge when send next bit
	// controller sample sda level after rise edge when controller receive next bit

	//
	u16 fedl = max(div / 4, 1);
	u16 redl = max(div / 4, 1);
	iowrite32((fedl << 16) | redl, i2c_controller->reg + I2C_DELAY_REG);

	// Number of SCL clock cycles to wait after the rising edge of
	// SCL before deciding that the slave is not responding
	iowrite32(div, i2c_controller->reg + I2C_CLKT_REG);

	return 0;
}
static struct clk_ops clk_ops = {
	.recalc_rate = recalc_get_rate,
	.round_rate = round_rate,
	.set_rate = set_rate,
};

static irqreturn_t i2c_irq_handler(int irq, void *data)
{
	struct i2c_controller *i2c_controller = data;
	u32 status = ioread32(i2c_controller->reg + I2C_STATUS_REG);
	u32 value = ioread32(i2c_controller->reg + I2C_CONTROL_REG);
	printk("status:0x%x value:0x%x", status & 0xffff, value & 0xffff);
	if (status & S_DONE_BIT) {
		printk("done\n");
		iowrite32(S_DONE_BIT, i2c_controller->reg + I2C_STATUS_REG);
		complete(&i2c_controller->done);
	}
	if ((status & S_RXR_BIT) || (status & S_TXW_BIT)) {
		printk("irq rw\n");
		status = ioread32(i2c_controller->reg + I2C_DLEN_REG);
		printk("len:%u\n", status);
		complete(&i2c_controller->rw_completion);

		// ?????????? bug
		// iowrite32(C_CLEAR_BIT, i2c_controller->reg + I2C_CONTROL_REG);
	}

	if (status & S_CLKT_BIT) {
		iowrite32(S_CLKT_BIT, i2c_controller->reg + I2C_STATUS_REG);
		pr_err("CLKT\n");
	}
	if (status & S_ERR_BIT) {
		iowrite32(S_ERR_BIT, i2c_controller->reg + I2C_STATUS_REG);
		pr_err("error\n");
	}

	return IRQ_HANDLED;
}
static int i2c_controller_probe(struct platform_device *pdev)
{
	int ret;

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
	init_completion(&i2c_controller->rw_completion);
	init_completion(&i2c_controller->done);

	struct resource *reg = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c_controller->reg = devm_ioremap_resource(&pdev->dev, reg);
	mylog("reg:%llx\n", (uint64_t)i2c_controller->reg);
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

	i2c_controller->irq = platform_get_irq(pdev, 0);
	ret = request_irq(i2c_controller->irq, i2c_irq_handler, IRQF_SHARED,
			  "i2c-controller", i2c_controller);
	if (ret) {
		pr_err("request irq failed,ret:%d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(i2c_controller->i2c_bus_hw_clk.clk);
	if (ret) {
		free_irq(i2c_controller->irq, i2c_controller);
		dev_err(&pdev->dev, "Couldn't prepare clock");
		return ret;
	}
	// add a i2c bus,and foreach child node to register i2c_client.
	ret = i2c_add_adapter(&i2c_controller->adap);
	if (ret)
		clk_unprepare(i2c_controller->i2c_bus_hw_clk.clk);

	return ret;
}

int i2c_controller_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adap = platform_get_drvdata(pdev);

	struct i2c_controller *i2c_controller =
		container_of(adap, struct i2c_controller, adap);
	free_irq(i2c_controller->irq, i2c_controller);
	clk_rate_exclusive_put(i2c_controller->i2c_bus_hw_clk.clk);
	clk_disable_unprepare(i2c_controller->i2c_bus_hw_clk.clk);
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
