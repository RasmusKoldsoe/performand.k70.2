#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/GPIO/gpio_api.h"
#include "../Common/verbosity.h"
#include "../Common/utils.h"
#include "tcp_stuff.h"
#include "log_stuff.h"

//#define MAXSIZE 40
//#define MAXTOKEN 20

#define SENSOR_DIR_PATH   "/sensors/"
#define PARSER_DIR_PATH   "/nand/parser/"
#define MAX_SENSOR_COUNT  5
#define MAX_PARSER_COUNT  2
#define MAX_TOKEN_COUNT   29
#define MAX_TOKEN_LENGTH  40
#define STD_LINE_LENGTH  700
#define START_TOKEN       '$'
#define LS_LINE_OFFSET    57
//#define NAME_MAX MAX_TOKEN_LENGTH
#define SAMPLE_PERIOD_US 500000
/*
#define LOCK_FILE(file_name, fd) { fl.l_pid = getpid(); \
					  			   fd = open(file_name, O_RDONLY); \
					  			   if(fd < 0){ fprintf(stderr, "Can not open file %s\n", file_name); continue; } \
					  			   fcntl(fd, F_SETLKW, &fl); }

#define UNLOCK_FILE(fd) { fl.l_type  = F_UNLCK; \
						  fcntl(fd, F_SETLK, &fl); \
						  close(fd); }

#define GET_FILE_SIZE(file_size, fd) { file_size = lseek(fd, 0, SEEK_END); \
								       lseek(fd, 0, SEEK_SET); }
*/
static char *programName = NULL;
static char done;

struct parse_element { char token_count;
                       char delimitor[5];
                       char show_unused_tokens;
                       char delim_at_final_token;
                       char parse_str[MAX_TOKEN_COUNT][MAX_TOKEN_LENGTH];
                     };

struct sensor_element { char name[NAME_MAX];
						char fullpath[NAME_MAX];
                        struct parse_element parser[MAX_PARSER_COUNT];
						h_mmapped_file mmapped_file;
                      };
/*
struct output_element { char filename[NAME_MAX];
                        int fd;
                        int parserID;
                      };
*/
int match_sensorname(struct sensor_element* sensors, char* file_name, int sensor_count)
{
	while(sensor_count > 0) {
		if( strcmp(sensors[--sensor_count].name, file_name) == 0 )
			return sensor_count;
	}
	return -1;
}

void sigint_handler(int sig)
{
	printf("[%s] Terminating TCP/IP stream.\n", programName );
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

void pretty_print_parse_element( struct parse_element *element )
{
#if VERBOSITY >= 3 
	printf("token_count: %d\n", element->token_count);
	printf("delimitor: %s\n", strcmp(element->delimitor, "\r\n")==0?"<cr><lf>":element->delimitor);
	printf("show_unused_tokens: %d\n", element->show_unused_tokens);
	printf("delim_at_final_token: %d\n", element->delim_at_final_token);
	printf("parse_str: ");
	int i;
	for(i=0; i<element->token_count; i++) {
		printf("%s ", element->parse_str[i]);
	}
	printf("\n\n");
#endif
}

int read_parser_line( struct parse_element *element, char *line, struct sensor_element *sensors)
{
	int count, rcount;
	const int orig_len = strlen(line);
	char delim[5];

	{
		int token_count, show_unused_tokens, delim_at_final_token;
		sscanf(line, "%d '%[^']' %d %d", &token_count, delim, &show_unused_tokens, &delim_at_final_token);
		element->token_count = (char)token_count;
		element->show_unused_tokens = (char)show_unused_tokens;
		element->delim_at_final_token = (char)delim_at_final_token;
	}

	if( element->token_count <= 0 )
		return -1;

	if( delim[0] == '\\' ) {
		if( delim[1] == 'n' || delim[1] == 'r') sprintf(element->delimitor, "\r\n");
		else if(delim[1] == 't' ) sprintf(element->delimitor, "\t");
	}
	else {
		snprintf(element->delimitor, 2, "%s", delim);
	}

	for(rcount=0; rcount < orig_len && *(line+rcount) != '"'; rcount++); // Find first quote (") sign in line

	for(count=0; count < element->token_count && rcount < orig_len; count++) { 
		sscanf(line+rcount, "\"%[^\"]\"",  element->parse_str[count]);
		rcount += strlen(element->parse_str[count]) + 3; 
	}

	return 0;
}

int get_folder_content ( char *folderName, char entries[][NAME_MAX] )
{
	FILE *pp;
	char buffer[100];
	char lsCommandLine[100];
	int count = 0;

	sprintf(lsCommandLine, "ls -l %s", folderName);

	pp = popen( lsCommandLine, "r" );
	while( fgets(buffer, 100, pp) != NULL ) {
		buffer[strlen(buffer)-1] = '\0';
		if(buffer[0] == '-')
			sprintf(entries[count++], "%s", buffer+LS_LINE_OFFSET);
	}

	pclose(pp);
	return count;
}

int get_sensor_entries ( struct sensor_element *sensors )
{
	char sensorEntries[MAX_SENSOR_COUNT][NAME_MAX];
	int entryCount = get_folder_content( SENSOR_DIR_PATH, sensorEntries );

	int i;
	for(i=0; i < entryCount; i++) {
		strncpy(sensors[i].name, sensorEntries[i], NAME_MAX);
		strncpy(sensors[i].fullpath, SENSOR_DIR_PATH, NAME_MAX);
		strcat(sensors[i].fullpath, sensorEntries[i]);

		//Prepare memory mapping for sensor
		memset(&sensors[i].mmapped_file, 0, sizeof(h_mmapped_file));	
		sensors[i].mmapped_file.filename = sensors[i].fullpath;
		sensors[i].mmapped_file.size = DEFAULT_FILE_LENGTH;
		if((mm_prepare_mapped_mem(&sensors[i].mmapped_file)) < 0) {
			printe("Error mapping %s file.\n",sensors[i].mmapped_file.filename);
			continue;	
		}
	}

	return entryCount;
}

int get_parser_entries ( struct sensor_element *sensors, int sensor_count)
{

	char parserEntries[MAX_SENSOR_COUNT][NAME_MAX];
	int entryCount = get_folder_content( PARSER_DIR_PATH, parserEntries );

	if ( sensor_count != entryCount )
		printw("Sensor and Parser count don't match\n");

	for (entryCount--; entryCount >= 0; entryCount--) {
		int sensor_id = match_sensorname(sensors, parserEntries[entryCount], sensor_count);
		if (sensor_id < 0) {
			printw("Could not find sensor for parser %s\n", parserEntries[entryCount]);
			continue;
		}

		char fullfilepath[NAME_MAX];
		snprintf(fullfilepath, NAME_MAX, "%s%s", PARSER_DIR_PATH, sensors[sensor_id].name);

		char buffer[STD_LINE_LENGTH];
		int parser_count = 0;

		FILE *fp = fopen(fullfilepath, "r");
		if( fp == NULL ) {
			printe("Could not open parser file %s\n", fullfilepath);
			return -2;
		}

		while( fgets(buffer, STD_LINE_LENGTH, fp) != NULL ) {
			read_parser_line( &sensors[sensor_id].parser[parser_count++], buffer, sensors );
		}

		fclose(fp);
	}

	for(sensor_count--; sensor_count >= 0; sensor_count--) {
		if(sensors[sensor_count].parser[0].token_count <= 0 )
			printw("Could not find parser for sensor %s\n", sensors[sensor_count].name );
	}
	return 0;
}

int parse_line(char *i_str, char *o_str, struct parse_element *element)
{
// At this point we can trust that i_str is \0 terminated string containing only data (= no $ token)
	int count, rcount;
	char data_str[MAX_TOKEN_COUNT][MAX_TOKEN_LENGTH];
	memset(data_str, '\0', sizeof(data_str));

	for(count=0, rcount=0; count < element->token_count; count++) {
		sscanf(i_str+rcount, "%[^,]", data_str[count]);
		rcount += strlen(data_str[count]) + 1;
//printf("data token: %s\trest: %s\n", data_str[count], i_str+rcount);
	}

	char f_data[STD_LINE_LENGTH];
	int f_data_len = 0;
	memset(f_data, '\0', sizeof(f_data));

	for(count=1; count<element->token_count; count++) {
		if( element->show_unused_tokens == 0 && strlen(data_str[count]) == 0 )
			continue;
//printf("data token: %s\tline token: %s\n", d[count], s[count]);	
		f_data_len += sprintf(f_data + f_data_len, "%s", element->delimitor);
		f_data_len += sprintf(f_data + f_data_len, element->parse_str[count], data_str[count] );
	}

	if( element->delim_at_final_token > 0 ) {
		f_data_len += sprintf(f_data + f_data_len, "%s", element->delimitor);
	}

	int i = sprintf(o_str, element->parse_str[0], data_str[0], f_data);
	strcat(o_str, "\r\n");

	return i+2;
}

void *ledIndicatorThread(void *sem) {
	sem_t *sample_sem = (sem_t *)sem;

	if(!gpio_export(LED_IND2))
		printf("[%s] ERROR: Exporting gpio port: %d LED_IND2\n", programName, LED_IND2);
	
	if(!gpio_setDirection(LED_IND2, GPIO_DIR_OUT))
		printf("[%s] ERROR: Set gpio port direction: %d LED_IND2\n", programName, LED_IND2);

	gpio_setValue(LED_IND2, GPIO_SET_LOW);

	while( !done ) {
		sem_wait(sample_sem);
		gpio_setValue(LED_IND2, GPIO_SET_HIGH);
		usleep(75000);
		gpio_setValue(LED_IND2, GPIO_SET_LOW);
	}

	if(!gpio_unexport(LED_IND2))
		printf("[%s] ERROR: UnExporting gpio port: %d LED_IND2\n", programName, LED_IND2);

	return NULL;
}

int main (int argc, char **argv)
{
	char **sp = argv;
	while( *sp != NULL ) {
		programName = strsep(sp, "/");
	}

	struct timespec spec, beginning, final;
	struct sensor_element sensors[MAX_SENSOR_COUNT];
	pthread_t ledIndication_thread;
	sem_t sample_start_sem;

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	sem_init(&sample_start_sem, 0, 0);

	register_sig_handler();
	int entryCount = get_sensor_entries ( sensors );
	get_parser_entries( sensors, entryCount);

	int i;
	for(i = 0; i < entryCount; i++) {
		pretty_print_parse_element( &sensors[i].parser[0] );
		pretty_print_parse_element( &sensors[i].parser[1] );
	}
// At this point we know all sensors and have loaded their parsers

	FILE *fp;
	unsigned long process_time;
	int length;
	char buffer[STD_LINE_LENGTH];
	char outb[STD_LINE_LENGTH];
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	fl.l_pid = getpid();

	tcp_init("/dev/ttyS3");
	log_init();

	pthread_create(&ledIndication_thread,&thread_attr,ledIndicatorThread, (void *)&sample_start_sem);

	while ( !done ) {
		/*
		Foreach sensor in sensors
			open file
			while !EOF
				Read sensor line
				foreach output in outputs
					Parse line with and output according to parser and file specified by output
			close file
		Schedule for later
		*/
		sem_post(&sample_start_sem);
		clock_gettime(CLOCK_REALTIME, &beginning);
		/* ****** START TIMING ****** */

		for( i=0; i < entryCount; i++) {
			fp = fopen(sensors[i].fullpath, "rw");
			if(fp == NULL) {
				printe("Cannot open sensor file %s\n", sensors[i].fullpath);
				continue;
			}

			fl.l_type  = F_WRLCK;
			fcntl(fileno(fp), F_SETLK, &fl);
			fseek(fp, MM_FILE_INDEX_SIZE, SEEK_SET);

			/* ****** CRITICAL REGION *********** */

			while( fgets(buffer, STD_LINE_LENGTH, fp) != NULL) {
				if(strlen(buffer) == 0) break;

				buffer[strlen(buffer)-1] = '\0';

				length = parse_line( buffer, outb, &sensors[i].parser[0] ); // XML Parser
				tcp_send_data(outb, length);
//printf("%s", outb);

				length = parse_line( buffer, outb, &sensors[i].parser[1] ); // Raw Parser
				log_store_data(outb, length);
//printf("%s", outb);
			}

			reset_mapped_mem(&sensors[i].mmapped_file);

			/* ****** END CRITICAL REGION ******* */
			fl.l_type  = F_UNLCK;
			fcntl(fileno(fp), F_SETLK, &fl);
			fclose(fp);
		}

		/* ****** END TIMING ****** */
		clock_gettime(CLOCK_REALTIME, &final);
		spec = subtract_timespec(final, beginning);
		process_time = spec.tv_sec * USEC_PER_SEC + spec.tv_nsec / NSEC_PER_USEC;
		if(process_time > SAMPLE_PERIOD_US) process_time = SAMPLE_PERIOD_US;
		else if(process_time < 0) process_time = 0;

		//printf("%lu ", process_time); fflush(stdout);
		usleep(SAMPLE_PERIOD_US - process_time);
	}

/*	for( i=0; i<sizeof(outputs)/sizeof(struct output_element); i++) {
		close(outputs[i].fd);
	}*/
	tcp_end();
	log_end();

	return 0;
}

















