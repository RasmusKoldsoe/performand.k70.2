/*
 * dev_tools.c
 *
 *  Created on: Sep 23, 2013
 *      Author: rasmus
 */
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

char *unload_mac_addr(char* data, int *i)
{
	int j;
	char *mac;
	mac = (char*)malloc(6);

	for(j=5; j>=0; j--) {
		mac[j] = data[*i++];
	}

	return mac;
}

char compareMAC(char* MAC1, char* MAC2) 
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


void format_time_of_day(char* str, struct timeval tv)
{
	char tmbuf[100];
	struct tm *tmp;
	
	time_t t = (time_t)tv.tv_sec;
	tmp = localtime(&t);
	if(tmp == NULL) {
		sprintf(str, "ERROR: localtime\n");
		return;
	}
	strftime(tmbuf, sizeof(tmbuf),  "%Y-%m-%d %H:%M:%S", tmp);
	sprintf(str, "%s.%06d", tmbuf, tv.tv_usec);
}









