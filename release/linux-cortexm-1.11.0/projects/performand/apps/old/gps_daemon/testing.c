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

int main(int argc, char **argv)
{
	FILE * fp;
	int memfp, len;
	char * line = NULL;
	size_t fmemsize = 0;
	ssize_t read;
	int line_counter=0;
	char* file_memory;

	GS_HAL_uart_set_comm_port("/dev/ttyS3");

	//GPS Memory Mapping
	/* Open the file. */
	memfp = open ("gps", O_RDWR, S_IRUSR | S_IWUSR);

	/* Create the memory mapping. */
	file_memory = mmap (0, FILE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, memfp, 0);
	printf("Mapped GPS file memory at: %X",file_memory);
	close (memfp);

	while(1) {
		fp = fopen("/dev/imu", "r");
		
		if (fp!=0) {
			while ((read = getline(&line, &len, fp)) != -1) {		   
				//AtLib_BulkDataTransfer(0x31, line, strlen(line));
				printf("%s",line);
			}
			fclose(fp);
		} else {
			//ADD STATUS ERROR ...
		}
	
		
		fmemsize = *file_memory;
		fmemsize = (fmemsize<<8) + *(file_memory+1);

		print_char_array(file_memory, fmemsize, 2);
		
		//AtLib_BulkDataTransfer(0x31, file_memory+2, fmemsize+2);

		*file_memory=0;
		*(file_memory+1)=0;

		/* Flush file */
		//memset(file_memory, '\0', sizeof(char)*FILE_LENGTH);

		usleep(500000);
	}
	exit(1);
}
