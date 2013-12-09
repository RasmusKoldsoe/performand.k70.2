#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gps_utils.h"

int main(void) {
	FILE * fp;
	FILE *file;
	char *line = NULL;
	char *strintoparse = NULL;
	size_t len = 0;
	ssize_t read;
	int line_counter = 0;
	RMC_DATA gps_data;

	fp = fopen("datalog-gps8.0.txt", "r");
	if (fp == NULL) {
		printf("datalog.txt could not open\n");
		exit(EXIT_FAILURE);
	}


	file = fopen("file.txt","a+"); /* apend file (add text to a file or create a file if it does not exist.*/
	if (file == NULL) {
		printf("file.txt could not open\n");
		exit(EXIT_FAILURE);
	}

	printf("Start processing\n");
	while ((read = getline(&line, &len, fp)) != -1) {
	   	line_counter+=1;
		strintoparse = (char*)malloc(read+1);
		memcpy(strintoparse, line, read+1);
		if(parseGPRMC(line, &gps_data)) {
		/*printf("%s %2d %2d %f - %2d %2d %f", line,gps_data.latitudeDegrees,
						gps_data.latitudeMinutes,
						gps_data.latitudeSeconds,
						gps_data.longitudeDegrees,
						gps_data.longitudeMinutes,
						gps_data.longitudeSeconds);*/
		fprintf(file,"%02lu %02lu %02.6f / %02lu %02lu %02.6f / %s", gps_data.latitudeDegrees,
						gps_data.latitudeMinutes,
						gps_data.latitudeSeconds,
						gps_data.longitudeDegrees,
						gps_data.longitudeMinutes,
						gps_data.longitudeSeconds, strintoparse);
		}
		free(strintoparse);
	}
	printf("Finished reading %d lines.\n", line_counter);
	if (line) {
	   free(line);
	}
	fclose(file); /*done!*/
	fclose(fp);
	exit(EXIT_SUCCESS);
}
