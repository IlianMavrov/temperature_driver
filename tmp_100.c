// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#define DRIVER_NAME		"tmp100"

#define BUFSIZE			(10)

#define READ_REGISTER		0x00
#define CONF_REGISTER		0x01
#define TMP100_LOW_REGISTER	0x02
#define TMP100_HIGH_REGISTER	0x03

struct tmp100_data {
	dev_t dev;
	struct i2c_client *master_client;
	struct class *dev_class;
	struct cdev tmp100_cdev;
	struct regmap *regmap;
};
//struct tmp100_data tmp100_data;		//use container_of to get rid of global

static ssize_t tmp100_print(struct file *filp, char __user *buf,
				size_t len, loff_t *off)
{
	//add here regmap_read
	unsigned int regval;
	int err, count;
	s16 regval_signed;
	char temp[BUFSIZE];

	struct tmp100_data *data = filp->private_data;

	err = regmap_read(data->regmap, READ_REGISTER, &regval);
	if (err < 0)
		return err;

	regval_signed = regval;
	snprintf(temp, sizeof(temp), "%d.%04d\n", regval_signed>>8, (((regval_signed>>4)&0xf)*10000)>>4);

	temp[BUFSIZE -1] = '\0';
	count = strlen(temp);

	/*we need to read full size temp*/
	if (len < count)
		return -EINVAL;

	/*check if there is no more data to be read*/
	if (*off != 0)
		return 0;

	//copy_to_user
	if (copy_to_user(buf, temp, count));
		return -EINVAL;

	*off = count;
	return count;
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

	struct tmp100_data *data;
	data = kzalloc(sizeof(struct tmp100_data), GFP_KERNEL);
	if (!data) {
		pr_err("ERROR!! No memmory!\n");
		goto exit;
	}

	data->master_client = client;
	data->dev = 0;
	data->regmap = devm_regmap_init_i2c(client, &tmp100_regmap_config);

	if (IS_ERR(data->regmap))
		return PTR_ERR(data->regmap);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C_FUNC_I2C not supported\n");
		return -ENODEV;
	}

	/*Allocating major number*/
	if ((alloc_chrdev_region(&data->dev, 0, 1, DRIVER_NAME)) < 0) {
		pr_err("Cannot alocate major number for device 1\n");
		return -1;
	}

	/*Creating cdev structire*/
	cdev_init(&data->tmp100_cdev, &fops);

	/*Adding character device to the system*/
	if ((cdev_add(&data->tmp100_cdev, data->dev, 1)) < 0) {
		pr_info("Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	data->dev_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (data->dev_class == NULL) {
		pr_err("Cannot create the struct class for device\n");
		goto r_class;
	}

	/*Creating the device*/
	if ((device_create(data->dev_class, NULL, data->dev, NULL,
					DRIVER_NAME)) == NULL) {
		pr_err("Cannot create the Device\n");
		goto r_device;
	}
	pr_alert("Kernel Driver Inserted Successfully...\n");

	return 0;

exit:
	kfree(data);
r_device:
	class_destroy(data->dev_class);
r_class:
	unregister_chrdev_region(data->dev, 1);

	return -1;
};
static int tmp100_remove(struct i2c_client *client)
{
	struct tmp100_data *data = i2c_get_clientdata(client);

	device_destroy(data->dev_class, data->dev);
	class_destroy(data->dev_class);
	cdev_del(&data->tmp100_cdev);
	unregister_chrdev_region(data->dev, 1);

	kfree(data);

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
