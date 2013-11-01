/*
 * Template for I2C driver on K70 (uClinux)
 */

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

/*
 * Driver verbosity level: 0->silent; >0->verbose
 */
static int hih6130_debug = 0;

#define DRIVER_NAME 	"imu"
#define I2C_ADDRESS	0x68

//i2c specific fields:
static struct i2c_client *test_i2c_client;
static struct i2c_board_info test_i2c_board_info = {I2C_BOARD_INFO(DRIVER_NAME, I2C_ADDRESS)};

static char data_str[128];
static char *data_end;

module_param(hih6130_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(hih6130_debug, "HIH-6130 driver verbosity level");

#define d_printk(level, fmt, args...)				\
	if (hih6130_debug >= level) printk(KERN_INFO "%s: " fmt,	\
					__func__, ## args)

static uint hih6130_major = 168;

module_param(hih6130_major, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(hih6130_major, "HIH-6130 driver major number");


static int hih6130_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	return ret;
}


static int hih6130_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

/* 
 * Device read
 */
static ssize_t hih6130_read(struct file *filp, char *buffer,
			 size_t length, loff_t * offset)
{
	char * addr;
	unsigned int len = 0;
	int ret = 0;
	int Hum, Temp;
	char dat[5];

	if (! access_ok(0, buffer, length)) {
		ret = -EINVAL;
		goto Done;
	}

	dat[0] = 0x75;
	//dat[1] = 0x75;
	if(i2c_master_send(test_i2c_client, dat,0)<0)
		printk(KERN_ALERT "%s: error writing to i2c device.\n", __func__);

	if(i2c_master_recv(test_i2c_client, dat, 1)<0)
		printk(KERN_ALERT "%s: error writing to i2c device.\n", __func__);
 
	//Hum = dat[3] << 6 | (dat[4] & 0x3f);
	//Temp = dat[2] << 6 | (dat[1] & 0x3f);
	//d_printk(2, "Hum: %06d Temp: %06d \n", Hum, Temp);

	sprintf(data_str,"%x;%x\n", dat[0], dat[1]);

	data_end = data_str + strlen(data_str);
	
	addr = data_str + *offset;

	/*
	 * Check for an EOF condition.
	 */
	if (addr >= data_end) {
		ret = 0;
		goto Done;
	}
		
	len = addr + length < data_end ? length : data_end - addr;
	strncpy(buffer, addr, len);
	*offset += len;
	ret = len;

Done:
	d_printk(3, "length=%d,len=%d,ret=%d\n", length, len, ret);
	return ret;
}

/* 
 * Device write
 */
static ssize_t hih6130_write(struct file *filp, const char *buffer,
			  size_t length, loff_t * offset)
{
	d_printk(3, "length=%d\n", length);
	return -EIO;
}

/*
 * Device operations
 */
static struct file_operations hih6130_fops = {
	.read = hih6130_read,
	.write = hih6130_write,
	.open = hih6130_open,
	.release = hih6130_release
};

static struct i2c_driver test_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

#define I2C_BUS		0
int __init test_init_i2c(void)
{
	int ret;
	struct i2c_adapter *adapter;

	d_printk(2, "%s: register I2C driver for address 0x%x ...\t", __func__, I2C_ADDRESS);

	/* register our driver */
	ret = i2c_add_driver(&test_i2c_driver);
	if (ret) {
		printk(KERN_ALERT "%s: error registering i2c driver\n", __func__);
		return ret;
	}
	/* add our device */
	adapter = i2c_get_adapter(I2C_BUS);

	if (!adapter) {
		printk(KERN_ALERT "%s: i2c_get_adapter(%d) failed\n", __func__, I2C_BUS);
		return -1;
	}

	test_i2c_client = i2c_new_device(adapter, &test_i2c_board_info); 

			
	if (!test_i2c_client) {
		printk(KERN_ALERT "%s: i2c_new_device failed\n", __func__);
		return -1;
	}

	i2c_put_adapter(adapter);	

	return 0;
}

static int __init hih6130_init_module(void)
{
	int ret = 0;

	if (hih6130_major == 0) {
		printk(KERN_ALERT "%s: hih6130_major can't be 0\n", __func__);
		ret = -EINVAL;
		goto Done;
	}

	ret = register_chrdev(hih6130_major, DRIVER_NAME, &hih6130_fops);
	if (ret < 0) {
		printk(KERN_ALERT "%s: registering device %s with major %d "
				  "failed with %d\n",
		       __func__, DRIVER_NAME, hih6130_major, ret);
		goto Done;
	}

	if (test_init_i2c() < 0) {
		printk(KERN_ALERT "%s: Registering i2c device failed.",__func__);
		unregister_chrdev(hih6130_major, DRIVER_NAME);
		return -1;
	}

	
	
Done:
	d_printk(1, "name=%s,major=%d\n", DRIVER_NAME, hih6130_major);

	return ret;
}
static void __exit hih6130_cleanup_module(void)
{

	unregister_chrdev(hih6130_major, DRIVER_NAME);

	i2c_unregister_device(test_i2c_client);
	test_i2c_client = NULL;
	i2c_del_driver(&test_i2c_driver);

	d_printk(1, "%s\n", "clean-up successful");
}

module_init(hih6130_init_module);
module_exit(hih6130_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Brehm, brehm@mci.sdu.dk");
MODULE_DESCRIPTION("HIH-6130/6131 - Digital Humidity/Temperature Sensor Driver");
