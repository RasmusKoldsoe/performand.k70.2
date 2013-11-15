/*
 * NetworkStatistics.c
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */

#include <stdio.h>
#include "NetworkStatistics.h"

typedef struct {
	int rxDatagrams;
	int txDatagrams;
	int rxBytes;
	int txBytes;
} netStat_t;

netStat_t networkStatistic;

void initNetworkStat()
{
	networkStatistic.rxBytes = 0;
	networkStatistic.rxDatagrams = 0;
	networkStatistic.txBytes = 0;
	networkStatistic.txDatagrams = 0;
}

void updateTxStat(int datagrams, int bytes)
{
	if(datagrams < 0) datagrams = 0;
	if(bytes < 0) bytes = 0;

	networkStatistic.txDatagrams += datagrams;
	networkStatistic.txBytes += bytes;
}

void updateRxStat(int datagrams, int bytes)
{
	if(datagrams < 0) datagrams = 0;
	if(bytes < 0) bytes = 0;

	networkStatistic.rxDatagrams += datagrams;
	networkStatistic.rxBytes += bytes;
}

void printNetworkStat()
{
	printf("Network Statistics:\nReceived %3d Datagrams\n\t %3d Bytes\nSent\t %3d Datagrams\n\t %3d Bytes\n", \
			networkStatistic.rxDatagrams, networkStatistic.rxBytes, networkStatistic.txDatagrams, networkStatistic.txBytes);
}
