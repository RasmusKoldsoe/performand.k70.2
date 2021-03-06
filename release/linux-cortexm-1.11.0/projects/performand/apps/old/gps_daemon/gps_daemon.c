#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <time.h>
#include <termios.h>

#include "gps_utils.h"

#define FILE_LENGTH 	0x1000	//1KB File memory

char* file_memory;
int fileptr=0;
struct flock lock;
struct RMC_DATA gprmc;
static int fd;

static unsigned char disableAllMessages[] = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableTenHz[] = "$PMTK220,1000*XX\r\n"; 

static int writeMessageToPort(unsigned char* message, unsigned int messageLength, int tty)
{
        unsigned int checksum=0;
        unsigned int i;
        unsigned char currentChar = '0';
        unsigned long flags;
        unsigned int ier;
        int locked = 1;
        char csStr[3];

        for(i=1; i<messageLength-1; i++)
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


	//printf("Message: %s Length: %d",message, messageLength);

	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;

}

void print_byte_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%02X ", buff[i] & 0xff);
	}
	printf("\n");
}

void print_char_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%c", buff[i] & 0xff);
	}
	printf("\n");
}

/* 
 * Prepare a memory mapped file in /gps
 */
int preparemappedMem() {
	
	/* Prepare a file large enough.*/
	if((fd = open ("/gps", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
		fprintf(stderr, "Unable to open file: gps\n");
		return -1;	
	}

	/* Initialize the flock structure. */
	memset (&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	/* Place a write lock on the file. */
	fcntl (fd, F_SETLKW, &lock);


	/* go to the location corresponding to the last byte */
	if (lseek (fd, FILE_LENGTH-1, SEEK_SET) == -1) {
		fprintf(stderr, "lseek error");
		return -1;
	}

	/* write a dummy byte at the last location */
	if (write (fd, "", 1) != 1) {
		fprintf(stderr, "write error");
		return -1;
	}

	//Go to the start of the file again
	lseek (fd, 0, SEEK_SET);

	file_memory = (char*)mmap (0, FILE_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);

	if(file_memory == ((caddr_t) -1))
      	{
        	fprintf(stderr, "%s: mmap error for file 'gps' \n",strerror(errno));
		return -1;
        }

	memset(file_memory,0,FILE_LENGTH);


	/* Release the lock. */
	lock.l_type = F_UNLCK;
	fcntl (fd, F_SETLKW, &lock);

	close (fd);
	
	return 1;		
}

void write_mm_file(char *content) {
	sprintf((char*) file_memory, "%s",content);		
}


void append_mm_file(char *content) {
	//Check if the current content will overflows the file.
	if(fileptr+strlen(content)+1 > FILE_LENGTH)
		fileptr=0;

	sprintf((char*) file_memory+fileptr, "%s",content);
	fileptr+=strlen(content)+1;
	if(fileptr >= FILE_LENGTH)
		fileptr=0;		
}

void toXMLfile(RMC_DATA *gpsRMCData) {
	int strlen = 0;	
	int mem_ptr = 0;

	/* Place write lock on file. */
	lock.l_type = F_WRLCK;
	fcntl (fd, F_SETLKW, &lock);

	mem_ptr = *file_memory;
	mem_ptr = (mem_ptr<<8) + *(file_memory+1);

	strlen = sprintf((char*)(file_memory+2+mem_ptr),"<gmttime>\n\
<hours>%02d</hours>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<milliseconds>%03d</milliseconds>\n\
</gmttime>\n\
<date>\n\
<dd>%02d</dd>\n\
<mm>%02d</mm>\n\
<yyyy>20%02d</yyyy>\n\
</date>\n\
<latitude>\n\
<degrees>%02d</degrees>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<milliseconds>%03d</milliseconds>\n\
<hemisphere>%c</hemisphere>\n\
</latitude>\n\
<longitude>\n\
<degrees>%03d</degrees>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<hemisphere>%c</hemisphere>\n\
</longitude>\n\
<speedMS>%s</speedMS>\n\
<cog>%s</cog>\n",
	gpsRMCData->timeHour, gpsRMCData->timeMinute, gpsRMCData->timeSecond, gpsRMCData->timeMillisecond,
	gpsRMCData->dateDay, gpsRMCData->dateMonth,gpsRMCData->dateYear, 
	gpsRMCData->latitudeDegrees, gpsRMCData->latitudeMinutes, gpsRMCData->latitudeSeconds, gpsRMCData->latitudeHemisphere,
	gpsRMCData->longitudeDegrees, gpsRMCData->longitudeMinutes, gpsRMCData->longitudeSeconds, gpsRMCData->longitudeHemisphere, gpsRMCData->speedOverGround, gpsRMCData->courseOverGround);

	
	//printf("MemAddr: %02X MemPtr: %05d strlen: %05d\n",file_memory ,mem_ptr, strlen);

	if((strlen+mem_ptr) < FILE_LENGTH+2) {
		*file_memory = (char)(((strlen+mem_ptr) & 0xFF00)>>8);
		*(file_memory+1) = (char)((strlen+mem_ptr) & 0xFF);
	}

	/* Release the lock. */
	lock.l_type = F_UNLCK;
	fcntl (fd, F_SETLKW, &lock);

	//print_char_array(file_memory, strlen+mem_ptr, 2);	
}


int main(int argc, char **argv)
{
	struct termios tio;
	int tty_fd;
	fd_set rdset;
	unsigned char c=0;
	char buffer[255]; 
	char *bufptr; 
	int nbytes; 
	int pos=0;
	char rxData = 0;
	RMC_DATA gpsRMCData;

	//Prepare the mapped Memory file
	if(preparemappedMem() < 0) {
		printf("Error mapping GPS file.\n");
		return -1;	
	}
	printf("GPS Daemon: Mem mapped at %02X [%d Bytes]\n", file_memory, FILE_LENGTH);

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST; //0;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);
	tio.c_cc[VMIN]=0;
	tio.c_cc[VTIME]=0;

	tty_fd=open("/dev/ttyS1", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio,B9600);
	cfsetispeed(&tio,B9600);
	tcsetattr(tty_fd,TCSANOW,&tio);

	writeMessageToPort(enableTenHz, sizeof(enableTenHz), tty_fd);
	writeMessageToPort(disableAllMessages, sizeof(disableAllMessages), tty_fd);
	writeMessageToPort(enableRMC, sizeof(enableRMC), tty_fd);

	bufptr = buffer; 

	while(1) {
		// read characters into our string buffer until we get a CR or NL 
		/*nbytes = 0;
		while (!read(tty_fd,&rxData, 1)) {
			printf("%c",rxData);
			switch (rxData) {
				case 0:
					// Ignore, they mess up strstr
					break;
				case '\r':
					printf("Reveid 'r'\n");
				break;
				case '\n':
					printf("Reveid 'n'\n");
					break;
				default:
					break;
			}
		}
		printf("\n");
		
		*/
		
		while( (nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1) ) > 0)
		{
			bufptr += nbytes;
			if (bufptr[-2] == '\r' && bufptr[-1] == '\n') {	
				*bufptr = '\0';
				bufptr = buffer;
				//printf("%s\n",buffer);				
				parseGPRMC(buffer, &gprmc);
				toXMLfile(&gprmc);
			}
			usleep(100000);
		}
	}
  	return 1;
}
