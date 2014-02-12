#include <stdio.h>

#include "../Common/MemoryMapping/memory_map.h"

#include "gainspan/hardware/GS_HAL.h"
#include "gainspan/API/GS_API.h"
//#include "gainspan/AT/AtCmdLib.h"
#include "tcp_stuff.h"


int tcp_init(char *port) // "/dev/ttyS3"
{
	GS_HAL_uart_set_comm_port(port);
	return 0;
}


int tcp_send_data(char *data, int len)
{
	int memptr;
	unsigned int left;

	if(len > TCP_MAX_LEN) {
		left = len;
		for(memptr=0; memptr < len; memptr += 1400) {	
			GS_API_SendTcpData(1, data+memptr, (left>TCP_MAX_LEN) ? TCP_MAX_LEN : left);
			left -= TCP_MAX_LEN;
		}
	} 
	else {
		GS_API_SendTcpData(1, data, len);	
	}
	return len;
}

void tcp_end()
{


}

