/* Includes */
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <string.h>     /* String handling */
#include <unistd.h>     /* for close() */

#include <math.h>
#include <sys/statfs.h>
#include <time.h>
#include <sys/time.h>

#include "../Common/common_tools.h"
#include "../Common/utils.h"

#define MAX_FILE_SIZE 5242880 //5MB

int setDate(int dd, int mm, int yy, int h, int min, int sec)  // format like MMDDYY
{
	struct timeval tv;
	struct tm time_to_set;

	time_to_set.tm_hour = h;
	time_to_set.tm_min = min;
	time_to_set.tm_sec = sec;
	time_to_set.tm_year = yy - 1900;
	time_to_set.tm_mon = mm-1;
	time_to_set.tm_mday = dd;

	// Make new system time.
	if ((tv.tv_sec = mktime(&time_to_set)) == (time_t)-1) {
		printf("Cannot convert system time\n");
		return -1;
	}

	// Set new system time.
	if (settimeofday(&tv, NULL) != 0){
		printf("Cannot set system time\n");
		return -1;
	}

	// Get current date & time since Epoch.
	if (gettimeofday(&tv, NULL) != 0) {
		printf("Cannot get current date & time since Epoch.\n");
		return -1;
	}

	// Calculate local time.
	if (localtime_r(&tv.tv_sec, &time_to_set) != &time_to_set) {
		printf("Cannot convert file time to local file time.\n");
		return -1;
	}

	debug(1, "Set new date/time from GPS: %d.%02d.%02d - %02d:%02d:%02d\r\n",
	time_to_set.tm_year + 1900, time_to_set.tm_mon + 1,
	time_to_set.tm_mday, time_to_set.tm_hour, time_to_set.tm_min,
	time_to_set.tm_sec);

	return 0;
}

/**
 * set_normalized_timespec - set timespec sec and nsec parts and normalize
 *
 * @ts:		pointer to timespec variable to be set
 * @sec:	seconds to set
 * @nsec:	nanoseconds to set
 *
 * Set seconds and nanoseconds field of a timespec variable and
 * normalize to the timespec storage format
 *
 * Note: The tv_nsec part is always in the range of
 *	0 <= tv_nsec < NSEC_PER_SEC
 * For negative values only the tv_sec field is negative !
 */

void set_normalized_timespec(struct timespec *ts, time_t sec, signed long nsec)
{
	while (nsec >= NSEC_PER_SEC) {
		/*
		 * The following asm() prevents the compiler from
		 * optimising this loop into a modulo operation. See
		 * also __iter_div_u64_rem() in include/linux/time.h
		 */
		asm("" : "+rm"(nsec));
		nsec -= NSEC_PER_SEC;
		++sec;
	}
	while (nsec < 0) {
		asm("" : "+rm"(nsec));
		nsec += NSEC_PER_SEC;
		--sec;
	}
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

struct timespec subtract_timespec(struct timespec lhs, struct timespec rhs)
{
	struct timespec ts_delta;
	set_normalized_timespec(&ts_delta, lhs.tv_sec - rhs.tv_sec,
				lhs.tv_nsec - rhs.tv_nsec);
	return ts_delta;
}

void format_timespec(char* str, struct timespec *ts)
{
	char tmbuf[100];
	struct tm tmp;

	if(localtime_r(&(ts->tv_sec), &tmp) == NULL) {
		fprintf(stderr, "ERROR: format_timespec - localtime_t()\n");
		sprintf(str, " ");
		return;
	}

	//strftime(tmbuf, sizeof(tmbuf),  "%Y-%m-%d %H:%M:%S", &tmp);
	//sprintf(str, "%s.%03ld", tmbuf, ts->tv_nsec/NSEC_PER_MSEC);
	sprintf(str, "%d.%ld", ts->tv_sec, ts->tv_nsec/NSEC_PER_MSEC);
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

int write_log_file(char *name, int runtime_count, int file_idx, char *receiveMessage) {
	FILE *file;
	int size=0;
 	struct statfs _statfs;
	long usedBytes;

	//if(statfs("/dev/mmcblk0p1", &_statfs)<0)
	//	printf("Error can't determine remain capacity on SD-Card\n");

	//usedBytes=(_statfs.f_blocks-_statfs.f_bfree)*_statfs.f_bsize;	 
	//printf("SD Card Capacity: %d\n",_statfs.f_blocks);

	char format[] = "/sdcard/datalog-%s" "%d.%d.txt";
	char filename[sizeof format+10];
	sprintf(filename,format, name, runtime_count, file_idx);	

	file = fopen(filename,"a+"); 

	fseek(file, 0, SEEK_END);
   	size = ftell(file);
	if(size >= MAX_FILE_SIZE) {
		fclose(file);
		file_idx+=1;
		sprintf(filename,format, name, runtime_count, file_idx);
		file = fopen(filename,"a+");
	}
	
	fwrite(receiveMessage,1,strlen(receiveMessage),file);

	fclose(file);

	return file_idx;
}

int read_rt_count(void) {
		FILE *ptr_myfile;
		int runtime_count = 0;
		char * line = NULL;
		size_t len = 0;
		ssize_t read;

		ptr_myfile=fopen("/sdcard/runtime_cnt.bin","rb");
		if (!ptr_myfile)
		{
			fprintf(stderr, "Error reading runtime count.\n");
			return -1;
		} else {
			fread(&runtime_count, sizeof(int), 1, ptr_myfile);
			printf("Runtime count: %d\n",runtime_count);
		}
		fclose(ptr_myfile);
		
		return runtime_count;
}

int rw_rnt_count(void) {
		FILE *ptr_myfile;
		int runtime_count = 0;

		char * line = NULL;
		size_t len = 0;
		ssize_t read;

		ptr_myfile=fopen("/sdcard/runtime_cnt.bin","rb");
		if (!ptr_myfile)
		{
			fprintf(stdout, "Unable to open runtime count file. Creating new file.\n");
			ptr_myfile = fopen("/sdcard/runtime_cnt.bin","wb+");
		} else {
			fread(&runtime_count, sizeof(int), 1, ptr_myfile);
		}
		fclose(ptr_myfile);
		runtime_count++;

		ptr_myfile=fopen("/sdcard/runtime_cnt.bin","wb");

		if (!ptr_myfile) {
			fprintf(stderr, "Unable to open runtime count file for write.\n");
			return -1;
		}
		if (!fwrite(&runtime_count, sizeof(int), 1, ptr_myfile)) {
			fprintf(stderr, "Unable to write runtime count file.\n");
			return -1;
		}
/*
		if (!fflush(ptr_myfile)) {
			fprintf(stderr, "Unable to flush file.\n");
		}
*/
		fclose(ptr_myfile);

		return runtime_count;
}
