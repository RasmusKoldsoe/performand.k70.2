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
#include "Queue/Queue.h"
#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/common_tools.h"
#include "../Common/utils.h"
#include "../Common/verbosity.h"
#include "../Common/logging.h"

#define SERIAL_READ_BUFFER_SIZE 255
#define PARSER_READ_BUFFER_SIZE 255

void register_sig_handler();
void sigint_handler(int sig);

sem_t data_available_sem;

static char *programName = NULL;
static char *portName = NULL;
queue_t readQueue;
int done;

static unsigned char testPackage[] = "$PMTK000*32\r\n";
static unsigned char disableAllMessages[] = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableTenHz[] = "$PMTK220,100*XX\r\n"; 

static void printHelp(void) {
	printf("USAGE: %s PORT (GPS_TIMESYNC / RTC_TIMESYNC / NO_TIMESYNC)\n", programName);
	printf("GPS_TIMESYNC includes RTC_TIMESYNC\n");
	printf("Example: %s /dev/ttyUSB0 WITH_TIMESYNC\n", programName);
}

static unsigned int calcCRC(char* message, unsigned int messageLength) {
	unsigned int checksum = 0;
	int i;

	for(i=0; i<messageLength; i++) {
		if( message[i] == '*' ) {
			break;
		}
		checksum ^= message[i];
	}

	return checksum;
}

static int writeMessageToPort(unsigned char* message, unsigned int messageLength, int tty)
{
	unsigned int i;
	char csStr[3];

	sprintf(csStr, "%02X", calcCRC(message+1, messageLength));
	message[messageLength-5] = csStr[0];
	message[messageLength-4] = csStr[1];

	printf("%s", message);
	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		fprintf(stderr, "[%s] ERROR writing to port\n", programName);
		return -1;
	}

	return i;
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
	printf("Requesting %s to terminate\n", programName);
	done = 1;

	// Force a data-available to make sure the main thread will exit.	
	sem_post(&data_available_sem); 
}

void* serialRead(void* arg)
{
	int fd = *((int*)arg);
	int nbytes;
	char buffer[SERIAL_READ_BUFFER_SIZE];

	debug(1, "Read thread up!\n");

	while(!done) {
		while(((nbytes=read(fd, buffer, SERIAL_READ_BUFFER_SIZE)) > 0) && (!done)) {
			enqueueRange(&readQueue, buffer, nbytes);
			sem_post(&data_available_sem);
			usleep(50000);
		}
	}

	debug(1, "%s Read thread closing\n", programName);
	pthread_exit((void *)0);
}

int main(int argc, char **argv)
{
	{
		char **sp = argv;
		while( *sp != NULL ) {
			programName = strsep(sp, "/");
		}
	}

	if( argc < 2 ) {
		printHelp();
		exit(0);
	}
	portName = argv[1];

	short USING_TIMESYNC = 0;
	if( argc >= 2 ) {
		if(strcmp(argv[2], "GPS_TIMESYNC") == 0) USING_TIMESYNC = 0x0003;
		else if(strcmp(argv[2], "RTC_TIMESYNC") == 0) USING_TIMESYNC = 0x0001;
		else if(strcmp(argv[2], "NO_TIMESYNC") == 0) USING_TIMESYNC = 0x0000;
		else {printHelp(); exit(0);}
	}
	debug(1,"Timesync option: %d\n", USING_TIMESYNC);

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

	register_sig_handler();
	int runtime_counter = read_rt_count();

	log_t log = {"gps", "", runtime_counter, 0, 0};
	if(creat_log(&log) < 0) {
		return -1;
	}

	int tty_fd, readThread_tp;
	int indexOfStartToken;
	int indexOfEndToken;
	int lengthOfNmeaString;
	
	int file_idx = 0;
	char ascii_checksum[3];
	char buffer[PARSER_READ_BUFFER_SIZE];
	readQueue = queueCreate();
	struct RMC_DATA gpsRMCData;
	struct timespec ts_begin;

	//Prepare serial PORT
	struct termios tio;
	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag &= ~OPOST;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 0;

	tty_fd=open(portName, O_RDWR | O_NONBLOCK);
	if( tty_fd < 0 ) {
		fprintf(stderr, "Cannot open port %s. Open return %d", portName, tty_fd);
		exit(0);
	}

	cfsetospeed(&tio,B9600);
	cfsetispeed(&tio,B9600);
	tcsetattr(tty_fd, TCSANOW, &tio);
	usleep(1000);
	tcflush(tty_fd, TCIOFLUSH);

	// Prepare serial read THREAD
	pthread_t serialReadThread;
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	readThread_tp = pthread_create( &serialReadThread, &thread_attr, serialRead, (void *)&tty_fd);
	if( readThread_tp ) {
  		printf("[%s] ERROR read thread: Return code from pthread_create(): %d\n", programName, readThread_tp);
  		exit(1);
	}
	pthread_attr_destroy(&thread_attr);

	// Setup GPS chip
	writeMessageToPort(disableAllMessages, sizeof(disableAllMessages), tty_fd);
	writeMessageToPort(enableRMC, sizeof(enableRMC), tty_fd);
	writeMessageToPort(enableTenHz, sizeof(enableTenHz), tty_fd);

	while(!done) {
		sem_wait(&data_available_sem);
		clock_gettime(CLOCK_REALTIME, &ts_begin);

		// Search for start token
		indexOfStartToken = queueGetFirstIndexOfToken(&readQueue, '$');
		if( indexOfStartToken < 0 ) { // Not found start token
			continue;
		}
		else if( indexOfStartToken > 0 ) { // String not aligned. Dequeue to align start token to index 0
			debug(1, "Dequeuing indexOfStartToken %d - q_count: %d\n", indexOfStartToken, queueCount(&readQueue));
			dequeueRange(&readQueue, buffer, indexOfStartToken);
			indexOfStartToken = 0;
		}

		// Search for end token
		indexOfEndToken = queueGetFirstIndexOfToken(&readQueue, '\n');
		if( indexOfEndToken < 0 ) { // Not found end token
			continue;
		}

		// Dequeue string from read buffer
		lengthOfNmeaString = (indexOfEndToken+1) - indexOfStartToken;
		dequeueRange(&readQueue, buffer, lengthOfNmeaString);
		buffer[lengthOfNmeaString] = '\0';

		// Perform checksum check
		snprintf(ascii_checksum, 3, "%02X", calcCRC(buffer+1, lengthOfNmeaString-1));
		if(memcmp(buffer+lengthOfNmeaString-4, ascii_checksum, 2) != 0) {
			debug(1, "[%s] WARNING Checksum mismatch: %s : %s", programName, ascii_checksum, buffer);
			continue;
		}

		// Finally we got a correct NMEA string - Log it on SD card!
		if ( append_log(&log, buffer, lengthOfNmeaString) < 0 ) {
			fprintf(stderr, "[%s] WARNING Could not log data to SD card. Still continueuing\n", programName);
		}

		if( parseGPRMC(buffer, &gpsRMCData) < 0 ) { // This function will replace all ',' with '\0'
			fprintf(stderr, "[%s] WARNING Failed parsing GPRMC message\n", programName);
			continue;
		}

		// Check if local time and date are to be set to GPS time. Do set only once.
		if( USING_TIMESYNC ) {
			if ( USING_TIMESYNC & 0x0001 ){ // RTC Sync
				printf("Time to set: %02d/%02d %02u %02d:%02d:%02d\n", 
					gpsRMCData.dateDay, 
					gpsRMCData.dateMonth, 
					(unsigned int)gpsRMCData.dateYear, 
					gpsRMCData.timeHour, 
					gpsRMCData.timeMinute, 
					gpsRMCData.timeSecond);

				setDate(gpsRMCData.dateDay, 
						gpsRMCData.dateMonth, 
						gpsRMCData.dateYear+(gpsRMCData.dateYear>30?1900:2000), 
						gpsRMCData.timeHour, 
						gpsRMCData.timeMinute, 
						gpsRMCData.timeSecond);
				fprintf(stderr, "[%s] Local time set from RTC\n", programName);
				USING_TIMESYNC &= ~0x0001;
			}

			if( (USING_TIMESYNC & 0x0002) && (gpsRMCData.positionFix != NO_FIX_INVALID) ) {
				setDate(gpsRMCData.dateDay, 
						gpsRMCData.dateMonth, 
						gpsRMCData.dateYear+(gpsRMCData.dateYear>30?1900:2000), 
						gpsRMCData.timeHour, 
						gpsRMCData.timeMinute, 
						gpsRMCData.timeSecond);
				fprintf(stderr, "[%s] Local time set from GPS\n", programName);
				USING_TIMESYNC &= ~0x0002;
			}
		}

		// Formating string to write to MMAP'ed sensor file. Reuse/overwrite buffer for this purpose.
		memset(buffer, 0, PARSER_READ_BUFFER_SIZE);
		lengthOfNmeaString = format_timespec(buffer, &ts_begin);

		if(gpsRMCData.positionFix == NO_FIX_INVALID) {
			snprintf( buffer + lengthOfNmeaString,
				PARSER_READ_BUFFER_SIZE - lengthOfNmeaString, 
				",,,,,,,,,,,,,,,,,\n" );
		} else {
			snprintf(buffer + lengthOfNmeaString,
				PARSER_READ_BUFFER_SIZE - lengthOfNmeaString, 
				",%02d,%02d,%02d,%03d,%02d,%02d,20%02d,%02d,%02d,%.3f,%c,%03d,%02d,%.3f,%c,%3.2f,%3.2f\n",
				gpsRMCData.timeHour,
				gpsRMCData.timeMinute,
				gpsRMCData.timeSecond,
				gpsRMCData.timeMillisecond,
				gpsRMCData.dateDay,
				gpsRMCData.dateMonth,
				gpsRMCData.dateYear,
				gpsRMCData.latitudeDegrees,
				gpsRMCData.latitudeMinutes,
				gpsRMCData.latitudeSeconds,
				gpsRMCData.latitudeHemisphere,
				gpsRMCData.longitudeDegrees,
				gpsRMCData.longitudeMinutes,
				gpsRMCData.longitudeSeconds,
				gpsRMCData.longitudeHemisphere,
				gpsRMCData.speedOverGround,
				gpsRMCData.courseOverGround );
		}

		debug(1, "%s", buffer);
		mm_append(buffer, &gps_mapped_file);
	}

	pthread_join(serialReadThread, NULL);
	printf("Serial read queue max count: %d\n", queueMaxCount(&readQueue));
	close(tty_fd);
	queueDestroy(&readQueue);
	printf("%s Terminating\n", programName);
	return 0;
}
