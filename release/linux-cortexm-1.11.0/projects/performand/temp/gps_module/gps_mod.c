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

static int gps_debug = 0;

#define DRIVER_NAME 	"gps"

static char data_str[128];
static char *data_end;

module_param(gps_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(gps_debug, "GPS driver verbosity level");

#define d_printk(level, fmt, args...)				\
	if (gps_debug >= level) printk(KERN_INFO "%s: " fmt,	\
					__func__, ## args)

static uint gps_major = 170;

module_param(gps_major, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(gps_major, "GPS driver major number");


static int gps_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	return ret;
}


static int gps_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t gps_read(struct file *filp, char *buffer,
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


	
	sprintf(data_str,"%d;%d\n", Hum, Temp);

	data_end = data_str + strlen(data_str);
	
	addr = data_str + *offset;

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

static ssize_t gps_write(struct file *filp, const char *buffer,
			  size_t length, loff_t * offset)
{
	d_printk(3, "length=%d\n", length);
	return -EIO;
}

static struct file_operations gps_fops = {
	.read = gps_read,
	.write = gps_write,
	.open = gps_open,
	.release = gps_release
};

static int __init gps_init_module(void)
{
	int ret = 0;

	if (gps_major == 0) {
		printk(KERN_ALERT "%s: gps_major can't be 0\n", __func__);
		ret = -EINVAL;
		goto Done;
	}

	ret = register_chrdev(gps_major, DRIVER_NAME, &gps_fops);
	if (ret < 0) {
		printk(KERN_ALERT "%s: registering device %s with major %d "
				  "failed with %d\n",
		       __func__, DRIVER_NAME, gps_major, ret);
		goto Done;
	}


	
	
Done:
	d_printk(1, "name=%s,major=%d\n", DRIVER_NAME, gps_major);

	return ret;
}
static void __exit gps_cleanup_module(void)
{

	unregister_chrdev(gps_major, DRIVER_NAME);

	d_printk(1, "%s\n", "clean-up successful");
}

module_init(gps_init_module);
module_exit(gps_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Brehm, brehm@mci.sdu.dk");
MODULE_DESCRIPTION("Test driver - GPS via serial port.");
