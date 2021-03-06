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

#include "gps_utils.h"

int parseGPRMC(char *receiveMessage, RMC_DATA* gpsRMCData)
{
        char **bp = &(receiveMessage);
        char* pch;
        char pchBuf[30];
        float speedKnots;
        //printf("Received: %s\n", receiveMessage);
        //pch = strtok((char*)receiveMessage,"$,");//messageID
        pch = strsep(bp,",");//messageID	
        if(pch != NULL)
        {
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        gpsRMCData->timeHour = (pch[0]-'0')*10 + pch[1]-'0';
                        gpsRMCData->timeMinute = (pch[2]-'0')*10 + pch[3]-'0';
                        gpsRMCData->timeSecond = (pch[4]-'0')*10 + pch[5]-'0';
                        gpsRMCData->timeMillisecond = (pch[7]-'0')*100 + (pch[8]-'0')*10 + (pch[9]-'0');
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        if(pch[0] == 'A')
                          {
                            gpsRMCData->positionFix = STD_GPS;
                          }
                        else
                          {
                            gpsRMCData->positionFix = NO_FIX_INVALID;
			    return 0;
                          }
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                  {
                          gpsRMCData->latitudeDegrees = (pch[0]-'0')*10 + pch[1]-'0';
                          gpsRMCData->latitudeMinutes = (pch[2]-'0')*10 + pch[3]-'0';
                          //gpsRMCData->latitudeSeconds = (pch[5]-'0')*6;
			
			if((pch[5]-'0') < 0) printf("%d < 0\n", pch[5]-'0');
			if((pch[6]-'0') < 0) printf("%d < 0\n", pch[5]-'0');
			if((pch[7]-'0') < 0) printf("%d < 0\n", pch[5]-'0');
			if((pch[8]-'0') < 0) printf("%d < 0\n", pch[5]-'0');

			gpsRMCData->latitudeSeconds =  ((pch[5]-'0')*1000) +
							((pch[6]-'0')*100) +
							((pch[7]-'0')*10) +
							((pch[8]-'0'));


			//printf("Seconds: %f\n",gpsRMCData->longitudeSeconds);
			gpsRMCData->latitudeSeconds = 60.0 * gpsRMCData->latitudeSeconds/10000;

                        /*  if(((pch[6]-'0')*6) > 54)
                            {
                              gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 6;
                            }
                          else if(((pch[6]-'0')*6) > 44)
                          {
                            gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 5;
                          }
                          else if(((pch[6]-'0')*6) > 34)
                          {
                            gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 4;
                          }
                          else if(((pch[6]-'0')*6) > 24)
                          {
                            gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 3;
                          }
                          else if(((pch[6]-'0')*6) > 14)
                          {
                            gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 2;
                          }
                          else if(((pch[6]-'0')*6) > 4)
                          {
                            gpsRMCData->latitudeSeconds = gpsRMCData->latitudeSeconds + 1;
                          } */
                  }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        gpsRMCData->latitudeHemisphere = pch[0];
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
			//$GPRMC,133945.100,A,5445.8839,N,00919.1162,E,1.32,60.99,101113,,,A*5C
			//printf("%s\n",pch);

                        gpsRMCData->longitudeDegrees = (pch[0]-'0')*100 + (pch[1]-'0')*10 + pch[2]-'0';
                        gpsRMCData->longitudeMinutes = (pch[3]-'0')*10 + pch[4]-'0';
                        gpsRMCData->longitudeSeconds =  ((pch[6]-'0')*1000) +
							((pch[7]-'0')*100) +
							((pch[8]-'0')*10) +
							((pch[9]-'0'));
			//printf("Seconds: %f\n",gpsRMCData->longitudeSeconds);
			gpsRMCData->longitudeSeconds = 60.0 * gpsRMCData->longitudeSeconds/10000;
			//printf("Converted: %f\n",gpsRMCData->longitudeSeconds);
			
                       /* if(((pch[7]-'0')*6) > 54)
                          {
                            gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 6;
                          }
                        else if(((pch[7]-'0')*6) > 44)
                        {
                          gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 5;
                        }
                        else if(((pch[7]-'0')*6) > 34)
                        {
                          gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 4;
                        }
                        else if(((pch[7]-'0')*6) > 24)
                        {
                          gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 3;
                        }
                        else if(((pch[7]-'0')*6) > 14)
                        {
                          gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 2;
                        }
                        else if(((pch[7]-'0')*6) > 4)
                        {
                          gpsRMCData->longitudeSeconds = gpsRMCData->longitudeSeconds + 1;
                        } */
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        gpsRMCData->longitudeHemisphere = pch[0];
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        sprintf(pchBuf, "%s", pch);                
                        gpsRMCData->speedOverGround = (float)atof(pchBuf)*0.514;//0.514*speedKnots;
			
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        //kstrtol(pch, 10, &tempVal);
                        //printk("PCH: %s\n", pch);
                        sprintf(pchBuf, "%s", pch);//make sure the \0 is there upon searching
    
                        /*if(0)//then this is the date format
                          {
                            gpsRMCData->courseOverGround = strToIFloat("0.0");
                            gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
                            gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
                            gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
                          }
                        else*/
                          {
                           // gpsRMCData->courseOverGround = atof(pchBuf); //strToIFloat(pchBuf,3);
                            //pch = strtok(NULL, ",");
			    gpsRMCData->courseOverGround = (float)atof(pchBuf);
                            pch = strsep(bp,",");
                            if(pch != NULL)
                            {
                                    gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
                                    gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
                                    gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
                            }
                          }
                }

        } else	{
		printf("Error: Parsing ... %s\n",__func__);
		return -1;
	}
	
        return 1;
}
