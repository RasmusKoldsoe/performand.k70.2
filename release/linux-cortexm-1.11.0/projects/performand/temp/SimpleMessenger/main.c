/*
 * main.c
 *
 *  Created on: Sep 12, 2013
 *      Author: rasmus
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void print_byte_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%02X ", buff[i] & 0xff);
	}
	printf("\n");
}

int main (void)
{
	int fd;
	char msg[42]; memset(msg, 0, sizeof(msg));
	char buff[128]; memset(buff,0,sizeof(buff));
	int size = sizeof(msg);
	int n=0;

	msg[0] = 0x01;
	msg[2] = 0xFE;
	msg[3] = 0x26;
	msg[4] = 0x08;
	msg[5] = 0x05;
	msg[38] = 0x01;

	fd = open("/dev/ttyACM0", O_RDWR);
	if(fd < 0){
		printf("ERROR Opening port\n");
		return -1;
	}

	write(fd, msg, size);
	printf("Message sent (%d bytes): ", size);
	print_byte_array(msg, sizeof(msg), 0);

	while(1) {
		n = read(fd, buff, 3);
		printf("Message header received (%d bytes): ", n);
		print_byte_array(buff, n, 0);

		int next = (int)buff[2];
		if(next > 0) {
			n = read(fd, buff, next);
			printf("Message body received (%d bytes): ", n);
			print_byte_array(buff, n, 0);
		}
	}
	close(fd);
	return 0;
}
