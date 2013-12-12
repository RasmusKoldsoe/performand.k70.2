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

	debug(2, "Received: %s\n", receiveMessage);

	pch = strsep(bp,","); //messageID	
	if(pch != NULL) {
		pch = strsep(bp,",");
		if(pch != NULL) {
			gpsRMCData->timeHour = (pch[0]-'0')*10 + pch[1]-'0';
			gpsRMCData->timeMinute = (pch[2]-'0')*10 + pch[3]-'0';
			gpsRMCData->timeSecond = (pch[4]-'0')*10 + pch[5]-'0';
			gpsRMCData->timeMillisecond = (pch[7]-'0')*100 + (pch[8]-'0')*10 + (pch[9]-'0');
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			if(pch[0] == 'A') {
				gpsRMCData->positionFix = STD_GPS;
			}
			else {
				gpsRMCData->positionFix = NO_FIX_INVALID;
				return 0;
			}
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			gpsRMCData->latitudeDegrees = (pch[0]-'0')*10 + pch[1]-'0';
			gpsRMCData->latitudeMinutes = (pch[2]-'0')*10 + pch[3]-'0';
			gpsRMCData->latitudeSeconds = (pch[5]-'0')*1000 + (pch[6]-'0')*100 + (pch[7]-'0')*10 + (pch[8]-'0');
			gpsRMCData->latitudeSeconds = 60.0 * gpsRMCData->latitudeSeconds/10000;
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			gpsRMCData->latitudeHemisphere = pch[0];
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			gpsRMCData->longitudeDegrees = (pch[0]-'0')*100 + (pch[1]-'0')*10 + pch[2]-'0';
			gpsRMCData->longitudeMinutes = (pch[3]-'0')*10 + pch[4]-'0';
			gpsRMCData->longitudeSeconds =  ((pch[6]-'0')*1000) +
				((pch[7]-'0')*100) +
				((pch[8]-'0')*10) +
				((pch[9]-'0'));
			gpsRMCData->longitudeSeconds = 60.0 * gpsRMCData->longitudeSeconds/10000;
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			gpsRMCData->longitudeHemisphere = pch[0];
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			sprintf(pchBuf, "%s", pch);                
			gpsRMCData->speedOverGround = (float)atof(pchBuf)*0.514;
		}

		pch = strsep(bp,",");
		if(pch != NULL) {
			sprintf(pchBuf, "%s", pch);
			gpsRMCData->courseOverGround = (float)atof(pchBuf);

			pch = strsep(bp,",");
 			if(pch != NULL) {
				gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
				gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
				gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
			}
		}
	}
	else {
		fprintf(stderr, "Error: GPS - Parsing ... %s\n",__func__);
		return -1;
	}

	return 1;
}
