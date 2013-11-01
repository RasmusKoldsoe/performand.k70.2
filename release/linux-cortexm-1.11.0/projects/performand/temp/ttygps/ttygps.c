
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/mutex.h>

struct gpstty_cb {
	struct tty_struct	*tty;
	//struct hci_dev		*hdev;
	//unsigned long		flags;

	//struct hci_uart_proto	*proto;
	//void			*priv;

	//struct sk_buff		*tx_skb;
	//unsigned long		tx_state;
	//spinlock_t		rx_lock;
};


static DEFINE_MUTEX(ttygps_mutex);

/* ------------------------------------------------------- */

/*
 * Function irtty_stop_receiver (tty, stop)
 *
 */

static inline void ttygps_stop_receiver(struct tty_struct *tty, int stop)
{
	struct ktermios old_termios;
	int cflag;

	mutex_lock(&tty->termios_mutex);
	old_termios = *(tty->termios);
	cflag = tty->termios->c_cflag;
	
	if (stop)
		cflag &= ~CREAD;
	else
		cflag |= CREAD;

	tty->termios->c_cflag = cflag;
	if (tty->ops->set_termios)
		tty->ops->set_termios(tty, &old_termios);
	mutex_unlock(&tty->termios_mutex);
}

/*****************************************************************/


/* 
 *  Function ttygps_open(tty)
 *
 *    This function is called by the TTY module when the GPS line
 *    discipline is called for.  Because we are sure the tty line exists,
 *    we only have to link it to a free GPS channel.  
 */
static int ttygps_open(struct tty_struct *tty) 
{
	/* Every entry point will now have access to our private data structure */
	struct gpstty_cb *ttygps = (void *) tty->disc_data;

	if (ttygps)
		return -EEXIST;

	if (!(ttygps = kzalloc(sizeof(struct gpstty_cb), GFP_KERNEL))) {
		printk("Can't allocate control structure");
		return -ENFILE;
	}

	return 1;
}

/* 
 *  Function ttygps_close (tty)
 *
 *    Close down a GPS channel. This means flushing out any pending queues,
 *    and then restoring the TTY line discipline to what it was before it got
 *    hooked to GPS (which usually is TTY again).  
 */
static void ttygps_close(struct tty_struct *tty) 
{
	//struct sirtty_cb *priv = tty->disc_data;

	//TTYGPS_DEBUG(0, "%s - %s: irda line discipline closed\n", __func__, tty->name);
}




static struct tty_ldisc_ops ttygps_ldisc = {
	.magic		= TTY_LDISC_MAGIC,	/* Magic */
 	.name		= "ttyGPS",		/* Name of our Line Discipline */
	.flags		= 0,			/* Our Line Discipline ID number */
	.open		= ttygps_open,		/* Called on open */
	.close		= ttygps_close,		/* Called on close */
	.read		= NULL, //ttygps_read,		/* Called when data is requested from user space */
	.write		= NULL,			/* Called to write data */
	.ioctl		= NULL, //ttygps_ioctl,		/* I/O Control commands */
 	.poll		= NULL,			/* Poll */
	.receive_buf	= NULL, //ttygps_receive_buf,	/* Called from the serial driver to pass on data */
	.write_wakeup	= NULL, //ttygps_write_wakeup,	/* Called when the serial device driver is ready to transmit 								more data */
	.owner		= THIS_MODULE,
};

static int __init ttygps_init(void)
{
	int err;

	if ((err = tty_register_ldisc(N_GPS, &ttygps)) != 0)
		printk("ttygps: can't register line discipline (err = %d)\n",err);
	return err;
}

static void __exit ttygps_cleanup(void) 
{
	int err;

	if ((err = tty_unregister_ldisc(N_GPS))) {
		printk("%s(), can't unregister line discipline (err = %d)\n",
			   __func__, err);
	}
}

module_init(ttygps_init);
module_exit(ttygps_cleanup);

MODULE_AUTHOR("Robert Brehm");
MODULE_DESCRIPTION("GPS TTY device driver");
MODULE_ALIAS_LDISC(N_GPS);
MODULE_LICENSE("GPL");
