#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>	//---for memset
#include <signal.h>	//---for memset


#define FILE_LENGTH 0x1000

int main (int argc, char* const argv[])
{
	int fd;
	char* file_memory;
	int count=0;
	int fileptr=0;
	int len=0;

	/* Prepare a file large enough to hold an unsigned integer.*/
	fd = open ("/gps", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, FILE_LENGTH+1, SEEK_SET);

	write (fd, "", 1);
	lseek (fd, 0, SEEK_SET);
	/* Create the memory mapping. */
	file_memory = mmap (0, FILE_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);
	close (fd);

	/* Flush file */
	memset(file_memory, '\0', sizeof(char)*FILE_LENGTH);

	/* Write a random integer to memory-mapped area. */
	while(1) {
		fileptr = *file_memory;
		fileptr = (fileptr<<8) + *(file_memory+1);

		len = sprintf((char*) file_memory+fileptr+2, "%02d\r\n", count++);
	
		if( (len+fileptr) < FILE_LENGTH+2) {
			*file_memory = (char)(((len+fileptr) & 0xFF00)>>8);
			*(file_memory+1) = (char)((len+fileptr) & 0xFF);
		}

		if(fileptr > FILE_LENGTH)
			fileptr=0;

		usleep(100000);
	}

	/* Release the memory (unnecessary because the program exits). */
	munmap (file_memory, FILE_LENGTH);
	return 0;
}

