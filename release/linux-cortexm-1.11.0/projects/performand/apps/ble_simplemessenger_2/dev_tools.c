/*
 * dev_tools.c
 *
 *  Created on: Sep 23, 2013
 *      Author: rasmus
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
//#include <math.h>
//#include <sys/mman.h>
#include <time.h>

#include "dev_tools.h"

char unload_8_bit(char* data, int *i)
{
	char b = data[*i] & 0xFF;
	*i += 1;
	return b;
}

long unload_16_bit(char* data, int *i, int swap)
{
	long h, rval;
	long l;
	if(swap) {
		l = *(data + *i) & 0x00FF;
		h = *(data + *i + 1) << 8;
	}
	else {
		l = *(data + *i + 1) & 0x00FF;
		h = *(data + *i) << 8;
	}
	*i += 2;
	rval = (h + l) & 0xFFFF;
	return rval;
}

void unload_MAC(char* data, int *i, char* MAC)
{
	char* MAC_i = MAC + 5;

	do {
		*MAC_i-- = data[*i];
		*i += 1;
	} while(MAC <= MAC_i);
}


char compareMAC(const char* MAC1, const char* MAC2) 
{ 
	int i; 
	for(i=0; i<6; i++) { 
		if( *MAC1++ != *MAC2++ ) return 0; 
	} 
	return 1; 
} 

BLE_Peripheral_t* findDeviceByMAC(BLE_Central_t *central, char *MAC) 
{ 
	int i; 
	for(i=0; i<MAX_PERIPHERAL_DEV; i++) { 
		if(central->devices[i]._defined) { 
			if(compareMAC(central->devices[i].connMAC, MAC)) { 
				return &(central->devices[i]); 
			} 
		} 
	} 
	return NULL; 
} 

BLE_Peripheral_t* findDeviceByConnHandle(BLE_Central_t *central, long connHandle) 
{
	int i; 
	for(i=0; i<MAX_PERIPHERAL_DEV; i++) { 
		if(central->devices[i]._defined) { 
			if(central->devices[i].connHandle == connHandle) { 
				return &(central->devices[i]); 
			} 
		} 
	} 
	return NULL; 
} 

BLE_Peripheral_t* getNextAvailableDevice(BLE_Central_t *central, char *MAC) 
{
	BLE_Peripheral_t* device = findDeviceByMAC(central, MAC); 
	if (device != NULL) return device; 
 
	int i;       
	for(i=0; i<MAX_PERIPHERAL_DEV; i++) { 
		if(central->devices[i]._defined == 0) { 
			return &(central->devices[i]);
		} 
	} 
	return NULL; 
}

long getDevIDbyConnHandle(BLE_Central_t *central, long connHandle)
{
	BLE_Peripheral_t* device = findDeviceByConnHandle(central, connHandle); 
	if (device = NULL) return -1; 

	return device->ID;
}

size_t getIndexInAttributeArray(attribute_t* arr, size_t length, long token)
{
	length--;
	for(; length >= 0; length--) {
		if(arr[length].handle == token)
			return length;
	}
	return -1;
}

