#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
int sht2x_probe(struct i2c_client *client)
{
	mylog("client name:%s addr:0x%x\n", client->name, client->addr);
	return 0;
}
const struct i2c_device_id sht2x_id_table[] = { {
							.name = "sht20",
							.driver_data = 0,
						},
						{
							.name = "sht21",
							.driver_data = 0,
						},
						{} };
static struct i2c_driver sht2x_driver = {
	.probe = sht2x_probe,
	.driver = {
		.name = "sht2x",
	},
	.id_table = sht2x_id_table,
};
static int __init sht2x_init(void)
{
	printk(KERN_INFO "SHT2x init\n");
	i2c_add_driver(&sht2x_driver);
	return 0;
}

static void __exit sht2x_exit(void)
{
	i2c_del_driver(&sht2x_driver);
	printk(KERN_INFO "SHT2x exit\n");
}

module_init(sht2x_init);
module_exit(sht2x_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("wangshuai");