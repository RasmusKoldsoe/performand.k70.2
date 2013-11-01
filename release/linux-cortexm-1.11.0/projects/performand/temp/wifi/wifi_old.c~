#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#include "gs1500m_defs.h"


#define FILE_LENGTH 	0x1000


static unsigned char AT[] = "AT\r\n";



static int writeMessageToPort(unsigned char* message, unsigned int messageLength, int tty)
{
        unsigned int checksum=0;
        unsigned int i;
        unsigned char currentChar = '0';
        unsigned long flags;
        unsigned int ier;
        int locked = 1;
        char csStr[3];

/*        for(i=1; i<messageLength-1; i++)
        {
                if(message[i] == '*')
                {
                        break;
                }
                else
                {
                        checksum = checksum ^ message[i];
                }
        }
        sprintf(csStr, "%X", checksum);
        message[messageLength-5] = csStr[0];
        message[messageLength-4] = csStr[1];
*/

//	printf("\nMessage: %s",message, messageLength);

	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;

}

int read_port(int tty_fd) {
	char buffer[1024];
	int nbytes; 
	char *bufptr;
	nbytes = 0;
	bufptr = buffer;
	while((nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1))>0)
	{
		bufptr += nbytes;
		if (bufptr[-3] == 'O' && bufptr[-2] == 'K') {
 			printf("break");				
			break;			
		}
	}
	*bufptr = '\0';
	printf("%s", buffer);
}


int main(){

	struct termios tio;

	fd_set rdset;
	unsigned char c=0;
	char buffer[255]; 
 int tty_fd;
	
	int fd;
	void* file_memory;
	int pos=0;

	
	RMC_DATA gpsRMCData;

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST; //0;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=0;

	tty_fd=open("/dev/ttyS3", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio,B9600);
	cfsetispeed(&tio,B9600);
	tcsetattr(tty_fd,TCSANOW,&tio);

	writeMessageToPort("AT\r\n", sizeof("AT\r\n"), tty_fd);
	usleep(100000);
	read_port(tty_fd);

	//writeMessageToPort("AT+NSTAT=?\r\n", sizeof("AT+NSTAT=?\r\n"), tty_fd);
	//usleep(100000);
	//read_port(tty_fd);
	


/*	writeMessageToPort("AT+NSET=192.168.0.1,255.255.255.0,192.168.0.1\r\n", 
			sizeof("AT+NSET=192.168.1.1,255.255.255.0,192.168.1.1\r\n"), tty_fd);
	usleep(40000);
	read_port(tty_fd); */

/*	writeMessageToPort("AT+WM=1\r\n", sizeof("AT+WM=2\r\n"), tty_fd);
	usleep(40000);
	read_port(tty_fd);

	writeMessageToPort("AT+WA=PerforManD,,11\r\n", sizeof("AT+WA=PerforManD,,11\r\n"), tty_fd);
	usleep(10000000);
	read_port(tty_fd);

	//writeMessageToPort("AT+DHCPSRVR=1\r\n", sizeof("AT+DHCPSRVR=1"), tty_fd);
	//usleep(5000000);
	//read_port(tty_fd);

	writeMessageToPort("AT+NSTAT=?", sizeof("AT+NSTAT=?"), tty_fd);
	usleep(1000000);
	read_port(tty_fd);
*/
  return(1);
}
