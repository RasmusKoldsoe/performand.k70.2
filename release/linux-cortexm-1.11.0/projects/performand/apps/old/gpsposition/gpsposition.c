#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>

#define BUFFER_SIZE 1024

enum MESSAGE_ID {
   GPGSV,
   GPRMC,
   GPVTG,
   GPGGA,
   GPGSA,
   GPGLL,
   GPGBS,
   GENERIC,
   ERROR
};

static int decodeMessage(unsigned char* message)
{
        char **bp = &(message);
        unsigned int messageType = ERROR;
        int i = 0;
        unsigned int checksum = 0;
        unsigned char recCsStr[3];
        unsigned char* foundChar = NULL;
        int offset = 0;
        char* pch;
        char csStr[3];
        foundChar = (unsigned char*) strchr((char*)message, '*');
        if(foundChar != NULL)
        {
                offset = foundChar - message + 1;
                recCsStr[0] = message[offset];
                recCsStr[1] = message[offset+1];
                recCsStr[2] = '\0';
        }

        for(i=1; i<=offset; i++)
        {
                if(message[i] == '*')
                {
                        break;
                }
                checksum = checksum ^ message[i];
        }
        //printf("Computed checksum is: %X\n", checksum);
        sprintf(csStr,"%02X", checksum);
        //printk("Message: %s vs Checksum: %s\n", message, csStr);
        if(strcmp(csStr, (char*)recCsStr) == 0)
        {
          //printk("Checksum ok\n");
          pch = strsep (bp,",");
          if (pch != NULL)
          {
                if(strcmp(pch, "$GPRMC") == 0)
                {
                        messageType = GPRMC;
                }
                else if(strcmp(pch, "$GPGSV") == 0)
                {
                        messageType = GPGSV;
                }
                else if(strcmp(pch, "$GPVTG") == 0)
                {
                        messageType = GPVTG;
                }
                else if(strcmp(pch, "$GPGGA") == 0)
                {
                        messageType = GPGGA;
                }
                else if(strcmp(pch, "$GPGLL") == 0)
                {
                        messageType = GPGLL;
                }
                else if(strcmp(pch, "$GPGSA") == 0)
                {
                        messageType = GPGSA;
                }
                else if(strcmp(pch, "$GPGBS") == 0)
                  {
                    messageType = GPGBS;
                  }
                else
                {
                        messageType = GENERIC;
                }
          }
        }
        return messageType;
}

void read_file(const char* fullpath, char* data)
{
	FILE* file = fopen(fullpath, "r");
	if (file != NULL) {
		size_t size = fread(data, sizeof(char), BUFFER_SIZE, file);
		if (size == 0) {
	    		fputs("Error reading file", stderr);
		} 
		else {
	   	 	data[++size] = '\0'; // Be safe.
		}
		fclose(file);
	}
}


int main()
{
	struct timeval now;
	unsigned char data[512];	

	while(1) {
		//gettimeofday(&now, NULL);
		//printf("[%6d.%6d]: ", now.tv_sec,now.tv_usec);
	
		read_file("/dev/ttyS3", data);
		printf("%x \n",decodeMessage(data));
		
		//printf("%s EOT\n",data);
 	}        
	return 0;
}
