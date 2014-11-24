#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include "../Common/GPIO/gpio_api.h"
#include "gainspan/API/GS_API.h"
#include "GS_Initialize_wifi.h"
#include "tcp_stuff.h"


int main(int argc, char **argv)
{
/*	const char delim = '/';
	char **sp = argv;

	while( *sp != NULL ) {
		programName = strsep(sp, &delim);
	}
	if(argc >= 2) {
		PROVISION_SSID = &argv[1];
	} else {
		printf("USAGE: %s SSID\nAllways put SSID!\n", programName);
		return -1;
	}
*/
	initWifi();

	return 0;
}



