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
#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/utils.h"

#define SAMPLE_PERIOD_US 50000

void register_sig_handler();
void sigint_handler(int sig);

struct RMC_DATA gprmc;
int fd;
int runtime_count,file_idx = 0;

int done;


static unsigned char disableAllMessages[] = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableTenHz[] = "$PMTK220,100*XX\r\n"; 

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


	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;

}

int convertIntoXml(char *buffer, RMC_DATA *gpsRMCData, int runtime_count) {
	int strlen=0;

	strlen = sprintf(buffer,"\n<gps id=\"%d\">\n\
<gmttime>\n\
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
<seconds>%.3f</seconds>\n\
<hemisphere>%c</hemisphere>\n\
</latitude>\n\
<longitude>\n\
<degrees>%03d</degrees>\n\
<minutes>%02d</minutes>\n\
<seconds>%.3f</seconds>\n\
<hemisphere>%c</hemisphere>\n\
</longitude>\n\
<speedMS>%3.2f</speedMS>\n\
<cog>%3.2f</cog>\n\
</gps>\n\n",runtime_count,
	gpsRMCData->timeHour, 
	gpsRMCData->timeMinute, 
	gpsRMCData->timeSecond,
	gpsRMCData->timeMillisecond,
	gpsRMCData->dateDay, 
	gpsRMCData->dateMonth,
	gpsRMCData->dateYear, 
	gpsRMCData->latitudeDegrees, 
	gpsRMCData->latitudeMinutes, 
	gpsRMCData->latitudeSeconds, 
	gpsRMCData->latitudeHemisphere,
	gpsRMCData->longitudeDegrees, 
	gpsRMCData->longitudeMinutes, 
	gpsRMCData->longitudeSeconds, 
	gpsRMCData->longitudeHemisphere, 
	gpsRMCData->speedOverGround, 
	gpsRMCData->courseOverGround);
	
	return strlen;
}

void register_sig_handler()
{
	struct sigaction sia;

	bzero(&sia, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(1);
	} 

	done = 0;
}

void sigint_handler(int sig)
{
	printf("Terminating gps daemon.\n");
	done = 1;
}

int main(int argc, char **argv)
{
	struct termios tio;
	int tty_fd;
	fd_set rdset;
	unsigned char c=0;
	char buffer[850]; 
	char *bufptr; 
	int nbytes; 
	int pos=0;
	int len=0;
	char rxData = 0;
	RMC_DATA gpsRMCData;
	int runtime_counter = 0;
	struct timespec spec;
	long ms_before, ms_after, process_time=0;
	long s_before, s_after;
	int time_updated = 0;

	//Prepare and initialise memory mapping
	h_mmapped_file gps_mapped_file;
	memset(&gps_mapped_file, 0, sizeof(h_mmapped_file));

	gps_mapped_file.filename = "gps";
	gps_mapped_file.size = DEFAULT_FILE_LENGTH;

	runtime_counter = rw_rnt_count();
	usleep(10000);

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&gps_mapped_file)) < 0) {
		printf("Error mapping %s file.\n",gps_mapped_file.filename);
		return -1;	
	}
	
	register_sig_handler();

	//Prepare serial port for tty data retrieval.
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

	while(!done) {
		while( (nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1) ) > 0)
		{
			bufptr += nbytes;
			if (bufptr[-2] == '\r' && bufptr[-1] == '\n') {	
				*bufptr = '\0';
				bufptr = buffer;

				// Measure the time it takes to send out the 
				// data to maintain a stable sample rate
				//  ./mnt/apps/daemons/gps_daemon & ./mnt/apps/daemons/testing
				//**********************************************
				clock_gettime(CLOCK_REALTIME, &spec);
				ms_before = round(spec.tv_nsec / 1.0e6);
				s_before = spec.tv_sec; 
								
				file_idx = write_log_file("gps", runtime_counter, 
								file_idx, buffer);				

				//printf("%s\n", buffer);
				parseGPRMC(buffer, &gprmc);
				//printf("%d\n", gprmc.latitudeSeconds);

				
				if(gprmc.positionFix==NO_FIX_INVALID) {
             			sprintf(buffer, "<gps id=\"%d\">\n<status>\nNO GPS SIGNAL\n</status>\n</gps>\n",runtime_count);	
					//setDate(gprmc.dateDay, gprmc.dateMonth, gprmc.dateYear+2000, 
					//		gprmc.timeHour, gprmc.timeMinute, gprmc.timeSecond);			      
					mm_append(buffer,&gps_mapped_file);
					//time_updated = 0;
                                } 
				else {
					//if(!time_updated) {
					//	setDate(gprmc.dateDay, gprmc.dateMonth, gprmc.dateYear+2000, 
					//		gprmc.timeHour, gprmc.timeMinute, gprmc.timeSecond);
					//	time_updated = 1;
					//}
					convertIntoXml(buffer, &gprmc,runtime_count);
				      	mm_append(buffer,&gps_mapped_file);
				} 
				// How long did it take ?
				//**********************************************
				clock_gettime(CLOCK_REALTIME, &spec);
				ms_after = round(spec.tv_nsec / 1.0e6);	
				process_time = ((ms_after+(spec.tv_sec - s_before)*1000)- ms_before)*1000;
				if(process_time>=SAMPLE_PERIOD_US)
					process_time = SAMPLE_PERIOD_US;
				//printf("[T_Process: %d T_Sample: %d]\n", process_time, SAMPLE_PERIOD_US - process_time);
				//**********************************************

				runtime_count+=1;
				usleep(SAMPLE_PERIOD_US-process_time);
			}
			
		}
		
	}
  	return 1;
}
