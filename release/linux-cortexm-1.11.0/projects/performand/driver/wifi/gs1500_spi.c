#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#define SPI_BUFF_SIZE	16
#define USER_BUFF_SIZE	128

#define SPI_BUS 0
#define SPI_BUS_CS1 0
#define SPI_BUS_SPEED 1000000


const char this_driver_name[] = "spike";
#define DRIVER_NAME "spike"

static uint spike_major = 168;

module_param(spike_major, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(spike_major, "spike driver major number");

struct spike_control {
	struct spi_message msg;
	struct spi_transfer transfer;
	u8 *tx_buff; 
	u8 *rx_buff;
};

static struct spike_control spike_ctl;

struct spike_dev {
	struct semaphore spi_sem;
	struct semaphore fop_sem;
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct spi_device *spi_device;
	char *user_buff;
	u8 test_data;
};

static struct spike_dev spike_dev;

static void spike_prepare_spi_message(void)
{
	spi_message_init(&spike_ctl.msg);

	/* put some changing values in tx_buff for testing */	
	spike_ctl.tx_buff[0] = 'A';
	spike_ctl.tx_buff[1] = 'T';
	spike_ctl.tx_buff[2] = '+';
	spike_ctl.tx_buff[3] = 'V';
	spike_ctl.tx_buff[4] = 'E';
	spike_ctl.tx_buff[5] = 'R';
	spike_ctl.tx_buff[6] = '=';
	spike_ctl.tx_buff[7] = '?';
	spike_ctl.tx_buff[8] = 0x0A;
	spike_ctl.tx_buff[9] = 0x0D;

	memset(spike_ctl.rx_buff, 0, SPI_BUFF_SIZE);

	spike_ctl.transfer.tx_buf = spike_ctl.tx_buff;
	spike_ctl.transfer.rx_buf = spike_ctl.rx_buff;
	spike_ctl.transfer.len = 9;

	spi_message_add_tail(&spike_ctl.transfer, &spike_ctl.msg);
}

static int spike_do_one_message(void)
{
	int status;

	if (down_interruptible(&spike_dev.spi_sem))
		return -ERESTARTSYS;

	if (!spike_dev.spi_device) {
		up(&spike_dev.spi_sem);
		return -ENODEV;
	}

	spike_prepare_spi_message();

	status = spi_sync(spike_dev.spi_device, &spike_ctl.msg);
	memset(spike_ctl.tx_buff, 0, SPI_BUFF_SIZE);

	status = spi_sync(spike_dev.spi_device, &spike_ctl.msg);
	
	up(&spike_dev.spi_sem);

	return status;	
}


static ssize_t spike_read(struct file *filp, char __user *buff, size_t count,
			loff_t *offp)
{
	size_t len;
	ssize_t status = 0;

	if (!buff) 
		return -EFAULT;

	if (*offp > 0) 
		return 0;

	if (down_interruptible(&spike_dev.fop_sem)) 
		return -ERESTARTSYS;

	status = spike_do_one_message();

	if (status) {
		sprintf(spike_dev.user_buff, 
			"spike_do_one_message failed : %d\n",
			status);
	}
	else {
		sprintf(spike_dev.user_buff, 
			"Status: %d\nTX: %d %d %d %d\nRX: %d %d %d %d\n",
			spike_ctl.msg.status,
			spike_ctl.tx_buff[0], spike_ctl.tx_buff[1], 
			spike_ctl.tx_buff[2], spike_ctl.tx_buff[3],
			spike_ctl.rx_buff[0], spike_ctl.rx_buff[1], 
			spike_ctl.rx_buff[2], spike_ctl.rx_buff[3]);
	}
		
	len = strlen(spike_dev.user_buff);
 
	if (len < count) 
		count = len;

	if (copy_to_user(buff, spike_dev.user_buff, count))  {
		printk(KERN_ALERT "spike_read(): copy_to_user() failed\n");
		status = -EFAULT;
	} else {
		*offp += count;
		status = count;
	}

	up(&spike_dev.fop_sem);

	return status;	
}

static int spike_open(struct inode *inode, struct file *filp)
{	
	int status = 0;

	if (down_interruptible(&spike_dev.fop_sem)) 
		return -ERESTARTSYS;

	if (!spike_dev.user_buff) {
		spike_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);
		if (!spike_dev.user_buff) 
			status = -ENOMEM;
	}	

	up(&spike_dev.fop_sem);

	return status;
}

static int spike_probe(struct spi_device *spi_device)
{
	if (down_interruptible(&spike_dev.spi_sem))
		return -EBUSY;

	spike_dev.spi_device = spi_device;

	up(&spike_dev.spi_sem);

	return 0;
}

static int spike_remove(struct spi_device *spi_device)
{
	if (down_interruptible(&spike_dev.spi_sem))
		return -EBUSY;
	
	spike_dev.spi_device = NULL;

	up(&spike_dev.spi_sem);

	return 0;
}

static int __init add_spike_device_to_bus(void)
{
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	struct device *pdev;
	char buff[64];
	int status = 0;

	spi_master = spi_busnum_to_master(SPI_BUS);
	if (!spi_master) {
		printk(KERN_ALERT "spi_busnum_to_master(%d) returned NULL\n",
			SPI_BUS);
		printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n");
		return -1;
	}

	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		printk(KERN_ALERT "spi_alloc_device() failed\n");
		return -1;
	}

	spi_device->chip_select = SPI_BUS_CS1;

	/* Check whether this SPI bus.cs is already claimed */
	snprintf(buff, sizeof(buff), "%s.%u", 
			dev_name(&spi_device->master->dev),
			spi_device->chip_select);

	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
 	if (pdev) {
		/* We are not going to use this spi_device, so free it */ 
		spi_dev_put(spi_device);
		
		/* 
		 * There is already a device configured for this bus.cs  
		 * It is okay if it us, otherwise complain and fail.
		 */
		if (pdev->driver && pdev->driver->name && 
				strcmp(this_driver_name, pdev->driver->name)) {
			printk(KERN_ALERT 
				"Driver [%s] already registered for %s\n",
				pdev->driver->name, buff);
			status = -1;
		} 
	} else {
		spi_device->max_speed_hz = SPI_BUS_SPEED;
		spi_device->mode = SPI_MODE_0;
		spi_device->bits_per_word = 8;
		spi_device->irq = -1;
		spi_device->controller_state = NULL;
		spi_device->controller_data = NULL;
		strlcpy(spi_device->modalias, this_driver_name, SPI_NAME_SIZE);
		
		status = spi_add_device(spi_device);		
		if (status < 0) {	
			spi_dev_put(spi_device);
			printk(KERN_ALERT "spi_add_device() failed: %d\n", 
				status);		
		}				
	}

	put_device(&spi_master->dev);

	return status;
}

static struct spi_driver spike_driver = {
	.driver = {
		.name =	this_driver_name,
		.owner = THIS_MODULE,
	},
	.probe = spike_probe,
	.remove = __devexit_p(spike_remove),	
};

static int __init spike_init_spi(void)
{
	int error;


	spike_ctl.tx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!spike_ctl.tx_buff) {
		error = -ENOMEM;
		goto spike_init_error;
	}

	spike_ctl.rx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!spike_ctl.rx_buff) {
		error = -ENOMEM;
		goto spike_init_error;
	}

	error = spi_register_driver(&spike_driver);
	if (error < 0) {
		printk(KERN_ALERT "spi_register_driver() failed %d\n", error);
		return error;
	}

	error = add_spike_device_to_bus();
	if (error < 0) {
		printk(KERN_ALERT "add_spike_to_bus() failed\n");
		spi_unregister_driver(&spike_driver);
		return error;
	}

	return 0;

spike_init_error:

	if (spike_ctl.tx_buff) {
		kfree(spike_ctl.tx_buff);
		spike_ctl.tx_buff = 0;
	}

	if (spike_ctl.rx_buff) {
		kfree(spike_ctl.rx_buff);
		spike_ctl.rx_buff = 0;
	}
	
	return error;
}

static const struct file_operations spike_fops = {
	.owner =	THIS_MODULE,
	.read = 	spike_read,
	.open =		spike_open,	
};

static int __init spike_init_cdev(void)
{

int ret = 0;

	if (spike_major == 0) {
		printk(KERN_ALERT "%s: hih6130_major can't be 0\n", __func__);
		ret = -EINVAL;
		goto Done;
	}

	ret = register_chrdev(spike_major, DRIVER_NAME, &spike_fops);
	if (ret < 0) {
		printk(KERN_ALERT "%s: registering device %s with major %d "
				  "failed with %d\n",
		       __func__, DRIVER_NAME, spike_major, ret);
		goto Done;
	}


/*	int error;

	spike_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&spike_dev.devt, 0, 1, this_driver_name);
	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", 
			error);
		return -1;
	}

	cdev_init(&spike_dev.cdev, &spike_fops);
	spike_dev.cdev.owner = THIS_MODULE;
	
	error = cdev_add(&spike_dev.cdev, spike_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(spike_dev.devt, 1);
		return -1;
	}	
*/
Done:
	return 0;
}

static int __init spike_init_class(void)
{
	spike_dev.class = class_create(THIS_MODULE, this_driver_name);

	if (!spike_dev.class) {
		printk(KERN_ALERT "class_create() failed\n");
		return -1;
	}

	if (!device_create(spike_dev.class, NULL, spike_dev.devt, NULL, 	
			this_driver_name)) {
		printk(KERN_ALERT "device_create(..., %s) failed\n",
			this_driver_name);
		class_destroy(spike_dev.class);
		return -1;
	}

	return 0;
}

static int __init spike_init(void)
{
	memset(&spike_dev, 0, sizeof(spike_dev));
	memset(&spike_ctl, 0, sizeof(spike_ctl));

	sema_init(&spike_dev.spi_sem, 1);
	sema_init(&spike_dev.fop_sem, 1);
	
	if (spike_init_cdev() < 0) 
		goto fail_1;
	
	//if (spike_init_class() < 0)  
	//	goto fail_2;

	if (spike_init_spi() < 0) 
		goto fail_3;

	return 0;

fail_3:
	device_destroy(spike_dev.class, spike_dev.devt);
	class_destroy(spike_dev.class);

fail_2:
	cdev_del(&spike_dev.cdev);
	unregister_chrdev_region(spike_dev.devt, 1);

fail_1:
	return -1;
}
module_init(spike_init);

static void __exit spike_exit(void)
{
	spi_unregister_device(spike_dev.spi_device);
	spi_unregister_driver(&spike_driver);

	//device_destroy(spike_dev.class, spike_dev.devt);
	//class_destroy(spike_dev.class);

	unregister_chrdev(hih6130_major, DRIVER_NAME);

	cdev_del(&spike_dev.cdev);
	unregister_chrdev_region(spike_dev.devt, 1);

	if (spike_ctl.tx_buff)
		kfree(spike_ctl.tx_buff);

	if (spike_ctl.rx_buff)
		kfree(spike_ctl.rx_buff);


	if (spike_dev.user_buff)
		kfree(spike_dev.user_buff);
}
module_exit(spike_exit);


MODULE_AUTHOR("Scott Ellis");
MODULE_DESCRIPTION("spike module - an example SPI driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");



