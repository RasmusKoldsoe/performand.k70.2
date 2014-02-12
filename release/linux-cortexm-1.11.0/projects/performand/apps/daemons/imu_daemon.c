#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/verbosity.h"
#include "../Common/utils.h"

static char *programName = NULL;
static char done = 0;

void sigint_handler(int sig)
{
	printf("[%s] Terminating.\n", programName );
	done = 1;
}

void register_sig_handler()
{
	struct sigaction sia;

	memset(&sia, 0, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(1);
	}
	done = 0;
}

int main(int argc, char **argv)
{
	char **sp = argv;
	while( *sp != NULL ) {
		programName = strsep(sp, "/");
	}

	FILE *fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int runtime_count = 0;
	char buffer[512];
	char *bufptr; 
	int file_idx = 0;

	h_mmapped_file mapped_file;
	memset(&mapped_file, 0, sizeof(h_mmapped_file));

	mapped_file.filename = "/sensors/imu";
	mapped_file.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&mapped_file)) < 0) {
		printe("Mapping IMU file %s\n", mapped_file.filename);
		return -1;	
	}

	done = 0;
	register_sig_handler();
	runtime_count = read_rt_count();
	
	bufptr = buffer;
	while( !done ) {
		fp = fopen("/dev/imu", "rw");
		if (fp == NULL) {
			printe("opening IMU device %s\n", "/dev/imu");
			break;
		}
	
		while ((read = getline(&line, &len, fp)) != -1) {
			memcpy(bufptr, line, read);	
			bufptr+=read;				
		}
		bufptr='\0';
		mm_append(buffer, &mapped_file);
		file_idx = write_log_file("imu", runtime_count, file_idx, buffer);
		fclose(fp);
		bufptr = buffer;
		usleep(100000); // 100ms
	}

  	return 1;
}






