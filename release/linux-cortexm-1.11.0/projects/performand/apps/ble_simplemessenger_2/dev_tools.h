/*
 * dev_tools.h
 *
 *  Created on: Sep 23, 2013
 *      Author: rasmus
 */

#ifndef DEV_TOOLS_H_
#define DEV_TOOLS_H_

#include "ble_device.h"

extern char unload_8_bit(char* data, int* i);
extern long unload_16_bit(char* data, int* i, int swap);

extern char compareMAC(char* MAC1, char* MAC2);
extern BLE_Peripheral_t* findDeviceByMAC(BLE_Central_t *central, char *MAC);
extern BLE_Peripheral_t* findDeviceByConnHandle(BLE_Central_t *central, long connHandle);
extern BLE_Peripheral_t* getNextAvailableDevice(BLE_Central_t *central, char *MAC);

extern void format_time_of_day(char* str, struct timeval tv);
#endif /* DEV_TOOLS_H_ */
