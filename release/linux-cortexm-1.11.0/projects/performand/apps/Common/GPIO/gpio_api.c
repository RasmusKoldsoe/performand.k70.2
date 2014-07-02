#include <stdio.h>
#include "gpio_api.h"

int gpio_export(int gpio)
{
#if __arm__
	int fd;
	char buf[16];
	sprintf(buf, "%d", gpio); 

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0)
		return -1;

	write(fd, buf, strlen(buf));
	close(fd);
#elif __i386__
	printf("GPIO PORT %d EXPORT\n", gpio);
#else
#error "Target arcitechture not recognized"
#endif

	return 1;
}

int gpio_unexport(int gpio)
{
#if __arm__
	int fd;
	char buf[16];
	sprintf(buf, "%d", gpio); 

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if(fd<0)
		return -1;

	write(fd, buf, strlen(buf));
	close(fd);
#elif __i386__
	printf("GPIO PORT %d UNEXPORT\n", gpio);
#else
#error "Target arcitechture not recognized"
#endif

	return 1;
}

int gpio_setDirection(int gpio, int dir) {
#if __arm__
	int fd;
	char buf[36]; 	

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	fd = open(buf, O_WRONLY);
	if(fd<0)
		return -1;

	if(dir)
		write(fd, "in", 2);
	else 
		write(fd, "out", 3); 

	close(fd);
#elif __i386__
	printf("GPIO PORT %d SET DIRECTION %s\n", gpio, dir==GPIO_DIR_IN?"IN":"OUT");
#else
#error "Target arcitechture not recognized"
#endif

	return 1;
}

int gpio_setValue(int gpio, int value) {
#if __arm__
	int fd;
	char buf[36]; 

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if(fd<0)
		return -1;

	if(value)
		// Set GPIO high status
		write(fd, "1", 1); 
	else
		// Set GPIO low status 
		write(fd, "0", 1); 

	close(fd);
#elif __i386__
	printf("GPIO PORT %d SET VALUE %s\n", gpio, value==GPIO_SET_HIGH?"HIGH":"LOW");
#else
#error "Target arcitechture not recognized"
#endif

	return 1;
}

int gpio_getValue(int gpio) {
	int val;

#if __arm__
	char value;
	int fd;
	char buf[36]; 

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if(fd<0)
		return -1;

	read(fd, &value, 1);

	if(value == '0')
	{ 
	     val = 0;
	}
	else
	{
	    val = 1;
	}

	close(fd);
#elif __i386__
	printf("GPIO PORT %d GET VALUE - Read 0\n", gpio);
	val = 0;
#else
#error "Target arcitechture not recognized"
#endif

	return val;
}

