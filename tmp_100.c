//SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>	/*file operation, open/close, read/write to device*/
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#define BYTES_TO_READ 4
#define READ_REGISTER 0x00

struct i2c_client *master_client;
dev_t dev = 0;
static struct class *dev_class;
/*add file_operations*/
static struct cdev etx_cdev;

/*try to read*/
static s32 tmp100_read(struct i2c_client *client, u8 buf) {
	s32 byte_data;

	byte_data = i2c_smbus_read_byte_data(client, buf);
	if (byte_data < 0){
		return -EIO;
	}
	else{
		return byte_data;
	}
}

static ssize_t tmp100_file_operation(struct file *filp, char __user *buf,
									size_t len, loff_t *off){
	pr_alert("The temperature is %d C\n", tmp100_read(master_client, READ_REGISTER));
	return 0;
}

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.read           = tmp100_file_operation,
};

static int tmp100_probe(struct i2c_client *client,
						const struct i2c_device_id *id) {

	int error;
	u8 val[BYTES_TO_READ];
	struct i2c_adapter *adapter = client->adapter;
	master_client = client;

	pr_alert("I have registered PROBE!!!\n");

	error = i2c_check_functionality(adapter, I2C_FUNC_I2C);
	if (error < 0) {
		return -ENODEV;
	}
	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0) {
		return -ENODEV;
	}

	if ((alloc_chrdev_region(&dev, 0, 1, "ilian_driver")) < 0) {
		pr_alert("Cannot alocate major number for device 1\n");
		return -1;
	}
	pr_alert("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&etx_cdev, &fops);

	/*Adding character device to the system*/
	if((cdev_add(&etx_cdev, dev, 1)) < 0){
		printk(KERN_INFO "Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	if ((dev_class = class_create(THIS_MODULE,"tmp100_class")) == NULL){
		printk(KERN_INFO "Cannot create the struct class for device\n");
		goto r_class;
	}

	/*Creating device*/
	if((device_create(dev_class, NULL, dev, NULL, "tmp100_device")) == NULL){
		printk(KERN_INFO "Cannot create the Device\n");
		goto r_device;
	}
	pr_alert("Kernel Driver Inserted Successfully...\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
};
static int tmp100_remove(struct i2c_client *client) {

	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&etx_cdev);
	unregister_chrdev_region(dev, 1);

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
