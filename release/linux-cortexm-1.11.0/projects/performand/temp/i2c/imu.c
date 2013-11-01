#include <stdio.h> 
#include <fcntl.h> 
#include <pthread.h> 
//#include <linux/i2c.h>
#include "i2c-dev.h"

#define ADDR 0x68   //<----Adresse des Slaves

void *f1(void *x){
	int i;
	i = (int*)x;
	sleep(1);
	printf("f1: %d",i);
	pthread_exit(0); 
}
void *f2(void *x){
	int i;
	i = (int*)x;
	sleep(1);
	printf("f2: %d",i);
	pthread_exit(0); 
}

int main(){
	int fd; 

	char buffer[128]; 
	int n, err; 
	char *filename="/dev/i2c-0";
	int file;

	pthread_t f2_thread, f1_thread; 
	void *f2(), *f1();
	int i1,i2;
	i1 = 1;
	i2 = 2;

	pthread_create(&f1_thread,NULL,f1,&i1);
	pthread_create(&f2_thread,NULL,f2,&i2);

	pthread_join(f1_thread,NULL);
	pthread_join(f2_thread,NULL);

	int slave_address = ADDR; 

	if ((fd = open(filename, O_RDWR)) < 0) {
		printf("i2c open error: %d\n",fd); 
		return -1; 
	} 
	printf("i2c device = %d\n", fd);

	//<----Vorbereiten des I2C Slaves
	if (ioctl(fd, I2C_SLAVE, slave_address) < 0) {
		printf("ioctl I2C_SLAVE error"); 
		return -1; 
	} 

	i2c_smbus_read_byte_data(fd,0x75);
}

