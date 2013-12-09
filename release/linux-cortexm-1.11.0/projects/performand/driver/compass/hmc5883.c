
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#include "hmc5883.h"

#define USER_BUFF_SIZE 128
char device_name[] = "HMC5883 Magnetometer";

static uint hmc_major = 168;

struct hmc5883_dev {
	dev_t devt;
	struct cdev cdev;
	struct semaphore sem;
	struct class *class;
	char *user_buff;
	unsigned char data_string[50];
};

static struct hmc5883_dev hmc5883_dev;
static int cnt = 0;

//I2C specific fields:
static struct i2c_client *hmc5883_i2c_client;
static struct i2c_board_info hmc5883_board_info = {I2C_BOARD_INFO(DRIVER_NAME, I2C_ADDRESS)};

int TwosCToDec(int val) {
	const int negative = (val & (1 << 15)) != 0;
	int retVal;

	if (negative)
		retVal = val | ~((1 << 16) - 1);
	else
		retVal = val;

	return retVal;
}

int init_registers(void) {
	unsigned char dat[5];
	
	dat[0] = 0x02; 
	dat[1] = 0x03; 
	i2c_master_send(hmc5883_i2c_client, dat,2);

	return 1;
}

static int hmc5883_read(struct file *filp, const char *user_buff, size_t count, loff_t *offp)
{
	int status = 0;

	if(hmc5883_dev.data_string[cnt]!='\0') {
		copy_to_user(user_buff, &hmc5883_dev.data_string[cnt], 1);
		status = 1;
		cnt++;
	}
	else {
		status = 0;
		cnt=0;	
	}
   
	return status;
}

static int hmc5883_open(struct inode *inode, struct file *filp)
{	
	signed int temp,x,y,z = 0;
	int status = 0;
	unsigned char dat[6];

	if (down_interruptible(&hmc5883_dev.sem)) 
		return -ERESTARTSYS;

	dat[0] = 0x02; 
	dat[1] = 0x01; 
	i2c_master_send(hmc5883_i2c_client, dat,2);
	
	dat[0] = 0x03;
	i2c_master_send(hmc5883_i2c_client, dat,1);

	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		x = (dat[0]<<8);
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	dat[0] = 0x04;
	i2c_master_send(hmc5883_i2c_client, dat,1);
	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		x |= dat[0];
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	dat[0] = 0x05;
	i2c_master_send(hmc5883_i2c_client, dat,1);
	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		z = (dat[0]<<8);
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	dat[0] = 0x06;
	i2c_master_send(hmc5883_i2c_client, dat,1);
	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		z |= dat[0];
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	dat[0] = 0x07;
	i2c_master_send(hmc5883_i2c_client, dat,1);
	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		y = (dat[0]<<8);
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	dat[0] = 0x08;
	i2c_master_send(hmc5883_i2c_client, dat,1);
	if(i2c_master_recv(hmc5883_i2c_client, dat, 1)){
		printk("dat[0]: %x\n", dat[0]);
		y |= dat[0];
        }
	else {
		printk(KERN_ERR "Error reading %s!\r\n", device_name);
		return -1;
	}

	
	sprintf(hmc5883_dev.data_string,"x: %d,y: %d,z: %d, \n\r",TwosCToDec(x),TwosCToDec(y),TwosCToDec(z)); 

	cnt = 0;
	up(&hmc5883_dev.sem);

	return status;
}

static const struct file_operations hmc5883_fops = {
	.owner = THIS_MODULE,
	.open =	hmc5883_open,	
	.read =	hmc5883_read,
};


//*** SPI BUS HANDLING FUNCTIONS ***//
static struct i2c_driver hmc5883_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

#define I2C_BUS		0
int __init hmc5883_init_i2c(void)
{
	int ret;
	struct i2c_adapter *adapter;

	printk("Register I2C driver for address 0x%x ...\t", I2C_ADDRESS);

	/* register our driver */
	ret = i2c_add_driver(&hmc5883_i2c_driver);
	if (ret) {
		printk(KERN_ALERT "Error registering i2c driver\n");
		return ret;
	}
	/* add our device */
	adapter = i2c_get_adapter(I2C_BUS);

	if (!adapter) {
		printk(KERN_ALERT "i2c_get_adapter(%d) failed\n", I2C_BUS);
		return -1;
	}

	hmc5883_i2c_client = i2c_new_device(adapter, &hmc5883_board_info); 

			
	if (!hmc5883_i2c_client) {
		printk(KERN_ALERT "i2c_new_device failed\n");
		return -1;
	}

	i2c_put_adapter(adapter);	
	printk("DONE\n\r");


	printk("Initialising %s...", device_name);

	if(!init_registers()) {
		printk("Failed to init registers in %s.\r\n", device_name);	
		return -1;
	}

	printk("Done\n\r");

	return 0;
}

void __exit hmc5883_cleanup_i2c(void)
{
	i2c_unregister_device(hmc5883_i2c_client);
	hmc5883_i2c_client = NULL;

	i2c_del_driver(&hmc5883_i2c_driver);
}

//*** DRIVER HANDLING FUNCTIONS ***//
static int __init hmc5883_init_cdev(void)
{
	int error;

	if (hmc_major == 0) {
		printk(KERN_ALERT "%s: imu_major can't be 0\n", __func__);
		//error = -EINVAL;
		return -EINVAL;
	}

	error = register_chrdev(hmc_major, DRIVER_NAME, &hmc5883_fops);
	if (error < 0) {
		printk(KERN_ALERT "%s: registering device %s with major %d "
				  "failed with %d\n",
		       __func__, DRIVER_NAME, hmc_major, error);
		return -EINVAL;
	}


	/*hmc5883_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&hmc5883_dev.devt, 0, 1, DRIVER_NAME);
	if (error < 0) {
		printk(KERN_ALERT 
			"alloc_chrdev_region() failed: error = %d \n", 
			error);
		
		return -1;
	}

	cdev_init(&hmc5883_dev.cdev, &hmc5883_fops);
	hmc5883_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&hmc5883_dev.cdev, hmc5883_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: error = %d\n", error);
		unregister_chrdev_region(hmc5883_dev.devt, 1);
		return -1;
	}	*/

	return 0;
}

static int __init hmc5883_init_class(void)
{
	hmc5883_dev.class = class_create(THIS_MODULE, DRIVER_NAME);

	if (!hmc5883_dev.class) {
		printk(KERN_ALERT "class_create(mi2c) failed\n");
		return -1;
	}

	if (!device_create(hmc5883_dev.class, NULL, hmc5883_dev.devt, 
				NULL, DRIVER_NAME)) {
		class_destroy(hmc5883_dev.class);
		return -1;
	}

	return 0;
}

static int __init hmc5883_init(void)
{
	printk(KERN_INFO "*******************************************************\n");
	printk(KERN_INFO "* HMC5883 - Magnetometer Driver v1.0.0                *\n");
	printk(KERN_INFO "* Mads Clausen Institute - Sydansk Universitet 2012   *\n");
	printk(KERN_INFO "* Author: Robert Brehm                                *\n");
	printk(KERN_INFO "* All rights reserved!                                *\n");
	printk(KERN_INFO "*******************************************************\n");
	printk(KERN_INFO "\n");

	memset(&hmc5883_dev, 0, sizeof(struct hmc5883_dev));

	sema_init(&hmc5883_dev.sem, 1);

	if (hmc5883_init_cdev() < 0)
		goto init_fail_1;

	if (hmc5883_init_class() < 0)
		goto init_fail_2;

	if (hmc5883_init_i2c() < 0)
		goto init_fail_3;


	return 0;

init_fail_3:
	device_destroy(hmc5883_dev.class, hmc5883_dev.devt);
	class_destroy(hmc5883_dev.class);

init_fail_2:
	cdev_del(&hmc5883_dev.cdev);
	unregister_chrdev_region(hmc5883_dev.devt, 1);

init_fail_1:

	return -1;
}
module_init(hmc5883_init);

static void __exit hmc5883_exit(void)
{
	printk(KERN_INFO "mi2c_exit()\n");

	hmc5883_cleanup_i2c();

	device_destroy(hmc5883_dev.class, hmc5883_dev.devt);
  	class_destroy(hmc5883_dev.class);

	cdev_del(&hmc5883_dev.cdev);
	unregister_chrdev_region(hmc5883_dev.devt, 1);

	if (hmc5883_dev.user_buff)
		kfree(hmc5883_dev.user_buff);
}
module_exit(hmc5883_exit);


MODULE_AUTHOR("Robert Brehm");
MODULE_DESCRIPTION("HMC5883 I2C Driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");



