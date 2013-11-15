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
#include <pthread.h> 

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/utils.h"

#define SAMPLE_PERIOD_US 500000
#define MAX_FILE_SIZE 500000

int file_idx = 0;

void write_testlog_file(char *name,int runtime_count, char *receiveMessage, int len) {
	FILE *file;
	int size = 0;
	char format[] = "/sdcard/datalog_%s_" "%d.%d.txt";
	char filename[sizeof format+10];
	sprintf(filename,format, name, runtime_count, file_idx);	

	file = fopen(filename,"a+"); 
	fseek(file, 0, SEEK_END);
   	size = ftell(file);
	if(size > MAX_FILE_SIZE) {
		fclose(file);
		file_idx+=1;
		sprintf(filename,format, name, runtime_count, file_idx);
		file = fopen(filename,"a+");
	}

	fwrite(receiveMessage,1,len,file);

	fclose(file);
}


int main(int argc, char **argv)
{
	FILE * fp;
	int memfp, len;
	char * line = NULL;
	size_t fmemsize = 0;
	ssize_t read;
	int line_counter=0;
	int fd;
	int runtime_count = 0;

	struct timespec spec;
	long ms,s,ms_before, ms_after, process_time=0;
	long s_before, s_after;

	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	
	len=0;
	h_mmapped_file gps_mapped_file;
	h_mmapped_file imu_mapped_file;

	//Prepare memory mapping for gps
	memset(&gps_mapped_file, 0, sizeof(h_mmapped_file));
	gps_mapped_file.filename = "gps";
	gps_mapped_file.size = DEFAULT_FILE_LENGTH;
	mm_prepare_mapped_mem(&gps_mapped_file);

	//Prepare memory mapping for imu
	memset(&imu_mapped_file, 0, sizeof(h_mmapped_file));
	imu_mapped_file.filename = "imu";
	imu_mapped_file.size = DEFAULT_FILE_LENGTH;
	mm_prepare_mapped_mem(&imu_mapped_file);

	//GS_HAL_uart_set_comm_port("/dev/ttyS3");

	runtime_count = rw_rnt_count();	
	while(1) {

		
		//./mnt/apps/daemons/gps_daemon & ./mnt/apps/daemons/testing
		
		// Measure the time it takes to read out the 
		// data from both files to maintain a stable sample rate
		//**********************************************

		clock_gettime(CLOCK_REALTIME, &spec);
		ms_before = round(spec.tv_nsec / 1.0e6);
		s_before = spec.tv_sec; 
		
		//************* Read GPS file ....

		fl.l_pid = getpid();
		// get the file descriptor
		fd = open(gps_mapped_file.filename, O_WRONLY);  
		fcntl(fd, F_SETLKW, &fl);

		len = mm_get_next_available(&gps_mapped_file,0);

		if(len > 0) {
			write_testlog_file("all", runtime_count, gps_mapped_file.mem_ptr+2, len);
			
			//Stream the stuff out ...
			//AtLib_BulkDataTransfer(0x31, gps_mapped_file.mem_ptr+2, len);
			
			memset(gps_mapped_file.mem_ptr,0, gps_mapped_file.size);

			asm("dsb");
			usleep(5000);
		}
		
		fl.l_type   = F_UNLCK;  			// tell it to unlock the region
		fcntl(fd, F_SETLK, &fl); 			// set the region to unlocked
		close(fd);

		//************ Read IMU ---------------
		fl.l_pid = getpid();
		// get the file descriptor
		fd = open(imu_mapped_file.filename, O_WRONLY);  
		fcntl(fd, F_SETLKW, &fl);

		len = mm_get_next_available(&imu_mapped_file,0);

		if(len > 0) {
			write_testlog_file("all", runtime_count, imu_mapped_file.mem_ptr+2, len);
			
			//Stream the stuff out ...
			//AtLib_BulkDataTransfer(0x31, gps_mapped_file.mem_ptr+2, len);
			
			memset(imu_mapped_file.mem_ptr,0, imu_mapped_file.size);

			asm("dsb");
			usleep(5000);
		}
		
		fl.l_type   = F_UNLCK;  			// tell it to unlock the region
		fcntl(fd, F_SETLK, &fl); 			// set the region to unlocked
		close(fd);
		

		// How long did it take ?
		//**********************************************
		clock_gettime(CLOCK_REALTIME, &spec);
		ms_after = round(spec.tv_nsec / 1.0e6);	
		process_time = ((ms_after+(spec.tv_sec - s_before)*1000)- ms_before)*1000;
		if(process_time>=SAMPLE_PERIOD_US)
			process_time = SAMPLE_PERIOD_US;
		//printf("Process time %d - delay left: %d\n", process_time, SAMPLE_PERIOD_US-process_time);
		usleep(SAMPLE_PERIOD_US-process_time);
	}
	exit(1);
}
