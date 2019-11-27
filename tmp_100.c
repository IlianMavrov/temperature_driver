// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>

#define BYTES_TO_READ		2
#define READ_REGISTER		0x00
#define CONF_REGISTER		0x01
#define TMP100_LOW_REGISTER	0x02
#define TMP100_HIGH_REGISTER	0x03

struct tmp100_data {
	dev_t dev;
	struct i2c_client *master_client;
	struct class *dev_class;
	struct cdev etx_cdev;
	struct regmap *regmap;
};
struct tmp100_data tmp100_data;

static ssize_t tmp100_print(struct file *filp, char __user *buf,
				size_t len, loff_t *off)
{
	//add here regmap_read
	unsigned int regval;
	int err;
	s16 temperature;
	char temp[9];

	err = regmap_read(tmp100_data.regmap, READ_REGISTER, &regval);
	if (err < 0)
		return err;

	temperature = regval;
	pr_alert("The temperature is : %d", regval>>8);
	//	snprintf(temp, sizeof(temp), "%2d.%2d" (regval>>8)(((regval>>4)&0xf)>>4));

	//copy_to_user
	return 0;
}

const struct file_operations fops = {
	.owner          = THIS_MODULE,
	.read           = tmp100_print,
};

static const struct regmap_config tmp100_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = TMP100_HIGH_REGISTER,
};

static int tmp100_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{

	int error;
	u8 val[BYTES_TO_READ];

	struct i2c_adapter *adapter = client->adapter;

	tmp100_data.master_client = client;

	error = i2c_check_functionality(adapter, I2C_FUNC_I2C);
	if (error < 0)
		return -ENODEV;
	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0)
		return -ENODEV;

	if ((alloc_chrdev_region(&tmp100_data.dev, 0, 1, "proc_tmp100_driver")) < 0) {
		pr_info("Cannot alocate major number for device 1\n");
		return -1;
	}

	cdev_init(&tmp100_data.etx_cdev, &fops);

	if ((cdev_add(&tmp100_data.etx_cdev, tmp100_data.dev, 1)) < 0) {
		pr_info("Cannot add the device to the system\n");
		goto r_class;
	}

	tmp100_data.dev_class = class_create(THIS_MODULE, "tmp100_class");
	if (tmp100_data.dev_class == NULL) {
		pr_info("Cannot create the struct class for device\n");
		goto r_class;
	}

	if ((device_create(tmp100_data.dev_class, NULL, tmp100_data.dev, NULL, "tmp100_device")) == NULL) {
		pr_info("Cannot create the Device\n");
		goto r_device;
	}
	pr_alert("Kernel Driver Inserted Successfully...\n");

	return 0;

r_device:
	class_destroy(tmp100_data.dev_class);
r_class:
	unregister_chrdev_region(tmp100_data.dev, 1);
	return -1;
};
static int tmp100_remove(struct i2c_client *client)
{

	device_destroy(tmp100_data.dev_class, tmp100_data.dev);
	class_destroy(tmp100_data.dev_class);
	cdev_del(&tmp100_data.etx_cdev);
	unregister_chrdev_region(tmp100_data.dev, 1);

	pr_alert("Kernel Driver Removed Successfully...\n");
	return 0;
};

static const struct i2c_device_id tmp100_i2c_id[] = {
	{"tmp100", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, tmp100_i2c_id);

static const struct of_device_id tmp100_of_match[] = {
	{ .compatible = "ti,tmp100"},
	{},
};
MODULE_DEVICE_TABLE(of, tmp100_of_match);

static struct i2c_driver tmp100_driver = {
	.driver = {
		.name = "MMS_tmp100",
		.of_match_table = tmp100_of_match,
		.owner = THIS_MODULE,
	},
	.probe = tmp100_probe,
	.remove = tmp100_remove,
	.id_table = tmp100_i2c_id,
};

module_i2c_driver(tmp100_driver);

MODULE_AUTHOR("Iliyan Mavrov");
MODULE_DESCRIPTION("TMP100 DRIVER");
MODULE_LICENSE("GPL");
