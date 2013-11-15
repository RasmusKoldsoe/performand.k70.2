/*
 * dev_tools.h
 *
 *  Created on: Sep 23, 2013
 *      Author: rasmus
 */

#ifndef DEV_TOOLS_H_
#define DEV_TOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <time.h>

#include "ble_device.h"

char unload_8_bit(char* data, int* i);
long unload_16_bit(char* data, int* i, int swap);
char *unload_mac_addr(char* data, int* i);

char compareMAC(char* MAC1, char* MAC2);
BLE_Peripheral_t* findDeviceByMAC(BLE_Central_t *central, char *MAC);
BLE_Peripheral_t* findDeviceByConnHandle(BLE_Central_t *central, long connHandle);
BLE_Peripheral_t* getNextAvailableDevice(BLE_Central_t *central, char *MAC);

void format_time_of_day(char* str, struct timeval tv);
#endif /* DEV_TOOLS_H_ */
