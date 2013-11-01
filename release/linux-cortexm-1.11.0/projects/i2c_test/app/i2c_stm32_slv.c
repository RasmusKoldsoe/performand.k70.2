/*
 * i2c_stm32_slv.c - Board-specific registration of I2C devices
 * Author: Vladimir Khusainov, vlad@emcraft.com
 * Copyright 2013 Emcraft Systems
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
#include <linux/i2c.h>
#include <linux/clk.h>
#include <asm/clkdev.h>
#include <mach/platform.h>
#include <mach/clock.h>

/*
 * Service to print debug messages
 */
#define d_printk(level, fmt, args...)					\
	if (i2c_stm32_slv_debug >= level) printk(KERN_INFO "%s: " fmt,	\
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
static int i2c_stm32_slv_debug = 0;

/*
 * User can change verbosity of the driver
 */
module_param(i2c_stm32_slv_debug, int, S_IRUSR);
MODULE_PARM_DESC(i2c_stm32_slv_debug, 
	"I2C device registration verbosity level");

/*
 * I2C busses handlers
 */
static struct i2c_adapter *i2c_bus0 = NULL;
static struct i2c_adapter *i2c_bus1 = NULL;
static struct i2c_adapter *i2c_bus2 = NULL;

/*
 * Handlers for registered I2C devices
 */
static struct i2c_client *i2c_devices[16] = { NULL, };

/*
 * I2C slave devices: STM32F4 SOM
 */
static struct i2c_board_info i2c_eeprom__stm32_som_slv = {
		I2C_BOARD_INFO("24c512", 0x57)
};

/*
 * Driver init
 */
static int __init i2c_stm32_slv_init_module(void)
{
	int p = stm32_platform_get();
	int i;
	int ret = 0;

	/*
	 * Get access to the I2C busses
	 */
	i2c_bus0 = i2c_get_adapter(0);
	if (! i2c_bus0) {
		d_printk(1, "unable to get adapter for i2c-0\n");
		ret = -EIO;
		goto Done_cleanup;
	}
	i2c_bus1 = i2c_get_adapter(1);
	if (! i2c_bus1) {
		d_printk(1, "unable to get adapter for i2c-1\n");
		ret = -EIO;
		goto Done_cleanup;
	}
	i2c_bus2 = i2c_get_adapter(2);
	if (! i2c_bus2) {
		d_printk(1, "unable to get adapter for i2c-2\n");
		ret = -EIO;
		goto Done_cleanup;
	}

	/*
 	 * Determine the board-specific info data structure
 	 */
	if (p == PLATFORM_STM32_STM_SOM) {
		i2c_devices[0] = i2c_new_device(i2c_bus0, 
					&i2c_eeprom__stm32_som_slv);
		if (! i2c_devices[0]) {
			d_printk(1, "unable to get client for:i2c-0:0x57\n");
			ret = -EIO;
			goto Done_cleanup;
		}
		i2c_devices[1] = NULL;
		goto Done;
	}


Done_cleanup:
	for (i = 0; i2c_devices[i] != NULL; i++) {
		i2c_unregister_device(i2c_devices[i]);
	}

	if (i2c_bus0) {
		i2c_put_adapter(i2c_bus0);
	}
	if (i2c_bus1) {
		i2c_put_adapter(i2c_bus1);
	}
	if (i2c_bus2) {
		i2c_put_adapter(i2c_bus2);
	}

Done:
	d_printk(1, "bus0=%p,bus1=%p,bus2=%p,ret=%d\n", 
		i2c_bus0, i2c_bus1, i2c_bus2, ret);
	return ret;
}

/*
 * Driver clean-up
 */
static void __exit i2c_stm32_slv_cleanup_module(void)
{
	int i;

	for (i = 0; i2c_devices[i] != NULL; i++) {
		i2c_unregister_device(i2c_devices[i]);
	}

	if (i2c_bus0) {
		i2c_put_adapter(i2c_bus0);
	}
	if (i2c_bus1) {
		i2c_put_adapter(i2c_bus1);
	}
	if (i2c_bus2) {
		i2c_put_adapter(i2c_bus2);
	}

	d_printk(1, "%s\n", "ok");
}

module_init(i2c_stm32_slv_init_module);
module_exit(i2c_stm32_slv_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir Khusainov, vlad@emcraft.com");
MODULE_DESCRIPTION("Board-specific registration of I2C devices");
