/*
 * speedLog.c
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>

#include "../dev_tools.h"
#include "../../Common/common_tools.h"
#include "speedLog.h"

BLE_Peripheral_t *bleDevice;

int Log_initialize(BLE_Peripheral_t *ble_device)
{
	bleDevice = ble_device;

	//Init mapped mem ...
	bleDevice->mapped_mem.filename = "speedlog";
	bleDevice->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -1;	
	}
	
	return 0;
}

// First thing to do is unload pduLength, handle and data
int Log_parseData(datagram_t* datagram, int *i, char* mm_str)
{
	char pduLength = unload_8_bit(datagram->data, i) - 2;
	long handle = unload_16_bit(datagram->data, i, 1);
// Battery attribute is 1 byte long
// Log Period attribute is 2 bytes long
	char hdlLength = (handle==0x0030)?1:2;

	if(pduLength < hdlLength) {
		fprintf(stderr, "ERROR: Log - Not enough data in datagram, pduLength=%d, expected %d for handle %#04X\n", pduLength, hdlLength, handle);
		return -1;
	}
	if(pduLength > hdlLength) {
		fprintf(stderr, "WARNING: Log - Data length mismatch, pduLength=%d, expected %d for handle %#04X\n", pduLength, hdlLength, handle); // Attempt to continue anyway.
	}

	int idx;
	char d[10];
	
	if(handle+1 == Log_serviceHdls[idx = 0]) { // period
		snprintf(d, 10, "%d", unload_16_bit(datagram->data, i, 1));
	}
	else if(handle+1 == Log_serviceHdls[idx = 1]) { // Battery
		snprintf(d, 10, "%d", unload_8_bit(datagram->data, i));
	}
	else {
		fprintf(stderr, "ERROR: Log - Could not find requested serviceHandle 0x%04X\n", handle);
		return -1;
	}

	char str[10];
	int h;
	for( h=0; h<Log_nServiceHdls; h++ ) {
		sprintf(str, ",%s", idx==h?d:"");
		strcat(mm_str, str);
	}
	strcat(mm_str, "\n");
	mm_append(mm_str, &bleDevice->mapped_mem);

	return 0;
}
