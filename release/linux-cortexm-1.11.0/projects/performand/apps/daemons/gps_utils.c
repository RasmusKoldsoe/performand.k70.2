#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "../Common/common_tools.h"
#include "gps_utils.h"

int parseGPRMC(char *receiveMessage, RMC_DATA* gpsRMCData)
{
	char **bp = &(receiveMessage);
	char* pch;
	char pchBuf[30];
	char backupMessage[256];
	memcpy(backupMessage, receiveMessage, strlen(receiveMessage));
	backupMessage[strlen(receiveMessage)] = '\0';


	// --- TYPE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Type Token\n");
		return -1;
	}
	debug(2, "Type:       %s\n", pch);
	if(strcmp(pch, "$GPRMC") != 0) {
		fprintf(stderr, "GPS PARSER: Type Token %s not supported - %s", pch, backupMessage);
		return -11;
	}

	// --- TIME TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Time Token\n");
		return -2;
	}
	gpsRMCData->timeHour = (pch[0]-'0')*10 + pch[1]-'0';
	gpsRMCData->timeMinute = (pch[2]-'0')*10 + pch[3]-'0';
	gpsRMCData->timeSecond = (pch[4]-'0')*10 + pch[5]-'0';
	gpsRMCData->timeMillisecond = (pch[7]-'0')*100 + (pch[8]-'0')*10 + (pch[9]-'0');
	debug(2, "Time:       %02d:%02d:%02d.%03d\n", 
			gpsRMCData->timeHour, 
			gpsRMCData->timeMinute, 
			gpsRMCData->timeSecond, 
			gpsRMCData->timeMillisecond);

	// --- STATUS TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Status Token\n");
		return -3;
	}
	if(pch[0] == 'A') {
		gpsRMCData->positionFix = STD_GPS;
	}
	else if(pch[0] == 'V'){
		gpsRMCData->positionFix = NO_FIX_INVALID;
		//return 0;
	}
	else {
		fprintf(stderr, "GPS PARSER: Unknown Status Token %c - %s", pch[0], backupMessage);
		return -33;
	}
	debug(2, "Status:     %d (%s)\n", gpsRMCData->positionFix, gpsRMCData->positionFix=='A'?"ACTIVE":"VOID");

	// --- LATITUDE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Latitude Token\n");
		return -4;
	}
	if(strlen(pch) > 0) {
		gpsRMCData->latitudeDegrees = (pch[0]-'0')*10 + pch[1]-'0';
		gpsRMCData->latitudeMinutes = (pch[2]-'0')*10 + pch[3]-'0';
		gpsRMCData->latitudeSeconds = ((pch[5]-'0')*1000) + ((pch[6]-'0')*100) + ((pch[7]-'0')*10) + (pch[8]-'0');
		gpsRMCData->latitudeSeconds = 60.0 * gpsRMCData->latitudeSeconds/10000;
	} else {
		gpsRMCData->latitudeDegrees = 0;
		gpsRMCData->latitudeMinutes = 0;
		gpsRMCData->latitudeSeconds = 0;
	}
	debug(2, "Latitude:   %d deg %d.%d\n", gpsRMCData->latitudeDegrees, gpsRMCData->latitudeMinutes, gpsRMCData->latitudeSeconds);

	// --- LATITUDE HEMISPHERE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Latitude Hemisphere Token\n");
		return -5;
	}
	if(strlen(pch) > 0) {
		gpsRMCData->latitudeHemisphere = pch[0];
	} else {
		gpsRMCData->latitudeHemisphere = 'N';
	}
	debug(2, "Hemisphere: %c\n", gpsRMCData->latitudeHemisphere);

	// --- LATITUDE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Longitude Token\n");
		return -6;
	}
	if(strlen(pch) > 0) {
		gpsRMCData->longitudeDegrees = (pch[0]-'0')*100 + (pch[1]-'0')*10 + pch[2]-'0';
		gpsRMCData->longitudeMinutes = (pch[3]-'0')*10 + pch[4]-'0';
		gpsRMCData->longitudeSeconds = ((pch[6]-'0')*1000) + ((pch[7]-'0')*100) + ((pch[8]-'0')*10) + (pch[9]-'0');
		gpsRMCData->longitudeSeconds = 60.0 * gpsRMCData->longitudeSeconds/10000;
	} else {
		gpsRMCData->longitudeDegrees = 0;
		gpsRMCData->longitudeMinutes = 0;
		gpsRMCData->longitudeSeconds = 0;
	}
	debug(2, "Longitude:  %d deg %d.%d\n", gpsRMCData->longitudeDegrees, gpsRMCData->longitudeMinutes, gpsRMCData->longitudeSeconds);

	// --- LONGITUDE HEMISPHERE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Longitude Hemisphere Token\n");
		return -7;
	}
	if(strlen(pch) > 0) {
		gpsRMCData->longitudeHemisphere = pch[0];
	} else {
		gpsRMCData->longitudeHemisphere = 'E';
	}
	debug(2, "Hemisphere: %c\n", gpsRMCData->longitudeHemisphere);

	// --- SPEED OVER GROUND TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Speed Over Ground Token\n");
		return -8;
	}
	sprintf(pchBuf, "%s", pch);                
	gpsRMCData->speedOverGround = (float)atof(pchBuf)*0.514;
	debug(2, "SOG:        %0.3f\n", gpsRMCData->speedOverGround);

	// --- COURSE OVER GROUND TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Course Over Ground Token\n");
		return -9;
	}
	sprintf(pchBuf, "%s", pch);
	gpsRMCData->courseOverGround = (float)atof(pchBuf);
	debug(2, "COG:        %0.3f\n", gpsRMCData->courseOverGround);

	// --- DATE TOKEN ---
	pch = strsep(bp,",");
	if(pch == NULL) {
		fprintf(stderr, "GPS PARSER: Missing Date Token\n");
		return -10;
	}
	gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
	gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
	gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
	debug(2, "Date:       %d/%d %d\n", gpsRMCData->dateDay, gpsRMCData->dateMonth, gpsRMCData->dateYear);

	return 0;
}
