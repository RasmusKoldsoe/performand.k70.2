#include <stdio.h>
#include "gpio_api.h"

int gpio_export(int gpio)
{
	int fd;
	char buf[16];
	sprintf(buf, "%d", gpio); 

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0)
		return -1;

	write(fd, buf, strlen(buf));
	close(fd);

	return 1;
}

int gpio_unexport(int gpio)
{
	int fd;
	char buf[16];
	sprintf(buf, "%d", gpio); 

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if(fd<0)
		return -1;

	write(fd, buf, strlen(buf));
	close(fd);

	return 1;
}

int gpio_setDirection(int gpio, int dir) {
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
	return 1;
}

int gpio_setValue(int gpio, int value) {
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
	return 1;
}

int gpio_getValue(int gpio) {
	char value;
	int val, fd;
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
	return val;
}

