#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

int main(int argc, char **argv)
{

	int fd_gps;
	char* file_memory;
	int len;

	static char tcpipStr[256];
	char testdata[] = "Hello_World_";
	char tcpClientCID = 1;	

	printf("GS1500m TCP Stream v0.1\n");

	// Initialize Gainspan module, print module information and switch to client mode 
        GS_API_Init("/dev/ttyS3");

}
