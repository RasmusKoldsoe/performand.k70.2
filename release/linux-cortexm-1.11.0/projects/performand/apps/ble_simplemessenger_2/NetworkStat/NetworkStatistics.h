/*
 * NetworkStatistics.h
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */

#ifndef NETWORKSTATISTICS_H_
#define NETWORKSTATISTICS_H_

extern void initNetworkStat();
extern void updateTxStat(int datagrams, int bytes);
extern void updateRxStat(int datagrams, int bytes);
extern void printNetworkStat();

#endif /* NETWORKSTATISTICS_H_ */
