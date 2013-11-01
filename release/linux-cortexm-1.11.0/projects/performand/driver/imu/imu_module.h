#ifndef IMU_MODULE_H
#define IMU_MODULE_H

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
#include <linux/delay.h>

#include "local_defaults.h"
#include "i2c_lib.h"
#include "mpu9150/mpu9150.h"

#define USER_BUFF_SIZE 200

struct imu_dev {
	dev_t devt;
	struct cdev cdev;
	struct semaphore sem;
	struct class *class;
	char *user_buff;
	int irq;
};

static struct imu_dev imu_dev;
//int _closing;

//static void irq_worker_function(struct work_struct *work_ptr);
//struct work_struct irq_work;
//struct workqueue_struct *irq_workqueue;
//DECLARE_WORK(irq_queued_work, irq_worker_function);


MODULE_AUTHOR("Rasmus Koldsoe");
MODULE_DESCRIPTION("MPU9150 9 DOF Motion Sensor Driver using DMP");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");

#endif
