#include <stdio.h>

#include "../Common/MemoryMapping/memory_map.h"

#include "gainspan/hardware/GS_HAL.h"
#include "gainspan/API/GS_API.h"
#include "tcp_stuff.h"


int tcp_init(char *port) // "/dev/ttyS3"
{
#if __arm__
	GS_HAL_uart_set_comm_port(port);
#elif __i386__
	printf("SETTING UP WIFI ON PORT %s\n", port);
#else
#error "Target arcitechture not recognized"
#endif
	return 0;
}


int tcp_send_data(char *data, int len)
{
#if __arm__
	int memptr;
	unsigned int left;

	if(len > TCP_MAX_LEN) {
		left = len;
		for(memptr=0; memptr < len; memptr += 1400) {	
			if(!GS_API_SendTcpData(1, (uint8_t *)(data+memptr), (left>TCP_MAX_LEN) ? TCP_MAX_LEN : left))
				//printf("ERROR: TCP/IP Transmission\n");
			left -= TCP_MAX_LEN;
		}
	} 
	else {
GS_API_SendTcpData(1, (uint8_t *)data, len);
//		if(!GS_API_SendTcpData(1, data, len))
			//printf("ERROR: TCP/IP Transmission\n");	
	}
#elif __i386__
	printf("%s", data);
#else
#error "Target arcitechture not recognized"
#endif
	return len;
}

void tcp_end()
{


}

