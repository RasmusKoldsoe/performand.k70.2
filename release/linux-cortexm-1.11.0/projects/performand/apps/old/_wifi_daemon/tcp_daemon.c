#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define FILE_LENGTH 	0x1000	//1KB File memory


int main(int argc, char **argv)
{

	int fd_gps;
	char* file_memory;
	int len;

	gs1500m_startAP();

	/* Open the file. */
	//fd_gps = open ("gps", O_RDWR, S_IRUSR | S_IWUSR);

	/* Create the memory mapping. */
	//file_memory = mmap (0, FILE_LENGTH, PROT_READ | PROT_WRITE,MAP_SHARED, fd_gps, 0);
	//close (fd_gps);

	while(1)
	{
	//	usleep(1500000);
	//	printf("**** ****\n",file_memory);

	/*	len = *file_memory;
		len = (len<<8) + *(file_memory+1);

		print_char_array(file_memory, len, 2);

		*file_memory=0;
		*(file_memory+1)=0;

		/* Flush file */
//		memset(file_memory, '\0', sizeof(char)*FILE_LENGTH);

		//printf("**** ****\n",file_memory);
		gs1500m_sendTCPIPstring(" Z10005Hello\n",12);
		usleep(1000000);
	}
	exit(0);
}
