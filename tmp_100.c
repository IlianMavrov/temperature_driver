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
#include <linux/err.h>
#include <linux/kernel.h>

#define DRIVER_NAME	"tmp100"

#define BUF_SIZE	(10)

#define TMP100_READ_REG	0x00
#define TMP100_CONF_REG	0x01
#define TMP100_LOW_REG	0x02
#define TMP100_HIGH_REG	0x03

struct tmp100_data {
	dev_t dev;
	struct regmap *regmap;
	struct class *dev_class;
	struct cdev etx_cdev;
};
struct tmp100_data tmp100_data;

static ssize_t tmp100_print(struct file *filp, char __user *buf,
				size_t len, loff_t *off)
{
	unsigned int regval;
	int err, count;
	s16 regval_signed;
	char temp[BUF_SIZE];

	err = regmap_read(tmp100_data.regmap, TMP100_READ_REG, &regval);
	if (err < 0)
		return err;

	regval_signed = regval;
	if (regval_signed < 0) {
		regval_signed = -regval_signed;
		snprintf(temp, sizeof(temp), "- %d.%04d\n", regval_signed>>8,
				(((regval_signed>>4)&0xf)*10000)>>4);
	} else {
		snprintf(temp, sizeof(temp), "%d.%04d\n", regval_signed>>8,
				(((regval_signed>>4)&0xf)*10000)>>4);
	}

	temp[BUF_SIZE - 1] = '\0';
	count = strlen(temp);

	/*read full size of temp*/
	if (len < count)
		return -EINVAL;
	/*check if all data is read*/
	if (*off != 0)
		return -EINVAL;

	if (copy_to_user(buf, temp, count))
		return -EFAULT;

	*off = count;
	return count;
}

static const struct regmap_config tmp100_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = TMP100_HIGH_REG,
};
const struct file_operations fops = {
	.owner          = THIS_MODULE,
	.read           = tmp100_print,
};

static int tmp100_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	tmp100_data.dev = 0;
	tmp100_data.regmap = devm_regmap_init_i2c(client,
			&tmp100_regmap_config);

	if (IS_ERR(tmp100_data.regmap))
		return PTR_ERR(tmp100_data.regmap);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C_FUNC_I2C not supported!!!\n");
		return -ENODEV;
	}

	if ((alloc_chrdev_region(&tmp100_data.dev, 0, 1, DRIVER_NAME)) < 0) {
		pr_info("Cannot alocate major number for device 1\n");
		return -1;
	}

	cdev_init(&tmp100_data.etx_cdev, &fops);

	if ((cdev_add(&tmp100_data.etx_cdev, tmp100_data.dev, 1)) < 0) {
		pr_info("Cannot add the device to the system\n");
		goto r_class;
	}

	tmp100_data.dev_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (tmp100_data.dev_class == NULL) {
		pr_info("Cannot create the struct class for device\n");
		goto r_class;
	}

	if ((device_create(tmp100_data.dev_class, NULL, tmp100_data.dev, NULL,
					DRIVER_NAME)) == NULL) {
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
