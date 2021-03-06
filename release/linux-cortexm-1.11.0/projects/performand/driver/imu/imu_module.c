
#include "imu_module.h"

unsigned char d_string[600];
static int cnt = 0;
mpudata_t mpu_data;

static uint imu_major = 168;

module_param(imu_major, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(imu_major, "IMU driver major number");

//----- Read data ----------------
int read_data(void)
{
	int ret;
//	gpio_set_value(G_PIN, 1);
	ret = mpu9150_read(&mpu_data);
//	gpio_set_value(G_PIN, 0);
	return ret;
}

/****** INTERRUPT HANDLING ******
void imu_enable_interrupt(void)
{
	if(!imu_dev.irq)
		printk("WARNING: No irq defined in board_info\n");
	enable_irq(imu_dev.irq);
}

void imu_disable_interrupt(void)
{
	if(!imu_dev.irq)
		printk("WARNING: No irq defined in board_info\n");
	disable_irq(imu_dev.irq);
}

static void irq_worker_function(struct work_struct *work_ptr)
{
	printk("±");
	mpu9150_read(&mpu_data);
	return;
}

static irqreturn_t imu_irq_handler(int irq, void *dev_id)
{
	if (irq == imu_dev.irq) {
		printk("§");
		//if(irq_workqueue) {
			queue_work( irq_workqueue, &irq_queued_work );
		//}
		printk("@");
	}
	
	return IRQ_HANDLED;
}

static void read_scheduled(void)
{
	printk("±");
	while(!_closing) {
		msleep( DEFAULT_WAIT_MS );
		printk("§");
	}
	_closing++;
	return;
}*/

/****** OPEN AND READ FUNCTIONS ******/
static ssize_t imu_read(struct file *filp, char __user *user_buff, size_t count, loff_t *offp)
{
	int status = 0;	

	if(cnt == 0) {
		if(read_data() < 0) {
			//snprintf(success_string, 40, "\nFailed reading from device\n");
			mpu9150_print_data(NULL, d_string);
		}
		else {
			//snprintf(success_string, 40, "\nSuccessfully read from device\n");
			mpu9150_print_data(&mpu_data, d_string);
		}
	}

	if(d_string[cnt]!='\0') {
		copy_to_user(user_buff, &d_string[cnt], 1);
		status = 1;
		cnt++;
	}
	else {
		status = 0;
		cnt=0;	
	}
    
	return status;
}

static int imu_open(struct inode *inode, struct file *filp)
{
	if (down_interruptible(&imu_dev.sem)) {
		return -ERESTARTSYS;
	}
	
	if (!imu_dev.user_buff) {
		imu_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

		if (!imu_dev.user_buff) {
			printk(KERN_ALERT "imu_open: user_buff alloc failed\n");
			up(&imu_dev.sem);
			return -ENOMEM;
		}
	}

	return 0;
}

static int imu_close(struct inode *inode, struct file *filep)
{
	up(&imu_dev.sem);

	return 0;
}

/****** INITIALIZATION OF MPU *******/
static int __init imu_init_mpu(void)
{
//	int ret;
//--- Init i2c ----------------
	if(i2c_init() < 0)
		return -1;
	printk("Succesfully registered i2c driver \n");

//--- Init MPU9150 ------------
	if (mpu9150_init(DEFAULT_SAMPLE_RATE_HZ, DEFAULT_YAW_MIX_FACTOR) < 0)
		goto imu_init_i2c_fail;
	printk("Succesfully set up mpu9150 \n");

//--- Init scheduling of read ---


//--- Init workqueue -----------
/*	printk("Initializing workqueue... ");
	if(!irq_workqueue) {
		irq_workqueue = create_singlethread_workqueue("IRQ Workqueue");
		printk("A workqueue has been created\n");
		irq_worker = (irq_work_t *)kmalloc(sizeof(irq_work_t), GFP_KERNEL);
		memset(irq_worker, 0, sizeof(irq_work_t));
		if(irq_worker) {
			printk("A worker has been created ");
			INIT_WORK( (struct work_struct *)irq_worker->work, irq_worker_function);
			printk("DONE\n");
		} else
			goto imu_init_workqueue_fail;
	} else
		goto imu_init_mpu_fail;
*/
//--- Init GPIO ---------------
/*	printk("Initialising IRQ Pin for I2C... ");	
	if (gpio_request(IRQ_PIN, "I2C interrupt")) {
		printk(KERN_ALERT "gpio_request failed\n");
		goto imu_init_workqueue_fail;
	}
	if (gpio_direction_input(IRQ_PIN)) {
		printk(KERN_ALERT "gpio_direction_input IRQ_PIN failed\n");
		goto imu_init_gpio_fail;
	}	

	if (gpio_request(G_PIN, "I2C interrupt")) {
		printk(KERN_ALERT "gpio_request failed\n");
		goto imu_init_workqueue_fail;
	}
	if (gpio_direction_output(G_PIN, 0)) {
		printk(KERN_ALERT "gpio_direction_output G_PIN failed\n");
		goto imu_init_gpio_fail;
	}
	printk("DONE\n");
*/
//--- Requesting IRQ ----------
/*	printk("Request device IRQ from pin... ");
	imu_dev.irq = OMAP_GPIO_IRQ(IRQ_PIN);
	printk("%d\nRequest irq ", imu_dev.irq);
	
	DECLARE_COMPLETION_ONSTACK(done);
	ret = request_irq(	imu_dev.irq, 
				imu_irq_handler,
				IRQF_TRIGGER_RISING,
				"imu_i2c_irq",
				&imu_dev);

	if (ret < 0) {
		printk(KERN_ALERT "failed: %d\n", ret);
		goto imu_init_gpio_fail;
	}
*/
	printk("Initialization DONE\n");
	return 0;

//--- Fail labels -----------------

//imu_init_irq_fail:
//	free_irq(imu_dev.irq, &imu_dev);
//imu_init_gpio_fail:
//	gpio_free(IRQ_PIN);
//	gpio_free(G_PIN);
//imu_init_workqueue_fail:
//	flush_workqueue( &workqq );
//	destroy_workqueue( &wq );
//	kfree(irq_worker);
//imu_init_mpu_fail:
//	mpu9150_exit();
imu_init_i2c_fail:
	i2c_cleanup();
	return -1;
}

void __exit imu_cleanup(void)
{
	if (imu_dev.user_buff)
		kfree(imu_dev.user_buff);

//	free_irq(imu_dev.irq, &imu_dev);
//	gpio_free(IRQ_PIN);
//	gpio_free(G_PIN);
//	flush_workqueue( &wq );
//	destroy_workqueue( &wq );
//	kfree(irq_worker);
	mpu9150_exit();
	i2c_cleanup();
}

/****** DRIVER HANDLING FUNCTIONS ******/
static const struct file_operations imu_fops = {
	.owner = THIS_MODULE,
	.open =	imu_open,
	.release = imu_close,
	.read =	imu_read,
};

static int __init imu_init_cdev(void)
{
	int error;

	if (imu_major == 0) {
		printk(KERN_ALERT "%s: imu_major can't be 0\n", __func__);
		//error = -EINVAL;
		return -EINVAL;
	}

	error = register_chrdev(imu_major, DRIVER_NAME, &imu_fops);
	if (error < 0) {
		printk(KERN_ALERT "%s: registering device %s with major %d "
				  "failed with %d\n",
		       __func__, DRIVER_NAME, imu_major, error);
		return -EINVAL;
	}
	return 0;


/*	imu_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&imu_dev.devt, 0, 1, DRIVER_NAME);
	if (error < 0) {
		printk(KERN_ALERT 
			"alloc_chrdev_region() failed: error = %d \n", 
			error);
		
		return -1;
	}

	cdev_init(&imu_dev.cdev, &imu_fops);
	imu_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&imu_dev.cdev, imu_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: error = %d\n", error);
		unregister_chrdev_region(imu_dev.devt, 1);
		return -1;
	}	

	return 0; */
}

/*
static int __init imu_init_class(void)
{
	imu_dev.class = class_create(THIS_MODULE, DRIVER_NAME);

	if (!imu_dev.class) {
		printk(KERN_ALERT "class_create(mi2c) failed\n");
		return -1;
	}

	if (!device_create(imu_dev.class, NULL, imu_dev.devt, 
				NULL, DRIVER_NAME)) {
		class_destroy(imu_dev.class);
		return -1;
	}

	return 0;
} */

static int __init imu_init(void)
{
	printk(KERN_INFO "********************************************************\n");
	printk(KERN_INFO "* MPU9150 - 9 DOF Motion Sensor Driver v0.0.1          *\n");
	printk(KERN_INFO "* Mads Clausen Institute - Sydansk Universitet 2013    *\n");
	printk(KERN_INFO "* Author: Rasmus Koldsoe                               *\n");
        printk(KERN_INFO "* Based on Invensense Userspace DMP Driver             *\n");
	printk(KERN_INFO "********************************************************\n");
//	printk(KERN_INFO "\n");

//	_closing = 0;
	memset(&imu_dev, 0, sizeof(struct imu_dev));

	sema_init(&imu_dev.sem, 1);

	if (imu_init_cdev() < 0)
		goto init_fail_1;

//	if (imu_init_class() < 0)
//		goto init_fail_2;

	if (imu_init_mpu() < 0)
		goto init_fail_3;


	return 0;

init_fail_3:
	device_destroy(imu_dev.class, imu_dev.devt);
	class_destroy(imu_dev.class);

init_fail_2:
	cdev_del(&imu_dev.cdev);
	unregister_chrdev_region(imu_dev.devt, 1);

init_fail_1:

	return -1;
}
module_init(imu_init);

static void __exit imu_exit(void)
{
//	_closing = 1;
	imu_cleanup();

	device_destroy(imu_dev.class, imu_dev.devt);
  	class_destroy(imu_dev.class);

	cdev_del(&imu_dev.cdev);
	unregister_chrdev_region(imu_dev.devt, 1);

	if (imu_dev.user_buff)
		kfree(imu_dev.user_buff);

//	printk("Waiting for threads to terminate\n");
//	while(_closing <=1);

	printk(KERN_INFO "MPU9150 Driver exited\n");
}
module_exit(imu_exit);
