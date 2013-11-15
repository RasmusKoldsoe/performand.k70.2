#include <stdio.h>
#include <stdlib.h>

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

	fp = fopen("datalog-gps8.1.txt", "r");
	if (fp == NULL)
	   exit(EXIT_FAILURE);


	file = fopen("file.txt","a+"); /* apend file (add text to
	a file or create a file if it does not exist.*/



	while ((read = getline(&line, &len, fp)) != -1) {
	   	line_counter+=1;
		strncpy(&strintoparse, &line, strlen(strintoparse));
		if(parseGPRMC(&strintoparse, &gps_data)) {
		printf("%s %2d %2d %f - %2d %2d %f\n", line,gps_data.latitudeDegrees,
						gps_data.latitudeMinutes,
						gps_data.latitudeSeconds,
						gps_data.longitudeDegrees,
						gps_data.longitudeMinutes,
						gps_data.longitudeSeconds);
		fprintf(file,"%2d %2d %f - %2d %2d %f\n", gps_data.latitudeDegrees,
						gps_data.latitudeMinutes,
						gps_data.latitudeSeconds,
						gps_data.longitudeDegrees,
						gps_data.longitudeMinutes,
						gps_data.longitudeSeconds);
		}
	}
	printf("Finished reading %d lines.\n", line_counter);
	if (line)
	   free(line);
	fclose(file); /*done!*/ 
	exit(EXIT_SUCCESS);
}
