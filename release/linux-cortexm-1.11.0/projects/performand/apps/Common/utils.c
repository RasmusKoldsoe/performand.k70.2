/* Includes */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <string.h>     /* String handling */

#include <math.h>
#include <sys/statfs.h>
#include <time.h>
#include <sys/time.h>

#include "../Common/common_tools.h"
#include "../Common/utils.h"

#define MAX_FILE_SIZE 5242880 //5MB


int lock_file(struct flock *fl, char *file_name, int *fd, ssize_t o_flags)
{
	fl->l_pid = getpid();
	*fd = open(file_name, o_flags);
	if(*fd < 0){ 
		fprintf(stderr, "[UTILS] Can not open file %s\n", file_name); 
		return -1;
	}
	fcntl(*fd, F_SETLKW, fl);
	return 0;
}

void unlock_file(struct flock *fl, int *fd)
{
	fl->l_type  = F_UNLCK;
	fcntl(*fd, F_SETLK, fl);
	close(*fd);
}


int setDate(int dd, int mm, int yy, int h, int min, int sec)  // format like MMDDYY
{
	time_t gps_time;
	struct tm time_to_set;
	struct timeval tv;


	//time( &gps_time );
	//time_to_set = localtime( &gps_time );
//printf("gps_time (1): %lu\n", gps_time);

	time_to_set.tm_hour = h;
	time_to_set.tm_min = min;
	time_to_set.tm_sec = sec;
	time_to_set.tm_year = yy-1900;
	time_to_set.tm_mon = mm-1;
	time_to_set.tm_mday = dd;

printf("Time to set: %02d/%02d %02d %02d:%02d:%02d\n", time_to_set.tm_mday, time_to_set.tm_mon+1, time_to_set.tm_year, time_to_set.tm_hour, time_to_set.tm_min, time_to_set.tm_sec);

	// Make new system time.
	gps_time = mktime(&time_to_set);

printf("gps_time (2): %lu\n", gps_time);

	
	tv.tv_sec = gps_time;// = {mktime(time_to_set), 0};
	tv.tv_usec = 0;

	// Set new system time.
	if (settimeofday(&tv, NULL) < 0){
		int err = errno;
		printf("Cannot set system time. Got error: "); fflush(stdout);
		if(err == EFAULT ) printf("EFAULT\n");
		if(err == EINVAL ) printf("EINVAL\n");
		if(err == EPERM  ) printf("EPERM\n");
		return -1;
	}

	/*debug(1, "Set new date/time from GPS: %d.%02d.%02d - %02d:%02d:%02d\r\n",
	time_to_set->tm_year + 1900, time_to_set->tm_mon + 1,
	time_to_set->tm_mday, time_to_set->tm_hour, time_to_set->tm_min,
	time_to_set->tm_sec);
*/
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

void set_normalized_timespec(struct timespec *ts, signed long sec, signed long nsec)
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

int format_timespec(char* str, struct timespec *ts)
{
	struct tm tmp;

	if(localtime_r(&(ts->tv_sec), &tmp) == NULL) {
		fprintf(stderr, "ERROR: format_timespec - localtime_t()\n");
		sprintf(str, " ");
		return -1;
	}

	return sprintf(str, "%d.%03ld", (int)ts->tv_sec, ts->tv_nsec/NSEC_PER_MSEC);
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

int write_log_file(char *name, int runtime_count, int* file_idx, char *receiveMessage)
{
	int fd;
	char filename[40];
	sprintf(filename,"/sdcard/datalog-%s%d.%d.txt", name, runtime_count, *file_idx);	

// Open is not called with O_APPEND because the filepointer would then already be placed 
// at the end of the file. Then lseek would not return the file size, just 0.
	if((fd = open(filename, O_CREAT|O_WRONLY|O_SYNC)) < 0) {
		fprintf(stderr, "ERROR: Could not open file %s\n", filename);
		return -1;
	}

	if( lseek(fd, 0, SEEK_END) >= MAX_FILE_SIZE) {
		close(fd);
		*file_idx += 1;
		sprintf(filename,"/sdcard/datalog-%s%d.%d.txt", name, runtime_count, *file_idx);
		if((fd = open(filename, O_CREAT|O_WRONLY)) < 0) {
			fprintf(stderr, "ERROR: Could not create new log file %s\n", filename);
			return -2;
		}
	}

	write(fd, receiveMessage, strlen(receiveMessage));	

	close(fd);

	return 0;
}

int read_rt_count(void) {
		int fd;
		int runtime_count = 0;

		if((fd = open("/sdcard/runtime_cnt.bin", O_RDONLY)) < 0) {
			fprintf(stderr, "Error opening runtime count file.\n");
			return -1;
		}

		read(fd, &runtime_count, sizeof(int));
		close(fd);

		return runtime_count;
}

int rw_rnt_count(void) {
		int fd;
		int runtime_count = 0;

		if ((fd = open("/sdcard/runtime_cnt.bin", O_RDWR|O_CREAT)) < 0)
		{
			fprintf(stderr, "Unable to create runtime count file. Aborting.\n");
			return -1;
		}

// Read will not read any bytes if the file pointer is at or past the file size.
// If the file is just created, the file size will be 0 and read will thus not overwrite 
// runtime_count, hence when incrementing runtime_count the counter will start at 1, 
// othwise the runtime_count read will be incremented.
		read(fd, &runtime_count, sizeof(int));
		
		runtime_count++;

		close(fd);

		return runtime_count;
}
