/*
 * main.c
 *
 *  Created on: Sep 12, 2013
 *      Author: rasmus
 */
#include <linux/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

	int fd;

void print_byte_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%02X ", buff[i] & 0xff);
	}
	printf("\n");
}

int read_bytes(char* rxBuf, int numToRead, int block) {
    if (block) {
          // Blocking mode, so don't return until we read all the bytes requested
          int bytesRead;
          // Keep getting data if we have a number of bytes to fetch 
          while (numToRead) {
               bytesRead = read(fd,rxBuf,numToRead);//ReadRxBuffer(rxBuf, numToRead);
               if (bytesRead) {
                    rxBuf += bytesRead;
                    numToRead -= bytesRead;
               }
          }
	
          return bytesRead;
     } else {
          // Non-blocking mode, just read what is available in buffer 
          return read(fd,rxBuf,numToRead);
     }
     

}

int main (void)
{

	char msg[42]; memset(msg, 0, sizeof(msg));
	char buff[128]; memset(buff,0,sizeof(buff));
	int size = sizeof(msg);
	int n=0;

	struct termios tio;
	fd_set rdset;
	unsigned char c=0;

	msg[0] = 0x01;
	msg[2] = 0xFE;
	msg[3] = 0x26;
	msg[4] = 0x08;
	msg[5] = 0x05;
	msg[38] = 0x01;

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag  = 0; //RAw output &= ~OPOST; //0;
	tio.c_cflag &= CRTSCTS | CS8 | CLOCAL | CREAD; //| CRTSCTS | CS8 | CLOCAL | CREAD;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);
	tio.c_cc[VMIN]=0;
	tio.c_cc[VTIME]=1;

	fd = open("/dev/ttyS4", O_RDWR);
	if(fd < 0){
		printf("ERROR Opening port\n");
		return -1;
	}

	cfsetospeed(&tio,B115200);
	cfsetispeed(&tio,B115200);
	tcsetattr(fd,TCSANOW,&tio);


         if ((n=write(fd, msg,size)) < size) {
		printf("ERROR writing to port.\n");
		return -1;
	}

	printf("Message sent (%d bytes): ", size);
	print_byte_array(msg, sizeof(msg), 0);

	while(1) {
		n = read_bytes(buff, 3, 1);
		if(n > 0) {
			printf("Message header received (%d bytes): ", n);
			print_byte_array(buff, n, 0);
		}
		//usleep(10000);
		int next = (int)buff[2];
		if(next > 0) {
			n = read(fd, buff, next);
			printf("Message body received (%d bytes): ", n);
			print_byte_array(buff, n, 0);
		}
		next=0;
		
	}
	close(fd); 
	return 0;
}
