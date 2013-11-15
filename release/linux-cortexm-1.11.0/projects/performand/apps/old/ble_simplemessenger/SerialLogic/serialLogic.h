/*
 * serialLogic.h
 *
 *   Created on: Sep 19, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark
 */

#ifndef SERIALLOGIC_H_
#define SERIALLOGIC_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "../ble_device.h"
#include "../Queue/Queue.h"
#include "../COM_Parser/COM_Parser.h"
#include "../NetworkStat/NetworkStatistics.h"

int open_serial(char *port, int oflags);
void close_serial(int fd);
void *read_serial(void *_bleCentral); // thread
void *write_serial(void *_bleCentral); // thread

#endif /* SERIALLOGIC_H_ */
