////////////////////////////////////////////////////////////////////////////
//
//  This file is part of linux-mpu9150
//
//  Copyright (c) 2013 Pansenti, LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "i2c_lib.h"

#define NUM_DEVICES 2
#define MAX_INTERRUPT_CALLBACK_MEMBERS 5
//struct tasklet_struct i2c_tasklet;
void (*read_fn_ptr)(void (*data));
void (*mpu_data_t);

static struct i2c_client *imu_i2c_client[NUM_DEVICES];
static struct i2c_board_info imu_board_info[NUM_DEVICES] = {
							      {
								I2C_BOARD_INFO("MPU9150", 0x68),
							      },
							      {
								I2C_BOARD_INFO("AK8975", 0x0C), 
							      },
							   };

//*** I2C BUS HANDLING FUNCTIONS ***//
static struct i2c_driver imu_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	/*.shutdown = i2c_shutdown,
	.suspend = i2c_suspend,
	.resume = i2c_resume,*/
};

/*static int i2c_shutdown(void)
{
	if (mpu9150_suspend())
		return -1;
	return 0;
}

static int i2c_suspend(void)
{
	if(mpu9150_suspend())
		return -1;
	return 0;
}

static int i2c_resume(void)
{
	if(mpu9150_resume())
		return -1;
	return 0;
}
*/

int __init i2c_init(void)
{
	int ret, i, j;
	struct i2c_adapter *adapter;

	/* register our driver */
	ret = i2c_add_driver(&imu_i2c_driver);
	if (ret) {
		printk(KERN_ALERT "Error registering i2c driver\n");
		return ret;
	}

	/* add our device */
	adapter = i2c_get_adapter(I2C_BUS);
	if (!adapter) {
		printk(KERN_ALERT "i2c_get_adapter(%d) failed\n", I2C_BUS);
		goto init_i2c_fail_1;
	}

	for (i=0; i<NUM_DEVICES; i++) {

		imu_i2c_client[i] = i2c_new_device(adapter, &imu_board_info[i]);

		if (!imu_i2c_client[i]) {
			printk(KERN_ALERT "i2c_new_device failed\n");
			goto init_i2c_fail_2;
		}
	}

	i2c_put_adapter(adapter);

	return 0;

init_i2c_fail_2:
	for (j=0; j<i; j++){
		i2c_unregister_device(imu_i2c_client[j]);
	}
init_i2c_fail_1:
	i2c_del_driver(&imu_i2c_driver);
	return -1;
}

void __exit i2c_cleanup(void)
{
	int i;

	for (i=0; i<NUM_DEVICES; i++){
		i2c_unregister_device(imu_i2c_client[i]);
		imu_i2c_client[i] = NULL;
	}

	i2c_del_driver(&imu_i2c_driver);
}

unsigned char i2c_get_addr(unsigned int device_id)
{
	if (device_id >= NUM_DEVICES)
		return -EINVAL;
	if (!imu_i2c_client[device_id])
		return -ENODEV;

	return imu_i2c_client[device_id]->addr;
}

unsigned int i2c_get_id_by_name(char *name)
{
	int i;
	for(i=0; i<=NUM_DEVICES; i++){
		if(strcmp(name, imu_i2c_client[i]->name) == 0)
			return i;
	}
	return -1;
}

int i2c_write(unsigned int device_id, unsigned char reg_addr, unsigned char length, char *data)
{
	int result, i;
	char i2c_buff[MAX_WRITE_LEN + 1];

	if (device_id >= NUM_DEVICES) {
		printk(KERN_WARNING "Device ID %d invalid. Valid device ID's: 0-%d\n", device_id, NUM_DEVICES);
		return -EINVAL;
	}

	if (length > MAX_WRITE_LEN) {
		printk(KERN_WARNING "Max write length exceeded in i2c_write()\n");
		return -1;
	}

	if (length == 0) {
		result = i2c_master_send(imu_i2c_client[device_id], &reg_addr, 1);

		if (result < 0) {
			printk(KERN_WARNING "Write 0: Negative retval\n");
			return -1;
		}
		else if (result != 1) {
			printk(KERN_WARNING "Write 0: Tried 1 Wrote 0\n");
			return -1;
		}
	}
	else {
		i2c_buff[0] = reg_addr;

		for (i = 0; i < length; i++)
			i2c_buff[i+1] = data[i];

		result = i2c_master_send(imu_i2c_client[device_id], i2c_buff, length + 1);

		if (result < 0) {
			printk(KERN_WARNING "Write 1: Negative retval");
			return -1;
		}
		else if (result < (int)length) {
			printk(KERN_WARNING "Write 1: Tried %u Wrote %d\n", length, result); 
			return -1;
		}
	}

	return 0;
}

int i2c_read(unsigned int device_id, unsigned char reg_addr, unsigned char length, char *data)
{
	int result;

	result = i2c_write(device_id, reg_addr, 0, NULL);
	if (result < 0)
		return -1;
	

	result = i2c_master_recv(imu_i2c_client[device_id], data, length);
	if (result < length)
		return -1;

	return 0;
}

