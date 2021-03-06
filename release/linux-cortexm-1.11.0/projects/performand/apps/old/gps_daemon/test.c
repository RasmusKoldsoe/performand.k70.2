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

#define FILE_LENGTH 	0x400

void* file_memory=NULL;
int fileptr=0;

struct RMC_DATA gprmc;

static unsigned char disableAllMessages[] = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableRMC[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*XX\r\n";
static unsigned char enableTenHz[] = "$PMTK220,100*XX\r\n"; 

static int writeMessageToPort(unsigned char* message, unsigned int messageLength, int tty)
{
        unsigned int checksum=0;
        unsigned int i;
        unsigned char currentChar = '0';
        unsigned long flags;
        unsigned int ier;
        int locked = 1;
        char csStr[3];

        for(i=1; i<messageLength-1; i++)
        {
                if(message[i] == '*')
                {
                        break;
                }
                else
                {
                        checksum = checksum ^ message[i];
                }
        }
        sprintf(csStr, "%X", checksum);
        message[messageLength-5] = csStr[0];
        message[messageLength-4] = csStr[1];


	//printf("Message: %s Length: %d",message, messageLength);

	if ((i=write(tty, message, messageLength-1)) < messageLength-1)	{
		printf("ERROR writing to port.\n\r");
		return -1;
	}
	
	return 0;

}



void print_byte_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%02X ", buff[i] & 0xff);
	}
	printf("\n");
}

void print_char_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%c", buff[i] & 0xff);
	}
	printf("\n");
}

void preparemappedMem() {
	int fd;
	int count=0;
	int fileptr=0;

	/* Prepare a file large enough to hold an unsigned integer.*/
	fd = open ("/gps", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, FILE_LENGTH+1, SEEK_SET);

	write (fd, "", 1);
	lseek (fd, 0, SEEK_SET);
	/* Create the memory mapping. */
	file_memory = mmap (0, FILE_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);
	close (fd);
}

void write_mm_file(char *content) {
	//Check if the current content will overflows the file.
	//if(fileptr+strlen(content)+1 > FILE_LENGTH)
	//	fileptr=0;
	//memset(file_memory,'\0',sizeof(FILE_LENGTH));
	fopen()	
	sprintf((char*) file_memory, "%s",content);
	close()
	//fileptr+=strlen(content)+1;
	//if(fileptr >= FILE_LENGTH)
	//	fileptr=0;		
}


void append_mm_file(char *content) {
	//Check if the current content will overflows the file.
	if(fileptr+strlen(content)+1 > FILE_LENGTH)
		fileptr=0;

	sprintf((char*) file_memory+fileptr, "%s",content);
	fileptr+=strlen(content)+1;
	if(fileptr >= FILE_LENGTH)
		fileptr=0;		
}

void toXMLfile(RMC_DATA *gpsRMCData) {
	memset(file_memory,0,FILE_LENGTH);	
	sprintf((char*) file_memory,"<gps>\n\
<gmttime>\n\
<hours>%02d</hours>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<milliseconds>%03d</milliseconds>\n\
</gmttime>\n\
<date>\n\
<dd>%02d</dd>\n\
<mm>%02d</mm>\n\
<yyyy>20%02d</yyyy>\n\
</date>\n\
<latitude>\n\
<degrees>%02d</degrees>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<milliseconds>%03d</milliseconds>\n\
<hemisphere>%c</hemisphere>\n\
</latitude>\n\
<longitude>\n\
<degrees>%03d</degrees>\n\
<minutes>%02d</minutes>\n\
<seconds>%02d</seconds>\n\
<hemisphere>%c</hemisphere>\n\
</longitude>\n\
<speedMS>%s</speedMS>\n\
<cog>%s</cog>\n\
</gps>\n",
	gpsRMCData->timeHour, gpsRMCData->timeMinute, gpsRMCData->timeSecond, gpsRMCData->timeMillisecond,
	gpsRMCData->dateDay, gpsRMCData->dateMonth,gpsRMCData->dateYear, 
	gpsRMCData->latitudeDegrees, gpsRMCData->latitudeMinutes, gpsRMCData->latitudeSeconds, gpsRMCData->latitudeHemisphere,
	gpsRMCData->longitudeDegrees, gpsRMCData->longitudeMinutes, gpsRMCData->longitudeSeconds, gpsRMCData->longitudeHemisphere, gpsRMCData->speedOverGround, gpsRMCData->courseOverGround);

	//printf("%s",content);
	//write_mm_file(content);		
}


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
 
int main(){

	struct termios tio;
	int tty_fd;
	fd_set rdset;
	unsigned char c=0;
	char buffer[255]; 
	char *bufptr; 
	int nbytes; 
	int fd;
	void* file_memory;
	int pos=0;

	//Prepare the mapped Memory file
	preparemappedMem();	

	RMC_DATA gpsRMCData;

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST; //0;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);
	tio.c_cc[VMIN]=0;
	tio.c_cc[VTIME]=0;

	tty_fd=open("/dev/ttyS1", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio,B9600);
	cfsetispeed(&tio,B9600);
	tcsetattr(tty_fd,TCSANOW,&tio);

	writeMessageToPort(enableTenHz, sizeof(enableTenHz), tty_fd);
	//readResponse();
	writeMessageToPort(disableAllMessages, sizeof(disableAllMessages), tty_fd);
	writeMessageToPort(enableRMC, sizeof(enableRMC), tty_fd);

	bufptr = buffer;
	while (1)
	{

		/* read characters into our string buffer until we get a CR or NL */
		nbytes = 0;
		
		while((nbytes=read(tty_fd,bufptr,buffer+sizeof(buffer)-bufptr-1))>0)
		{
			bufptr += nbytes;
			if (bufptr[-2] == '\r' && bufptr[-1] == '\n') {	
				*bufptr = '\0';
				bufptr = buffer;
				//printf("*** %s ***",buffer);				
				parseGPRMC(buffer, &gprmc);
				//printf("%d\r\n",gprmc.timeSecond);
				toXMLfile(&gprmc);
				//memset(&buffer,'\0',sizeof(buffer));	
			}
		}
	}

  return(0);
}
