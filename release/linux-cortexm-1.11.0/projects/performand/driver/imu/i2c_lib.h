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

#ifndef LINUX_GLUE_H
#define LINUX_GLUE_H

//#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>

#include <mach/gpio.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <asm/irq.h>

#include "local_defaults.h"

#define MIN_I2C_BUS 0
#define MAX_I2C_BUS 7

#define I2C_BUS		0

#define MAX_WRITE_LEN 511

int __init i2c_init(void);
void __exit i2c_cleanup(void);
unsigned char i2c_get_addr(unsigned int device_id);
unsigned int i2c_get_id_by_name(char *name);
int i2c_write(unsigned int device_id, unsigned char reg_addr, unsigned char length, char *data);
int i2c_read(unsigned int device_id, unsigned char reg_addr, unsigned char length, char *data);
/*static int i2c_shutdown(void);
static int i2c_suspend(void);
static int i2c_resume(void);*/

#endif /* ifndef LINUX_GLUE_H */

