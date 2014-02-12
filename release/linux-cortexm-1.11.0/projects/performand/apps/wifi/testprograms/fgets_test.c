#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#ifndef VERBOSITY
#warning "VERBOSITY not defined:ty Using VERBOSITY=1"
#define VERBOSITY 1
#endif

#define debug(level, str, ...) { if( level <= VERBOSITY ) printf(str, ## __VA_ARGS__); }

#define MAXSIZE 40
#define MAXTOKEN 20

#define SENSOR_DIR_PATH   "/sensors/"
#define PARSER_DIR_PATH   "/sensors/parser/"
#define MAX_SENSOR_COUNT  10
#define MAX_PARSER_COUNT  2
#define MAX_TOKEN_COUNT   20
#define MAX_TOKEN_LENGTH  50
#define PARSER_FILE_SIZE  512
#define TCP_CHUNK         1400
#define START_TOKEN       '$'
#define LS_LINE_OFFSET    57

#define LOCK_FILE(file_name, fd) { fl.l_pid = getpid(); \
					  			   fd = open(file_name, O_RDONLY); \
					  			   if(fd < 0){ fprintf(stderr, "Can not open file %s\n", file_name); continue; } \
					  			   fcntl(fd, F_SETLKW, &fl); }

#define UNLOCK_FILE(fd) { fl.l_type  = F_UNLCK; \
						  fcntl(fd, F_SETLK, &fl); \
						  close(fd); }

#define GET_FILE_SIZE(file_size, fd) { file_size = lseek(fd, 0, SEEK_END); \
								       lseek(fd, 0, SEEK_SET); }

int main (int argc, char **argv)
{
	FILE *pp;
	char lsCommandLine[25];
	char buffer[100];
	char *line;

	printf("Path: %s\n", PARSER_DIR_PATH);
	sprintf(lsCommandLine, "ls -l %s", PARSER_DIR_PATH);

	pp = popen(lsCommandLine, "r");
	while(fgets(buffer, 100, pp) != NULL) {
		line = buffer;
		printf("line: %s", line);
		printf("buffer: %s", buffer);
	}


	return 0;
}









