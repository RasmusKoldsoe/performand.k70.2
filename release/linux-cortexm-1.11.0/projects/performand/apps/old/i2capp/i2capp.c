/*
 * This is a user-space application that reads /dev/sample
 * and prints the read characters to stdout
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>

#define BUFFER_SIZE 1024

void read_file(const char* fullpath, char* data)
{
	FILE* file = fopen(fullpath, "r");
	if (file != NULL) {
		size_t size = fread(data, sizeof(char), BUFFER_SIZE, file);
		if (size == 0) {
	    		fputs("Error reading file", stderr);
		} 
		else {
	   	 	data[++size] = '\0'; // Be safe.
		}
		fclose(file);
	}
}

int main(int argc, char **argv)
{
	unsigned int hum = 0, temp = 0;
	struct timeval now;
	char data[100];	

	//while(1) {
		gettimeofday(&now, NULL);
		printf("[%6d.%6d]: ", now.tv_sec,now.tv_usec);
	
		read_file("/dev/humidity", data);
		hum=atoi(strtok(data, ";"));
		temp=atoi(strtok(NULL,","));

		printf("Humidity: %02.1f Temperature: %02.1f\n", ((float)hum/16383)*100, ((float)temp/16383)*160-40);
		usleep(200000-10000);	
	//}
}
