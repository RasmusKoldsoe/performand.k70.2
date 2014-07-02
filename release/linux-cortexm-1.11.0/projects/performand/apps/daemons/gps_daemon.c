#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <math.h>
#include <sys/mman.h>
#include <time.h>
#include <termios.h>
#include <pthread.h>
#include <semaphore.h>

#include "gps_utils.h"
#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/common_tools.h"
#include "../Common/utils.h"

#define SAMPLE_PERIOD_US 50000

void register_sig_handler();
void sigint_handler(int sig);

struct RMC_DATA gprmc;
int runtime_count, file_idx = 0;
static char *programName = NULL;
int done;


static unsigned char disableAllMessages[] = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableTenHz[] = "$PMTK220,100*XX\r\n"; 

static int writeMessageToPort(unsigned char* message, unsigned int messageLength, int tty)
{
	unsigned int checksum = 0;
	unsigned int i;

	char csStr[3];

	for(i=1; i<messageLength-1; i++) {
		if(message[i] == '*') {
			break;
		}
		checksum = checksum ^ message[i];
	}
	sprintf(csStr, "%02X", checksum);
	message[messageLength-5] = csStr[0];
	message[messageLength-4] = csStr[1];

	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		fprintf(stderr, "[%s] ERROR writing to port\n", programName);
		return -1;
	}

	return 0;
}

int convertIntoXml(char *buffer, RMC_DATA *gpsRMCData, int runtime_count)
{
/*
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
*/

	return sprintf(buffer, ",%02d,%02d,%02d,%03d,%02d,%02d,20%02d,%02d,%02d,%.3f,%c,%03d,%02d,%.3f,%c,%3.2f,%3.2f\n",
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
		gpsRMCData->courseOverGround );
}

void register_sig_handler()
{
	struct sigaction sia;

	memset(&sia, 0, sizeof(sia));
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(0);
	} 

	done = 0;
}

void sigint_handler(int sig)
{
	printf("%s Terminating.\n", programName);
	done = 1;
}

void *set_date(void *sem) {
	sem_t *data_sem = (sem_t *)sem;

	while(!done) {
		sem_wait(data_sem);

		if(gprmc.positionFix != NO_FIX_INVALID) {
			setDate(gprmc.dateDay, gprmc.dateMonth, gprmc.dateYear+2000, gprmc.timeHour, gprmc.timeMinute, gprmc.timeSecond);
			printf("[%s] Local time set from GPS\n", programName);
			break;
		}
	}
	return NULL	;
}


int main(int argc, char **argv)
{
	int START_TIMESYNC_THREAD = 0;
	{
		char **sp = argv;
		while( *sp != NULL ) {
			programName = strsep(sp, "/");
		}

		int i;
		for(i=0; i<argc; i++) {
			if (strcmp(argv[i], "WITH_TIMESYNC") == 0)
				START_TIMESYNC_THREAD = 1;
		}
	}

	struct termios tio;
	int tty_fd;
	char buffer[850]; 
	char *bufptr; 
	int nbytes; 
	int runtime_counter = 0;
	struct timespec ts_begin, ts_end, ts_interval;
	long process_time=0;
	int time_updated = 0;
	int sd_tp;
	pthread_t set_date_thread;
	pthread_attr_t thread_attr;
	sem_t data_sem;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	sem_init(&data_sem, 0, 0);

	if( START_TIMESYNC_THREAD == 1) {
		debug(1, "Starting time synchronization with GPS\n");
		sd_tp = pthread_create( &set_date_thread, &thread_attr, set_date, (void *)&data_sem);
		if( sd_tp ) {
	  		printf("[%s] ERROR read thread: Return code from pthread_create(): %d\n", programName, sd_tp);
	  		return -1;
		}
	}
	else {
		debug(1, "Starting without time synchronization. Start program with 'WITH_TIMESYNC' to enable\n");
	}

	//Prepare and initialise memory mapping
	h_mmapped_file gps_mapped_file;
	memset(&gps_mapped_file, 0, sizeof(h_mmapped_file));
	gps_mapped_file.filename = "/sensors/gps";
	gps_mapped_file.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&gps_mapped_file)) < 0) {
		printf("[%s] ERROR mapping %s file.\n", programName, gps_mapped_file.filename);
		return -1;	
	}

	//Prepare serial port for tty data retrieval.
	memset(&tio, 0, sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tio.c_cc[VMIN]=0;
	tio.c_cc[VTIME]=0;

	tty_fd=open("/dev/ttyS1", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio,B9600);
	cfsetispeed(&tio,B9600);
	tcsetattr(tty_fd, TCSANOW, &tio);

	// Setup GAINSPAN Wifi chip
	writeMessageToPort(enableTenHz, sizeof(enableTenHz), tty_fd);
	writeMessageToPort(disableAllMessages, sizeof(disableAllMessages), tty_fd);
	writeMessageToPort(enableRMC, sizeof(enableRMC), tty_fd);

	bufptr = buffer; 

	register_sig_handler();
	runtime_counter = read_rt_count();

	while(!done) {
		while( (nbytes=read(tty_fd, bufptr, buffer+sizeof(buffer)-bufptr-1) ) > 0)
		{
			bufptr += nbytes;
			if (bufptr[-2] == '\r' && bufptr[-1] == '\n') {	
				*bufptr = '\0';
				bufptr = buffer;

				// Measure the time it takes to send out the 
				// data to maintain a stable sample rate
				//  ./mnt/apps/daemons/gps_daemon & ./mnt/apps/daemons/testing
				//**********************************************
				clock_gettime(CLOCK_REALTIME, &ts_begin);

//printf("%s", buffer);
				parseGPRMC(buffer, &gprmc);
				sem_post(&data_sem);

				format_timespec(buffer, &ts_begin);
				if(gprmc.positionFix==NO_FIX_INVALID) {
					sprintf(buffer+strlen(buffer), ",,,,,,,,,,,,,,,,,\n");
				} 
				else {
					convertIntoXml(buffer+strlen(buffer)-1, &gprmc, runtime_count);
				}

				debug(2, "%s", buffer);
				mm_append(buffer, &gps_mapped_file);
				file_idx = write_log_file("gps", runtime_counter, file_idx, buffer);

				// How long time did it take?
				//**********************************************
				clock_gettime(CLOCK_REALTIME, &ts_end);

				ts_interval = subtract_timespec(ts_end, ts_begin);
				process_time = ts_interval.tv_sec * 1000000 + ts_interval.tv_nsec / 1000;

				if(process_time >= SAMPLE_PERIOD_US)
					process_time = SAMPLE_PERIOD_US;
				debug(2, "[T_Process: %d T_Sample: %d T_Percent: %0.1f]\n", process_time, SAMPLE_PERIOD_US - process_time, ((float)process_time/(float)SAMPLE_PERIOD_US)*100.0);
				//**********************************************

				runtime_count += 1;
				usleep(SAMPLE_PERIOD_US-process_time);
			}
		}
	}
  	return 1;
}
