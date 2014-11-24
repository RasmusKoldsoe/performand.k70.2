#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
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
#include <sys/reboot.h>

#include "../Common/MemoryMapping/memory_map.h"
#include "../Common/GPIO/gpio_api.h"
#include "../Common/verbosity.h"
#include "../Common/utils.h"
#include "tcp_stuff.h"
#include "../Common/logging.h"

//#define MAXSIZE 40
//#define MAXTOKEN 20


#if __arm__
  #define SENSOR_DIR_PATH   "/sensors/"
  #define PARSER_DIR_PATH   "/nand/parser/"
#elif __i386__
  #define SENSOR_DIR_PATH   "/sensors/"
  #define PARSER_DIR_PATH   "/sensors/parser/"
#endif

#define MAX_SENSOR_COUNT  5
#define MAX_PARSER_COUNT  2
#define MAX_TOKEN_COUNT   29
#define MAX_TOKEN_LENGTH  40
#define STD_LINE_LENGTH  700
#define START_TOKEN       '$'
#define LS_LINE_OFFSET    57
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

static struct parse_element { char token_count;
                       char delimitor[5];
                       char show_unused_tokens;
                       char delim_at_final_token;
                       char parse_str[MAX_TOKEN_COUNT][MAX_TOKEN_LENGTH];
                     };

static struct sensor_element { char name[NAME_MAX];
						char fullpath[NAME_MAX];
						h_mmapped_file mmapped_file;
						struct parse_element parser[MAX_PARSER_COUNT];
                      };

static struct sensor_element sensors[5] = {
	{ 
		.name = "compass",
		.fullpath = "/sensors/compass",
		.parser = {{.token_count = 8,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<COMPASS timestamp=\"%s\">%s</COMPASS>",
									"<mag_x>%s</mag_x>",
									"<mag_y>%s</mag_y>",
									"<mag_z>%s</mag_z>",
									"<accel_x>%s</accel_x>",
									"<accel_y>%s</accel_y>",
									"<accel_z>%s</accel_z>",
									"<battery>%s</battery>" }
				},
				{.token_count = 8,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<COMPASS timestamp=\"%s\">%s</COMPASS>",
									"<mag_x>%s</mag_x>",
									"<mag_y>%s</mag_y>",
									"<mag_z>%s</mag_z>",
									"<accel_x>%s</accel_x>",
									"<accel_y>%s</accel_y>",
									"<accel_z>%s</accel_z>",
									"<battery>%s</battery>" }
				}}
	},
	{ 
		.name = "wind",
		.fullpath = "/sensors/wind",
		.parser = {{.token_count = 5,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<WIND timestamp=\"%s\">%s</WIND>",
							"<temperature>%s</temperature>",
							"<speed>%s</speed>",
							"<direction>%s</direction>",
							"<battery>%s</battery>"}
				},
				{.token_count = 5,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {"<WIND timestamp=\"%s\">%s</WIND>",
							"<temperature>%s</temperature>",
							"<speed>%s</speed>",
							"<direction>%s</direction>",
							"<battery>%s</battery>"}
				}}
	},
	{	 
		.name = "speedlog",
		.fullpath = "/sensors/speedlog",
		.parser = {{.token_count = 3,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<LOG timestamp=\"%s\">%s</LOG>",
							"<period>%s</period>",
							"<battery>%s</battery>"}
				},
				{.token_count = 3,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<LOG timestamp=\"%s\">%s</LOG>",
							"<period>%s</period>",
							"<battery>%s</battery>"}
				},}
	},
	{ 
		.name = "gps",
		.fullpath = "/sensors/gps",
		.parser = {{.token_count = 18,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<GPS timestamp=\"%s\">%s</GPS>",
							"<hours>%s</hours>",
							"<minutes>%s</minutes>",
							"<seconds>%s</seconds>",
					 		"<msecond>%s</msecond>",
							"<day>%s</day>",
		 					"<month>%s</month>",
							"<year>%s</year>",
	 						"<lat_deg>%s</lat_deg>",
 							"<lat_min>%s</lat_min>",
	 						"<lat_sec>%s</lat_sec>",
							"<lat_hem>%s</lat_hem>",
							"<long_deg>%s</long_deg>",
							"<long_min>%s</long_min>",
							"<long_sec>%s</long_sec>",
							"<long_hem>%s</long_hem>",
							"<SOG>%s</SOG>",
							"<COG>%s</COG>"}
				},
				{.token_count = 18,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<GPS timestamp=\"%s\">%s</GPS>",
							"<hours>%s</hours>",
							"<minutes>%s</minutes>",
							"<seconds>%s</seconds>",
					 		"<msecond>%s</msecond>",
							"<day>%s</day>",
		 					"<month>%s</month>",
							"<year>%s</year>",
	 						"<lat_deg>%s</lat_deg>",
 							"<lat_min>%s</lat_min>",
	 						"<lat_sec>%s</lat_sec>",
							"<lat_hem>%s</lat_hem>",
							"<long_deg>%s</long_deg>",
							"<long_min>%s</long_min>",
							"<long_sec>%s</long_sec>",
							"<long_hem>%s</long_hem>",
							"<SOG>%s</SOG>",
							"<COG>%s</COG>"}
				}}
	},
	{ 
		.name = "imu",
		.fullpath = "/sensors/imu",
		.parser = {{.token_count = 18,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<IMU timestamp=\"%s\">%s</IMU>",
							"<rawMag_x>%s</rawMag_x>",
							"<rawMag_y>%s</rawMag_y>",
							"<rawMag_z>%s</rawMag_z>",
							"<rawMag_time>%s</rawMag_time>",
							"<rawAccel_x>%s</rawAccel_x>",
							"<rawAccel_y>%s</rawAccel_y>",
							"<rawAccel_z>%s</rawAccel_z>",
							"<rawAccel_time>%s</rawAccel_time>",
							"<rawGyro_x>%s</rawGyro_x>",
							"<rawGyro_y>%s</rawGyro_y>",
							"<rawGyro_z>%s</rawGyro_z>",
							"<rawGyro_time>%s</rawGyro_time>",
							"<pitch>%s</pitch>",
							"<roll>%s</roll>",
							"<heading>%s</heading>",
							"<emaHeading>%s</emaHeading>",
							"<time>%s</time>"}
				},
				{.token_count = 18,
					.delimitor = "\r\n",
					.show_unused_tokens = 0,
					.delim_at_final_token = 1,
					.parse_str = {	"<IMU timestamp=\"%s\">%s</IMU>",
							"<rawMag_x>%s</rawMag_x>",
							"<rawMag_y>%s</rawMag_y>",
							"<rawMag_z>%s</rawMag_z>",
							"<rawMag_time>%s</rawMag_time>",
							"<rawAccel_x>%s</rawAccel_x>",
							"<rawAccel_y>%s</rawAccel_y>",
							"<rawAccel_z>%s</rawAccel_z>",
							"<rawAccel_time>%s</rawAccel_time>",
							"<rawGyro_x>%s</rawGyro_x>",
							"<rawGyro_y>%s</rawGyro_y>",
							"<rawGyro_z>%s</rawGyro_z>",
							"<rawGyro_time>%s</rawGyro_time>",
							"<pitch>%s</pitch>",
							"<roll>%s</roll>",
							"<heading>%s</heading>",
							"<emaHeading>%s</emaHeading>",
							"<time>%s</time>"}
				}}
	}
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

static void pretty_print_parse_element( struct parse_element *element )
{
#if VERBOSITY >= 0 
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

static int read_parser_line( struct parse_element *element, char *line, struct sensor_element *sensors)
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
		if(count == 0){
			int k;
			for(k=0; k<strlen(element->parse_str[count]); k++) {
				if(element->parse_str[count][k] == '\'') element->parse_str[count][k] = '\"';
			}
		}
	}

	return 0;
}

static int get_folder_content ( char *folderName, char entries[][NAME_MAX] )
{
	/*FILE *pp;
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

	pclose(pp);*/

	int count = 0;
	strcpy( entries[count++], "compass");
	strcpy( entries[count++], "gps");
	strcpy( entries[count++], "imu");
	strcpy( entries[count++], "speedlog");
	strcpy( entries[count++], "wind");

	return count;
}

static int get_sensor_entries ( struct sensor_element *sensors , int entryCount)
{
	char sensorEntries[MAX_SENSOR_COUNT][NAME_MAX];
	//int entryCount = get_folder_content( SENSOR_DIR_PATH, sensorEntries );

	int i;
	for(i=0; i < entryCount; i++) {
		//strncpy(sensors[i].name, sensorEntries[i], NAME_MAX);
		//strncpy(sensors[i].fullpath, SENSOR_DIR_PATH, NAME_MAX);
		//strcat(sensors[i].fullpath, sensorEntries[i]);

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

static int get_parser_entries ( struct sensor_element *sensors, int sensor_count)
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

static int parse_line(char *i_str, char *o_str, struct parse_element *element)
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
	memset(f_data, 0, STD_LINE_LENGTH);

	for(count=1; count<element->token_count; count++) {
		if( element->show_unused_tokens == 0 && strlen(data_str[count]) == 0 )
			continue;
//printf("data token: %s\tline token: %s\n", d[count], s[count]);	
		f_data_len += sprintf(f_data + f_data_len, "%s", element->delimitor); // Insert delimitor
		f_data_len += sprintf(f_data + f_data_len, element->parse_str[count], data_str[count] ); // Insert the actual formatted data
	}

	if( element->delim_at_final_token > 0 ) {
		f_data_len += sprintf(f_data + f_data_len, "%s", element->delimitor);
	}
//debug(1, "%d ", f_data_len); fflush(stdout);
// Inserting the data into the Node tag, and formatting the timestamp as well
	int i = sprintf(o_str, element->parse_str[0], data_str[0], f_data); 
	strcat(o_str, "\r\n");

	return i+2;
}

static void *ledIndicatorThread(void *sem) {
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

	unsigned long process_time, process_time_parse;
	int length;
	int success = 0;
	int read_string_length = 0;
	char buffer[STD_LINE_LENGTH];
	char outb[STD_LINE_LENGTH];
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	fl.l_pid = getpid();
	int runtime_counter = read_rt_count();

	log_t log = {"all", "", runtime_counter, 0, 0};

	while( *sp != NULL ) {
		programName = strsep(sp, "/");
	}
//	printf("[%s] 0\n", programName);

	int i;
	struct timespec spec, spec_parse, beginning, beginning_parse, final, final_parse;
//	struct sensor_element sensors[MAX_SENSOR_COUNT];
	pthread_t ledIndication_thread;
	sem_t sample_start_sem;

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	sem_init(&sample_start_sem, 0, 0);

	register_sig_handler();
//printf("[%s] 1\n", programName);
	int entryCount = 5;//get_sensor_entries ( sensors );
/*for( i=0; i < entryCount; i++) {
debug(1, "(%p)%s\n", sensors[i].fullpath, sensors[i].fullpath); fflush(stdout);
debug(1, "(%p)%s\n", sensors[i].mmapped_file.filename, sensors[i].mmapped_file.filename); fflush(stdout);
}
*/
//printf("[%s] 2\n", programName);
	
//	get_parser_entries( sensors, entryCount);


/*for( i=0; i < entryCount; i++) {
debug(1, "(%p)%s\n", sensors[i].fullpath, sensors[i].fullpath); fflush(stdout);
debug(1, "(%p)%s\n", sensors[i].mmapped_file.filename, sensors[i].mmapped_file.filename); fflush(stdout);
}
*/


//3 '\n' 0 1 "<LOG timestamp='%s'>%s</LOG>" "<period>%s</period>" "<battery>%s</battery>"
//3 '\n' 0 1 "<LOG timestamp='%s'>%s</LOG>" "<period>%s</period>" "<battery>%s</battery>"


//printf("[%s] 3\n", programName);

	get_sensor_entries(&sensors, entryCount);

	//for(i = 0; i < entryCount; i++) {
		
	//	pretty_print_parse_element( &sensors[i].parser[0] );
		//pretty_print_parse_element( &sensors[i].parser[1] );
	
//}
// At this point we know all sensors and have loaded their parsers
//printf("[%s] 4\n", programName);


	if(creat_log(&log) < 0) {
		return -1;
	}
//printf("[%s] 5\n", programName);fflush(stdout);
	tcp_init("/dev/ttyS3");
//printf("[%s] 6\n", programName);fflush(stdout);
	pthread_create(&ledIndication_thread,&thread_attr,ledIndicatorThread, (void *)&sample_start_sem);
	printf("[%s] 7 Starting Loop\n", programName);

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

		length = snprintf(outb, STD_LINE_LENGTH, "<dataset timestamp=\"%d.%lu\">\r\n", (int)beginning.tv_sec, beginning.tv_nsec/NSEC_PER_MSEC);
		//if(tcp_send_data(outb, length)<0) reboot(RB_AUTOBOOT);
		tcp_send_data(outb, length);
		if ( append_log(&log, outb, length) < 0 ) {
			fprintf(stderr, "[%s] WARNING Could not log data to SD card. Still continueuing\n", programName);
		}

		for( i=0; i < entryCount; i++) {
			//clock_gettime(CLOCK_REALTIME, &beginning_parse);
			//debug(1, "(%p) %s ", sensors[i].fullpath, sensors[i].fullpath); fflush(stdout);
			//debug(1, "(%p) %s ", sensors[i].mmapped_file.filename, sensors[i].mmapped_file.filename); fflush(stdout);
			//memset(buffer, 0, STD_LINE_LENGTH);
			read_string_length = mm_get_line(&sensors[i].mmapped_file, buffer);
			//printf("%d\n", read_string_length);fflush(stdout);

			while(read_string_length > 0){
			//while( (read_string_length = mm_get_line(&sensors[i].mmapped_file, buffer)) > 0) {


//			debug(1, "."); fflush(stdout);
				//printf("%d\n", read_string_length);fflush(stdout);
				length = parse_line( buffer, outb, &sensors[i].parser[0] ); // XML Parser
				//if(tcp_send_data(outb, length)<0) reboot(RB_AUTOBOOT);
				tcp_send_data(outb, length);
				//printf("%s\n", outb);

				length = parse_line( buffer, outb, &sensors[i].parser[1] ); // Raw Parser
				if ( append_log(&log, outb, length) < 0 ) {
					fprintf(stderr, "[%s] WARNING Could not log data to SD card. Still continueuing\n", programName);
				}
				//usleep(50000);
				read_string_length = mm_get_line(&sensors[i].mmapped_file, buffer);
			
			}
				/*clock_gettime(CLOCK_REALTIME, &final_parse);
				spec_parse = subtract_timespec(final_parse, beginning_parse);
				process_time_parse = spec_parse.tv_sec * USEC_PER_SEC + spec_parse.tv_nsec / NSEC_PER_USEC;
				printf("Process time [%s]: %lu \n", &sensors[i].name, process_time_parse); fflush(stdout);*/
//debug(1, "\n");
		}

		length = sprintf(outb, "</dataset>\r\n");
		//if(tcp_send_data(outb, length)<0) reboot(RB_AUTOBOOT);
		tcp_send_data(outb, length);
		if ( append_log(&log, outb, length) < 0 ) {
			fprintf(stderr, "[%s] WARNING Could not log data to SD card. Still continueuing\n", programName);
		}

		/* ****** END TIMING ****** */
		clock_gettime(CLOCK_REALTIME, &final);
		spec = subtract_timespec(final, beginning);
		process_time = spec.tv_sec * USEC_PER_SEC + spec.tv_nsec / NSEC_PER_USEC;

		if(process_time > SAMPLE_PERIOD_US) process_time = SAMPLE_PERIOD_US;
		else if(process_time < 0) process_time = 0;

		//printf("Process time [TOTAL]: %lu - %d\n", process_time, tcp_checkconnection()); 
		fflush(stdout);
		usleep(SAMPLE_PERIOD_US - process_time);
	}
	debug(1, "PROGRAM EXIT \n");

	tcp_end();

	return -1;
}

















