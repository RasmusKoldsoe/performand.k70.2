/*
 * serialLogic.c
 *
 *  Created on: Sep 19, 2013
 *      Author: rasmus
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include "serialLogic.h"

#include "../ble_device.h"
#include "../Queue/Queue.h"
#include "../COM_Parser/COM_Parser.h"
#include "../NetworkStat/NetworkStatistics.h"
#include "../HCI_Parser/HCI_Parser.h"
#include "../APP.h"

void print_b_array(char *buff, int length, int offset)
{
#if (defined VERBOSITY) && (VERBOSITY >= 3)
	int i;
	for(i=offset; i<length; i++) {
		printf("%02X ", buff[i] & 0xff);
	}
#endif
#if (defined VERBOSITY) && (VERBOSITY >= 2)
	printf("\n");
#endif
}

int open_serial(char *port, int oflags)
{
	struct termios tio;
	fd_set rdset;
	unsigned char c = 0;

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag &= CRTSCTS | CS8 | CLOCAL | CREAD; 
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 10;

	int fd;
	fd = open(port, oflags);
	if( fd < 0 ){
		fprintf(stderr, "ERROR: Open port %s: %s\n", port, strerror(errno));
		return fd;
	}

	cfsetospeed(&tio,B115200);
	cfsetispeed(&tio,B115200);
	tcsetattr(fd,TCSANOW,&tio);

	return fd;
}

void close_serial(int fd)
{
	close(fd);
}

void *read_serial(void *_bleCentral)
{
	int n, offset=0;
	unsigned int next, count=0; 
	char buff[STD_BUF_SIZE]; memset(buff, 0, STD_BUF_SIZE);
	BLE_Central_t *bleCentral = (BLE_Central_t *)_bleCentral;
	datagram_t datagram;
	enum parserState_t parserState = package_type_token;
	enum parserState_t previousParserState = package_type_token;
	debug(1, "Read function started\n");

	while(bleCentral->_run) {
		n = read(bleCentral->fd, buff + count, 1);

		if ( n < 0 ) {
			fprintf(stderr, "ERROR: Read message header. Read return %d %s\n", n, strerror(errno));
			break;
		}
		else if( n == 0 ) {
			usleep(STD_WAIT_TIME);
			continue;
		}
		
		count += n;
		updateRxStat(0, n);

		previousParserState = parserState;
		parserState = COM_parse_data(&datagram, buff, count, &offset, parserState);

		if( parserState == package_type_token && previousParserState != parserState ) {
			clock_gettime(CLOCK_REALTIME, &datagram.timestamp);

			debug(2, "Datagram received ");
			print_b_array(buff, count, 0);
			pretty_print_datagram(&datagram);

			enqueue(&bleCentral->rxQueue, &datagram);
			APP_SetEvent(HCI_TaskID, HCI_RX_DATA_READY);
			updateRxStat(1, 0);

			memset(&datagram, 0, sizeof(datagram_t));
			offset = 0;
			count = 0;
		}
	}

	debug(1, "Read thread exiting\n");
	return NULL;
}

void *write_serial(void *_bleCentral)
{
	int n, l;
	char msg[STD_BUF_SIZE]; memset(msg, 0, STD_BUF_SIZE);
	BLE_Central_t *bleCentral = (BLE_Central_t *)_bleCentral;
	datagram_t datagram;
	debug(1, "Write thread started\n");

	while(bleCentral->_run){
		if(queueCount(&bleCentral->txQueue) == 0) {
			usleep(STD_WAIT_TIME);
			continue;
		}

		dequeue(&bleCentral->txQueue, &datagram);
		COM_compose_datagram(&datagram, msg, &l);

		n = write(bleCentral->fd, msg, l);
		updateTxStat(1, n);
		debug(2, "Message sent (%d bytes) ", n);
		print_b_array(msg, l, 0);
		pretty_print_datagram(&datagram);
		usleep(50*STD_WAIT_TIME);
	}

	debug(1, "Write thread exiting\n");
	return NULL;
}
