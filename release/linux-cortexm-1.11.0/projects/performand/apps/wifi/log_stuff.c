#include <stdio.h>

#include "../Common/utils.h"
#include "../Common/verbosity.h"
#include "log_stuff.h"


int runtime_count;
int file_idx;
int fd;

int log_init()
{
	runtime_count = read_rt_count();
	debug(2, "Runtime count: %d\n", runtime_count);
	file_idx = 0;
	return 0;
}

int log_store_data(char *data, int len)
{
	FILE *file;
	int size = 0;
	char format[] = "/sdcard/datalog-all%d.%d.txt";
	char filename[sizeof format+10];
	sprintf(filename, format, runtime_count, file_idx);	

	file = fopen(filename, "a+"); 
	fseek(file, 0, SEEK_END);
   	size = ftell(file);
	if(size > LOG_FILE_MAX_SIZE) {
		fclose(file);
		file_idx += 1;
		sprintf(filename, format, runtime_count, file_idx);
		file = fopen(filename,"a+");
	}

	fwrite(data, 1, len, file);
//	fwrite("\n", 1, 1, file);

	fclose(file);
	return 0;
}

void log_end()
{


}

