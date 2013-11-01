/*
 * i2c_a2f_dev.c - registration of I2C controllers
 * Author: Vladimir Khusainov, vlad@emcraft.com
 * Copyright 2012 Emcraft Systems
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <asm/clkdev.h>
#include <mach/platform.h>
#include <mach/a2f.h>
#include <mach/clock.h>
#include <mach/i2c.h>

/*
 * Service to print debug messages
 */
#define d_printk(level, fmt, args...)					\
	if (i2c_a2f_dev_debug >= level) printk(KERN_INFO "%s: " fmt,	\
					       __func__, ## args)

/*
 * Service to print error messages
 */
#define d_error(fmt, args...)						\
	printk(KERN_ERR "%s:%d,%s: " fmt, __FILE__, __LINE__, __func__,	\
	       ## args)

/*
 * Driver verbosity level: 0->silent; >0->verbose
 */
static int i2c_a2f_dev_debug = 0;

/*
 * User can change verbosity of the driver
 */
module_param(i2c_a2f_dev_debug, int, S_IRUSR);
MODULE_PARM_DESC(i2c_a2f_dev_debug, "I2C contollers "
				"registration verbosity level");

#define CONFIG_A2F_MSS_I2C0
#define CONFIG_A2F_MSS_I2C1

/*
 * Device release callback. Unless this is defined, the device framework
 * complains that there is no release entry point for this device
 */
static void i2c_a2f_dev_release(struct device * dev)
{
	d_printk(1, "%s\n", "ok");
}

/*
 * MSS I2C_0
 */
#if defined(CONFIG_A2F_MSS_I2C0)

#define I2C_A2F_DEV0_IRQ	14
#define I2C_A2F_DEV0_REGS	0x40002000

static struct resource i2c_a2f_dev0_resources[] = {
	{
		.start	= I2C_A2F_DEV0_IRQ,
		.end	= I2C_A2F_DEV0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= I2C_A2F_DEV0_REGS,
		.end	= I2C_A2F_DEV0_REGS + 0xFFF,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device i2c_a2f_dev0 = {
	.name           = "i2c_a2f",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(i2c_a2f_dev0_resources),
	.resource       = i2c_a2f_dev0_resources,
	.dev		= {
		.release = i2c_a2f_dev_release,
	},
};

static struct i2c_a2f_data i2c_a2f_data_dev0 = {
	.i2c_clk	= 100000,
};

#endif	/* CONFIG_A2F_MSS_I2C0 */

/*
 * MSS I2C_1
 */
#if defined(CONFIG_A2F_MSS_I2C1)

#define I2C_A2F_DEV1_IRQ	17
#define I2C_A2F_DEV1_REGS	0x40012000

static struct resource i2c_a2f_dev1_resources[] = {
	{
		.start	= I2C_A2F_DEV1_IRQ,
		.end	= I2C_A2F_DEV1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= I2C_A2F_DEV1_REGS,
		.end	= I2C_A2F_DEV1_REGS + 0xFFF,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device i2c_a2f_dev1 = {
	.name           = "i2c_a2f",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(i2c_a2f_dev1_resources),
	.resource       = i2c_a2f_dev1_resources,
	.dev		= {
		.release = i2c_a2f_dev_release,
	},
};

static struct i2c_a2f_data i2c_a2f_data_dev1 = {
	.i2c_clk	= 400000,
};

#endif	/* CONFIG_A2F_MSS_I2C1 */

/*
 * Driver init
 */
static int __init i2c_a2f_dev_init_module(void)
{
	int ret = 0;

#if defined(CONFIG_A2F_MSS_I2C0)
	/*
 	 * Pass the device parameters to the driver
 	 */
	i2c_a2f_data_dev0.ref_clk = a2f_clock_get(CLCK_PCLK0);
	platform_set_drvdata(&i2c_a2f_dev0, &i2c_a2f_data_dev0);

	/*
	 * Register a platform device for this interface
	 */
	platform_device_register(&i2c_a2f_dev0);		
#endif

#if defined(CONFIG_A2F_MSS_I2C1)
	/*
 	 * Pass the device parameters to the driver
 	 */
	i2c_a2f_data_dev1.ref_clk = a2f_clock_get(CLCK_PCLK1);
	platform_set_drvdata(&i2c_a2f_dev1, &i2c_a2f_data_dev1);

	/*
	 * Register a platform device for this interface
	 */
	platform_device_register(&i2c_a2f_dev1);		
#endif

	d_printk(1, "ret=%d\n", ret);
	return ret;
}

/*
 * Driver clean-up
 */
static void __exit i2c_a2f_dev_cleanup_module(void)
{
#if defined(CONFIG_A2F_MSS_I2C0)
	platform_device_unregister(&i2c_a2f_dev0);		
#endif

#if defined(CONFIG_A2F_MSS_I2C1)
	platform_device_unregister(&i2c_a2f_dev1);		
#endif

	d_printk(1, "%s\n", "ok");
}

module_init(i2c_a2f_dev_init_module);
module_exit(i2c_a2f_dev_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir Khusainov, vlad@emcraft.com");
MODULE_DESCRIPTION("Registration of I2C controllers");

