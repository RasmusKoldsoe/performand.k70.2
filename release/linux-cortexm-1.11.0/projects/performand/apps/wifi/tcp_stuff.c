#include <stdio.h>


#include "../Common/MemoryMapping/memory_map.h"
#include "gainspan/hardware/GS_HAL.h"
#include "gainspan/API/GS_API.h"
#include "tcp_stuff.h"

#define min(a, b) (a<b?a:b)

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
	uint16_t left = (uint16_t)len;

	while(left > 0) {
		uint16_t to_send = min(left, TCP_MAX_LEN);
		GS_API_SendTcpData(1, (uint8_t *)(data), to_send);

/*		if(GS_API_SendTcpData(1, (uint8_t *)(data), to_send) == 0){
			fprintf(stderr, "[TCP_STUFF] ERROR: TCP/IP Transmission failed\n");
			return -1;
		}*/
		data += to_send;
		left -= to_send;
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

