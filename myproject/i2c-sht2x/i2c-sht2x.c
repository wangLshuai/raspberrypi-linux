#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>

#define TEMP_CMD 0xe3
#define HUMIDITY_CMD 0xe5
static const struct of_device_id sht2x_id_table[] = {
	{
		.compatible = "sht20",
	},
	{
		.compatible = "sht21",
	},
	{}
};

struct sht2x_device {
	struct i2c_client *client;
	struct attribute_group group;
	struct mutex lock;
};

ssize_t temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct i2c_client *client =
		container_of(container_of(kobj, struct device, kobj),
			     struct i2c_client, dev);
	struct sht2x_device *sht2x = dev_get_drvdata(&client->dev);
	char data[3] = { 0 };
	data[0] = TEMP_CMD;
	mutex_lock(&sht2x->lock);
	i2c_master_send(client, data, 1);
	i2c_master_recv(client, data, 3);
	mutex_unlock(&sht2x->lock);
	uint64_t temp = data[0];
	temp = (temp << 8) | data[1];
	uint64_t temp_value = (temp * 175720) / 65536 - 46850;

	return sprintf(buf, "%llu", temp_value);
}

ssize_t humidity_show(struct kobject *kobj, struct kobj_attribute *attr,
		      char *buf)
{
	struct i2c_client *client =
		container_of(container_of(kobj, struct device, kobj),
			     struct i2c_client, dev);
	struct sht2x_device *sht2x = dev_get_drvdata(&client->dev);
	char data[3] = { 0 };
	data[0] = HUMIDITY_CMD;
	mutex_lock(&sht2x->lock);
	i2c_master_send(client, data, 1);
	i2c_master_recv(client, data, 3);
	mutex_unlock(&sht2x->lock);
	uint64_t humidity = (data[0] << 8) | data[1];
	uint64_t humidity_value = ((humidity * 125000) >> 16) - 6000;

	return sprintf(buf, "%llu", humidity_value);
}
struct kobj_attribute sht2x_humidity = {
	.attr = { .name = "humidity", .mode = 0444 },
	.show = humidity_show,
};

struct kobj_attribute sht2x_temp = {
	.attr = { .name = "temp", .mode = 0444 },
	.show = temp_show,
};

static struct attribute *sht2x_attr[] = { &sht2x_temp.attr,
					  &sht2x_humidity.attr, NULL

};

static void group_release(struct device *dev, void *res)
{
	struct sht2x_device *sht2x_dev = res;
	sysfs_remove_group(&sht2x_dev->client->dev.kobj, &sht2x_dev->group);
}
int sht2x_probe(struct i2c_client *client)
{
	int ret;
	struct sht2x_device *sht2x_dev;
	if (!of_match_node(sht2x_id_table, client->dev.of_node)) {
		pr_err("device tree node not match\n");
		return -EINVAL;
	}
	sht2x_dev = devres_alloc(group_release, sizeof(*sht2x_dev), GFP_KERNEL);
	sht2x_dev->group.attrs = sht2x_attr;
	sht2x_dev->group.name = "sensor_data";
	sht2x_dev->client = client;
	mutex_init(&sht2x_dev->lock);
	devres_add(&client->dev, sht2x_dev);
	dev_set_drvdata(&client->dev, sht2x_dev);
	ret = sysfs_create_group(&client->dev.kobj, &sht2x_dev->group);

	if (ret) {
		pr_err("sysfs create group failed\n");
		return ret;
	}

	return 0;
}

static struct i2c_driver sht2x_driver = {
	.probe = sht2x_probe,
	.driver = { .name = "sht2x", .of_match_table = sht2x_id_table },
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