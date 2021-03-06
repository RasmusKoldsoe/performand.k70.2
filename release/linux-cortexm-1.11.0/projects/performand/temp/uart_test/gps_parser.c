#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#include "gps_parser.h"

int parseGPRMC(char *receiveMessage, RMC_DATA* gpsRMCData)
{
        char **bp = &(receiveMessage);
        char* pch;
        char pchBuf[20];
        float speedKnots;
      //  printf("Received: %s\n", receiveMessage);
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
                        gpsRMCData->timeMillisecond = (pch[7]-'0')*100 + (pch[8]-'0')*10;
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
                          gpsRMCData->latitudeSeconds = (pch[5]-'0')*6;
                          if(((pch[6]-'0')*6) > 54)
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
                          }
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
                        gpsRMCData->longitudeDegrees = (pch[0]-'0')*100 + (pch[1]-'0')*10 + pch[2]-'0';
                        gpsRMCData->longitudeMinutes = (pch[3]-'0')*10 + pch[4]-'0';
                        gpsRMCData->longitudeSeconds = (pch[6]-'0')*6;
                        if(((pch[7]-'0')*6) > 54)
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
                        }
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
                        //sprintf(pchBuf, "%s", pch);
                        //speedKnots = strToIFloat(pchBuf,3);
                        gpsRMCData->speedOverGround = 0;//0.514*speedKnots;
                }
                //pch = strtok(NULL, ",");
                pch = strsep(bp,",");
                if(pch != NULL)
                {
                        //kstrtol(pch, 10, &tempVal);
                        //printk("PCH: %s\n", pch);
                        sprintf(pchBuf, "%s", pch);//make sure the \0 is there upon searching
                        //printk("pchBuf: %s", pch);
                        /*if(0)//then this is the date format
                          {
                            gpsRMCData->courseOverGround = strToIFloat("0.0");
                            gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
                            gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
                            gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
                          }
                        else*/
                          {
                            gpsRMCData->courseOverGround = 0.0; //strToIFloat(pchBuf,3);
                            //pch = strtok(NULL, ",");
                            pch = strsep(bp,",");
                            if(pch != NULL)
                            {
                                    gpsRMCData->dateDay = (pch[0]-'0')*10 + pch[1]-'0';
                                    gpsRMCData->dateMonth = (pch[2]-'0')*10 + pch[3]-'0';
                                    gpsRMCData->dateYear = (pch[4]-'0')*10 + pch[5]-'0';
                            }
                          }
                }

        }
        return 1;
}
