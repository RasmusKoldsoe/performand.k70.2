/*
 * NetworkStatistics.h
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */

#ifndef NETWORKSTATISTICS_H_
#define NETWORKSTATISTICS_H_

void initNetworkStat();
void updateTxStat(int datagrams, int bytes);
void updateRxStat(int datagrams, int bytes);
void printNetworkStat();

#endif /* NETWORKSTATISTICS_H_ */
