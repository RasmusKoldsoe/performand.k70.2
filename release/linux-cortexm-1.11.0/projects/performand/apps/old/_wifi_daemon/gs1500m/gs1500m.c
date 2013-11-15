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
#include "config.h"

#define FILE_LENGTH 	0x1000

int tty_fd;
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

	//printf("\nMessage: %s \r\n Length: %d",message, messageLength);

	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;

}


int check_response(char *message, char *expected) {
	//printf("Message %s", message);
	//printf("Expected %s", expected);

	if(strstr(message, expected))
		return 1;
	else if(strstr(message,"ERROR: INVALID INPUT\r\n\n\r\n"))
		return -1;
	else
		return -2;
}

/*int read_port(int tty_fd) {
	char buffer[2048];
	char numrec=0;

	int nbytes; 
	char *bufptr;
	bufptr = buffer;
	buffer[0] = '\0';
	
	usleep(400000);

	do {
    		nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1);
		bufptr += nbytes;
	} while( nbytes > 0 && (check_response(buffer)) || (bufptr-buffer > sizeof(buffer)));


//	while( (nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1) > 0))
//	{
//		bufptr += nbytes;
 //		numrec += nbytes;
//		//if (bufptr[-2] == 0xD && bufptr[-1] == 0xA)
		//	break;		
//	}
	*bufptr = '\0';

#ifdef DEBUG	
	printf("bufptr: %d\n\r", bufptr);	
	printf("buffer: %d\n\r", buffer);
	printf("Received Chars: %d\n\r", strlen(buffer));
	
	print_char_array(buffer, 35, 0);
	print_byte_array(buffer, 35, 0);
	printf("%s\n",buffer);

	printf("Found OK at %d\n\r", check_response(buffer));
#endif
	printf("%s\n",buffer);
} */

int send_and_check(char *cmd, int sleeptime, char *expected, int tty_fd)
{
    char buffer[1024];
    int numbytes=0;
    memset(buffer, 0, sizeof(char)*sizeof(buffer));
    usleep(500000);
    writeMessageToPort(cmd, strlen(cmd), tty_fd);
    usleep(sleeptime*1000);

    numbytes=read(tty_fd, buffer,strlen(expected));
   
    if(numbytes < strlen(expected) && !check_response(buffer, expected))
	return -1;

    print_char_array(buffer, strlen(buffer), 0);

    return 1;
}

void print_char_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%s", buff[i] & 0xff);
	}
	printf("\n");
}

void print_byte_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%X", buff[i] & 0xff);
	}
	printf("\n");
}

void gs1500m_sendTCPIPstring(char *d, int len) {
	char data[] = " Z10005Hello";	
	char *cmnd_ptr = data;

	*cmnd_ptr =(char)0x1B;
	//printf("Send string [len: %d]: %s\n",strlen(data),data);

	print_byte_array(data, strlen(data), 0);

	if (write(tty_fd, data, strlen(data)) < strlen(data))	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;
}

int gs1500m_startAP(){

	struct termios tio;

	fd_set rdset;
	unsigned char c=0;
	char buffer[255]; 
	void* file_memory;

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST; //0;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);
		
	tty_fd=open("/dev/ttyS3", O_RDWR);
	cfsetospeed(&tio,B115200);
	cfsetispeed(&tio,B115200);
	tcsetattr(tty_fd,TCSANOW,&tio);

	if(!send_and_check("AT\r\n", 1000, "AT\r\n\r\nOK\r\n\n\r\n", tty_fd)) {
		printf("Error!\r\n");
		return 0;
	}

	if(!send_and_check("AT+WD\r\n", 1000, "AT+WD\r\n\r\nOK\r\n\n\r\n", tty_fd)) {
		printf("Error!\r\n");
		return 0;
	}

	if(!send_and_check(	"AT+NSET=10.42.43.1,255.255.255.0,10.42.43.1\r\n"
				,1000, 
				"AT+NSET=10.42.43.1,255.255.255.0,10.42.43.1\r\n\r\nOK\r\n\n\r\n"
				,tty_fd)
			  ) {
		printf("Error!\r\n");
		return 0;
	}

	if(!send_and_check("AT+WM=2\r\n", 1000, "AT+WM=2\r\n\r\nOK\r\n\n\r\n",tty_fd)) {
			printf("Error!\r\n");
			return 0;
	}


	if(!send_and_check("AT+WA=" WIFI_NAME ",,11\r\n", 5000,
                   "AT+WA=" WIFI_NAME ",,11\r\n    IP              SubNet         Gateway\r\n 110.42.43.1: 255.255.255.0: 19"
                   "110.42.43.1\r\n\r\nOK\r\n\n\r\n",tty_fd)) {
			printf("Error!\r\n");
			return 0;
	}
	
	if(!send_and_check("AT+DHCPSRVR=1\r\n", 1000, "AT+DHCPSRVR=1\r\n\r\nOK\r\n\n\r\n", tty_fd)) {
		printf("Error!\r\n");
		return 0;
	}



	if(!send_and_check("AT+BDATA=1\r\n", 1000, "AT+BDATA=1\r\n\r\nOK\r\n\n\r\n", tty_fd)) {
		printf("Error!\r\n");
		return 0;
	}

	if(!send_and_check("AT+NSTCP=2000\r\n", 1000, "AT+NSTCP=2000\r\n\r\nOK\r\n\n\r\n", tty_fd)) {
		printf("Error!\r\n");
		return 0;
	}

	while(1) {
		gs1500m_sendTCPIPstring(NULL, 0);
		usleep(500000);
	}

  	return(1);
}
