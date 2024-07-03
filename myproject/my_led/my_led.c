#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of_address.h>

static uintptr_t gpio_base_phy_addr;

#define GPIO_INPUT_SELECT 0x00
#define GPIO_OUTPUT_SELECT 0x01

#define GPIO_ALT0_SELECT 0x04
#define GPIO_ALT1_SELECT 0x05
#define GPIO_ALT2_SELECT 0x06
#define GPIO_ALT3_SELECT 0x07
#define GPIO_ALT4_SELECT 0x03
#define GPIO_ALT5_SELECT 0x02

#define GPIO_PIN21_FSEL_REG 2
#define GPIO_SET_REG0 7
#define GPIO_SET_REG1 8
#define GPIO_CLR_REG0 10
#define GPIO_CLR_REG1 11

#define GPIO_PIN21_SET_REG GPIO_SET_REG0
#define GPIO_PIN21_CLR_REG GPIO_CLR_REG0
#define GPIO_PIN21_OFFSET 3

static uint32_t *gpio_base;

struct my_led {
	char *name;
	struct cdev cdev;
	dev_t devid;
	int status;
	struct class *class;
	struct device *dev;
} my_led;

static ssize_t my_led_read(struct file *file, char __user *buf, size_t count,
			   loff_t *pos)
{
	if (my_led.status == 0) {
		return simple_read_from_buffer(buf, count, pos, "off\n",
					       strlen("off\n") + 1);
	}

	return simple_read_from_buffer(buf, count, pos, "on\n",
				       strlen("on\n") + 1);
}

static ssize_t my_led_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *pos)
{
	printk("my_led_write\n");
	char str[4] = { 0 };
	size_t ret;
	ret = simple_write_to_buffer(str, 4, pos, buf, count);
	str[ret] = 0;
	printk("%s\n", str);
	if (!strncmp(str, "off", 3)) {
		gpio_base[GPIO_PIN21_CLR_REG] |= (1 << 21);
		my_led.status = 0;
	} else if (!strncmp(str, "on", 2)) {
		gpio_base[GPIO_PIN21_SET_REG] |= (1 << 21);
		my_led.status = 1;
	}
	return ret;
}
struct file_operations my_led_fops = {
	.read = my_led_read,
	.write = my_led_write,
};
static int __init my_led_init(void)
{
	int rc;
	struct device_node *np;
	int count;
	const __be32 *addrp;
	u64 size;
	unsigned int flags;
	printk("my_led_init\n");
	my_led.name = "my_led";
	rc = alloc_chrdev_region(&my_led.devid, 1, 1, my_led.name);
	printk("major:%d minor:%d\n", MAJOR(my_led.devid), MINOR(my_led.devid));
	if (rc) {
		pr_err("alloc_chrdev_region failed\n");
		return rc;
	}

	cdev_init(&my_led.cdev, &my_led_fops);
	rc = cdev_add(&my_led.cdev, my_led.devid, 1);
	if (rc) {
		pr_err("cdev_add failed\n");
		unregister_chrdev_region(my_led.devid, 1);
		return rc;
	}

	my_led.class = class_create(my_led.name);
	my_led.dev = device_create(my_led.class, NULL, my_led.devid, NULL,
				   my_led.name);

	np = of_find_node_by_name(NULL, "gpio");
	if (np == NULL) {
		pr_err("of_find_node_by_name failed\n");
		cdev_del(&my_led.cdev);
		unregister_chrdev_region(my_led.devid, 1);
		return -ENODEV;
	}

	count = of_address_count(np);
	printk("device node  address count:%d", count);
	for (int i = 0; i < count; i++) {
		addrp = __of_get_address(np, i, -1, &size, &flags);
		print_hex_dump(KERN_INFO, "addr", DUMP_PREFIX_OFFSET, 1, 16,
			       addrp, sizeof(*addrp), 0);
		if (flags & IORESOURCE_MEM) {
			gpio_base_phy_addr = of_translate_address(np, addrp);
			printk("i:%d taddr:%lx size:%lld\n", i,
			       gpio_base_phy_addr, size);
		}
	}

	gpio_base = ioremap(gpio_base_phy_addr, size);

	if (IS_ERR(gpio_base)) {
		pr_err("ioremap failed\n");
		unregister_chrdev_region(my_led.devid, 1);
		return PTR_ERR(gpio_base);
	}

	gpio_base[GPIO_PIN21_FSEL_REG] |=
		(GPIO_OUTPUT_SELECT << GPIO_PIN21_OFFSET);
	gpio_base[GPIO_PIN21_CLR_REG] |= (1 << 21);
	my_led.status = 0;
	return 0;
}

static void __exit my_led_exit(void)
{
	device_del(my_led.dev);
	class_destroy(my_led.class);
	iounmap(gpio_base);
	cdev_del(&my_led.cdev);
	unregister_chrdev_region(my_led.devid, 1);
}

module_init(my_led_init);
module_exit(my_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangshuai");