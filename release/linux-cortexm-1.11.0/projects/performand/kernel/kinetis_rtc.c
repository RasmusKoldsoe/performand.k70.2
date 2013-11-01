/*
 * An RTC test device/driver
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>


#define RTC_IRQ         66


// Kinetis System Clock Gating Control Register 6
#define SIM_SCGC6	(* ((u32 *) 0x4004803C))		

// Kinetis RTC Control Register
#define RTC_CR		(* ((u32 *) 0x4003D010))
#define RTC_TCR		(* ((u32 *) 0x4003D00C))
#define RTC_TSR		(* ((u32 *) 0x4003D000))
#define RTC_TAR		(* ((u32 *) 0x4003D008)) 
#define RTC_SR		(* ((u32 *) 0x4003D014)) 
#define RTC_IER		(* ((u32 *) 0x4003D01C)) 

#define SIM_SCGC6_RTC_MASK			 0x20000000
#define RTC_CR_SWR_MASK                          0x1
#define RTC_CR_OSCE_MASK                         0x100
#define RTC_SR_TCE_MASK                          0x10

/* TCR Bit Fields */
#define RTC_TCR_TCR_MASK                         0xFF
#define RTC_TCR_TCR_SHIFT                        0
#define RTC_TCR_TCR(x)                           (((u32)(((u32)(x))<<RTC_TCR_TCR_SHIFT))&RTC_TCR_TCR_MASK)
#define RTC_TCR_CIR_MASK                         0xFF00
#define RTC_TCR_CIR_SHIFT                        8
#define RTC_TCR_CIR(x)                           (((u32)(((u32)(x))<<RTC_TCR_CIR_SHIFT))&RTC_TCR_CIR_MASK)
#define RTC_TCR_TCV_MASK                         0xFF0000
#define RTC_TCR_TCV_SHIFT                        16
#define RTC_TCR_TCV(x)                           (((u32)(((u32)(x))<<RTC_TCR_TCV_SHIFT))&RTC_TCR_TCV_MASK)
#define RTC_TCR_CIC_MASK                         0xFF000000
#define RTC_TCR_CIC_SHIFT                        24
#define RTC_TCR_CIC(x)                           (((u32)(((u32)(x))<<RTC_TCR_CIC_SHIFT))&RTC_TCR_CIC_MASK)


/*
 * Driver verbosity level: 0->silent; >0->verbose
 */
static int kinetis_rtc_debug = 0;

/*
 * User can change verbosity of the driver
 */
module_param(kinetis_rtc_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(kinetis_rtc_debug, "Kinetis RTC clock");

/*
 * Service to print debug messages
 */
#define d_printk(level, fmt, args...)				\
	if (kinetis_rtc_debug >= level) printk(KERN_INFO "%s: " fmt,	\
					__func__, ## args)

static struct platform_device *rtc = NULL;


static int kinetis_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm) {
	u32 sec_cnt_reg = RTC_TAR;
	alrm->enabled=1;
	alrm->pending=1;

	rtc_time_to_tm(sec_cnt_reg, &alrm->time);
	d_printk(2, "RTC_TAR: 0x%08x\n", RTC_TAR);
	return 0;
}

static int kinetis_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm) {
	int err = 0;	
	long unsigned int secs = 0;

	//Enable the alarm interrupt
	RTC_IER = 0x04;
	
	//For now just setting the Alarm register to now + 5 seconds.
	//TODO: Need to do this properly !!
	u32 sec_cnt_reg = RTC_TSR;
	printk("Setting wake time to %d",sec_cnt_reg+3);
	RTC_TAR = sec_cnt_reg+3;

	return 0;
}

static int kinetis_rtc_read_time(struct device *dev, struct rtc_time *tm) {
	u32 sec_cnt_reg = RTC_TSR;
	d_printk(2, "RTC_TSR: 0x%08x\n", sec_cnt_reg);

	rtc_time_to_tm(sec_cnt_reg, tm);
	return 0;
}

static int kinetis_rtc_set_time(struct device *dev, struct rtc_time *tm) {
	int err = 0;
	long unsigned int secs = 0;
	
	/* Check if the tm struct is valid */
	if(err = rtc_valid_tm(tm)) { 	
		printk( KERN_WARNING "%s: tm struct invalid" , __func__ );	
		return err;
	}
	
	/* Converting Greorian date to seconds since epoch */
	if(err = rtc_tm_to_time(tm, &secs)) {
		printk( KERN_WARNING "%s: rtc_tm_to_time(tm, &secs) failed" , __func__ );	
		return err;		
	}	

	/*clear the software reset bit*/
  	RTC_CR  = RTC_CR_SWR_MASK;	 
 	RTC_CR  &= ~RTC_CR_SWR_MASK; 

	RTC_SR =0;
	d_printk(2, "Setting TSR to: %d Seconds\n", secs);
	RTC_TSR = secs;
	
  	/*Enable the oscillator*/
  	RTC_CR |= RTC_CR_OSCE_MASK;

  	/*Wait to all the 32 kHz to stabilize, refer to the crystal startup time in the crystal datasheet*/
  	mdelay(10); //FIXME: Need datasheet here to check what this should be

  	/*Set time compensation parameters*/
  	RTC_TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);

  	/*Enable the counter*/
  	RTC_SR |= RTC_SR_TCE_MASK;
	return 0;
}

static int kinetis_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct platform_device *plat_dev = to_platform_device(dev);

	seq_printf(seq, "rtc\t\t: yes\n");
	seq_printf(seq, "id\t\t: %d\n", plat_dev->id);

	return 0;
}

static int kinetis_rtc_ioctl(struct device *dev, unsigned int cmd,
	unsigned long arg)
{
	/* We do support interrupts, they're generated
	 * using the sysfs interface.
	 */
	switch (cmd) {
	case RTC_PIE_ON:
	case RTC_PIE_OFF:
	case RTC_UIE_ON:
	case RTC_UIE_OFF:
	case RTC_AIE_ON:
	case RTC_AIE_OFF:
		return 0;

	default:
		return -ENOIOCTLCMD;
	}
}

static const struct rtc_class_ops test_rtc_ops = {
	.proc = kinetis_rtc_proc,
	.read_time = kinetis_rtc_read_time,
	.read_alarm = kinetis_rtc_read_alarm,
	.set_alarm = kinetis_rtc_set_alarm,
	.set_time = kinetis_rtc_set_time,
	.ioctl = kinetis_rtc_ioctl,
};

static ssize_t test_irq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	printk("RTC IQR\n");
	return sprintf(buf, "%d\n", 42);
}
static ssize_t test_irq_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int retval;
	struct platform_device *plat_dev = to_platform_device(dev);
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	retval = count;
	printk("RTC IQR\n");
/*	if (strncmp(buf, "tick", 4) == 0)
		rtc_update_irq(rtc, 1, RTC_PF | RTC_IRQF);
	else if (strncmp(buf, "alarm", 5) == 0)
		rtc_update_irq(rtc, 1, RTC_AF | RTC_IRQF);
	else if (strncmp(buf, "update", 6) == 0)
		rtc_update_irq(rtc, 1, RTC_UF | RTC_IRQF);
	else
		retval = -EINVAL;
*/
	return retval;
}
static DEVICE_ATTR(irq, S_IRUGO | S_IWUSR, test_irq_show, test_irq_store);

static int init_kinetis_rtc_module( void ) {
	int i = 0;

	/*Enable the clock to SRTC module register space*/
 	 SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

  	/* Only VBAT_POR has an effect on the SRTC, RESET to the part does not, 
	 * so one must manually reset the SRTC to make sure everything 
	 * is in a known state
	*/
 	
	/*clear the software reset bit*/
  	RTC_CR  = RTC_CR_SWR_MASK;
  	RTC_CR  &= ~RTC_CR_SWR_MASK; 

  	/*Enable the oscillator*/
  	RTC_CR |= RTC_CR_OSCE_MASK;

  	/*Wait to all the 32 kHz to stabilize, refer to the crystal startup time in the crystal datasheet*/
  	mdelay(10); //FIXME: Need datasheet here to check what this should be

  	/*Set time compensation parameters*/
  	RTC_TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);
  
  	/*Configure the timer seconds and alarm registers*/
 	RTC_TSR = 0;
  	RTC_TAR = 0;
	RTC_IER = 0;
  
  	/*Enable the counter*/
  	RTC_SR |= RTC_SR_TCE_MASK;
}


static int test_probe(struct platform_device *plat_dev)
{
	int err;

	init_kinetis_rtc_module();

	device_init_wakeup(&plat_dev->dev, 1);

	struct rtc_device *rtc= rtc_device_register("rtc", &plat_dev->dev, &test_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		err = PTR_ERR(rtc);
		return err;
	}

	err = device_create_file(&plat_dev->dev, &dev_attr_irq);
	if (err)
		goto err;

	platform_set_drvdata(plat_dev, rtc);

	return 0;

err:
	rtc_device_unregister(rtc);
	return err;
}

static int __devexit test_remove(struct platform_device *plat_dev)
{
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	rtc_device_unregister(rtc);
	device_remove_file(&plat_dev->dev, &dev_attr_irq);

	return 0;
}

static struct platform_driver kinetis_rtc_driver = {
	.probe	= test_probe,
	.remove = __devexit_p(test_remove),
	.driver = {
		.name = "kinetis-rtc",
		.owner = THIS_MODULE,
	},
};

static irqreturn_t kinetis_rtc_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
        printk(KERN_ALERT "RTC Interrupt!\n");

	/*Reset the timer seconds and alarm registers and disable the alarm interrupt.*/
 	RTC_TSR = 0;
  	RTC_TAR = 0;
	RTC_IER = 0;
	
        return IRQ_HANDLED;
}

static struct irqaction	kinetis_rtc_irqaction = {
	.name		= "Kinetis Kernel Time Tick",
	.flags		= IRQF_DISABLED | IRQF_IRQPOLL,
	.handler	= kinetis_rtc_irq_handler,
};

static int __init kinetis_rtc_init(void)
{
	int err;

	if ((err = platform_driver_register(&kinetis_rtc_driver)))
		return err;

	if ((rtc = platform_device_alloc("kinetis-rtc", 0)) == NULL) {
		err = -ENOMEM;
		goto exit_driver_unregister;
	}

	if ((err = platform_device_add(rtc)))
		goto exit_free;

	/* Setup, and enable IRQ */
	setup_irq(RTC_IRQ, &kinetis_rtc_irqaction);

	return 0;

exit_device_unregister:
	platform_device_unregister(rtc);

exit_free_test1:


exit_free:
	platform_device_put(rtc);

exit_driver_unregister:
	platform_driver_unregister(&kinetis_rtc_driver);
	return err;
}

static void __exit kinetis_rtc_exit(void)
{
	platform_device_unregister(rtc);
	platform_driver_unregister(&kinetis_rtc_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Brehm, brehm@mci.sdu.dk");
MODULE_DESCRIPTION("Kinetis RTC driver");

module_init(kinetis_rtc_init);
module_exit(kinetis_rtc_exit);
