#ifndef _GPS_UTILS_H_
#define _GPS_UTILS_H_

enum POSITION_FIX_STATUS {
   NO_FIX_INVALID = 0,
   STD_GPS = 1,
   DIFF_GPS = 2,
   ESTIMATED_FIX = 6
};

enum POSITION_FIX_MODE{
  NA = 1,
  TWO_D = 2,
  THREE_D = 3
};

enum POSITION_FIX_SMODE{
  MANUAL = 1,
  AUTOMATIC = 2
};

enum REQUESTED_DATA{
  POSITION = 1,
  SPEED = 2,
  HEADING = 3,
  TIME = 4,
  ALTITUDE = 5,
  ALL = 6
};

typedef struct RMC_DATA{
    unsigned long timeHour;
    unsigned long timeMinute;
    unsigned long timeSecond;
    unsigned long timeMillisecond;
    enum POSITION_FIX_STATUS positionFix;
    unsigned long latitudeDegrees;
    unsigned long latitudeMinutes;
    float latitudeSeconds;
    unsigned char latitudeHemisphere;
    unsigned long longitudeDegrees;
    unsigned long longitudeMinutes;
    float longitudeSeconds;
    unsigned char longitudeHemisphere;
    float speedOverGround;//in m/s
    float courseOverGround;//in degrees
    long dateDay;
    long dateMonth;
    long dateYear;
}RMC_DATA;

int parseGPRMC(char *receiveMessage, RMC_DATA* gpsRMCData);

#endif
