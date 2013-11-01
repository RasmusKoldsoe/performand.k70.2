#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define FILE_LENGTH 0x100

int main (int argc, char* const argv[])
{
	int fd;
	void* file_memory;
	int integer;
	char buffer[255];
	
	FILE* fifo = fopen ("/dev/fifo", "r");
	fscanf (fifo, "%s", &buffer);
	fclose (fifo);
	printf("%s",buffer);

	return 0;
}

