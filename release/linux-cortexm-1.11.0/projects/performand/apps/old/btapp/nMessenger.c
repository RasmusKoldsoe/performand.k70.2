#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char *argv[])
{
  if( argc < 4 ){
    printf("Usage: %s -r/-w -p port\n \
    -r    Read from port\n \
    -w    Write to port\n \
    port  Port name\n \
    Example: %s -w -p /dev/ttyUSB0\n", argv[0], argv[0]);
    exit(0);
  }

  char PORT[12];
  int r=0, w=0;
  int i;

  for(i=1; i<argc; i++) {
    char *param;
    param = argv[i];
    if(param[0] != '-') {
      printf("Not a valid input parameter: %s\n", argv[i]);
      exit(0);
    }

    switch(param[1]) {
    case 'r':
      r = 1;
      w = 0;
      break;
    case 'w':
      r = 0;
      w = 1;
      break;
    case 'p':
      i++;
      sprintf(PORT, "%s", argv[i]);
      break;
    default:
      printf("Input parameter not recognised: %s\n", argv[i]);
      exit(0);
    }
  }

  int fd;
  if( r && !w ) { // Read
    printf("Port %s selected in read mode\n", PORT);

    fd = open(PORT, O_RDWR | O_NOCTTY | O_NDELAY );
    if ( fd < 0 ) {
      printf("ERROR Open serial port %s\n", PORT);
      exit(0);
    }

    char message[256];
    memset(message, 0, 256);
    ssize_t n;

    n = read( fd, &message, 1);
    if(n > 0) {
    	printf("Message receive (%d bytes)d: %s\n", n, message);
    }
    else {
    	printf("Error receiving message. Read returned %d\n", n);
    }
    close(fd);
  }
  else if( !r && w ) { // Write
    printf("Port %s selected in write mode\n", PORT);

    fd = open(PORT, O_RDWR | O_NOCTTY | O_NDELAY );
    if ( fd < 0 ) {
      printf("ERROR Open serial port %s\n", PORT);
      exit(0);
    }

    char message[] = "hello";
    size_t length = 6;
    ssize_t n;
    n = write( fd, &message, length);
    printf("Message \"%s\" sent (%d bytes)\n", message, (int)n);
    close(fd);
  }
  else {  // Not recognised
    printf("ERROR: No read or write selected\n");
  }

  return 0;
}
