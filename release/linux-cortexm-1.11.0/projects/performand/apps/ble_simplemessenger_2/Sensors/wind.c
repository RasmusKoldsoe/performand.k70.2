/*
 * wind.c
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include <string.h>

#include "../dev_tools.h"
#include "../../Common/common_tools.h"
#include "wind.h"

typedef union {
	float f;
	char c[sizeof(float)];
} float_u;

BLE_Peripheral_t *bleDevice;

int Wind_initialize(BLE_Peripheral_t *ble_device)
{
	bleDevice = ble_device;

	//Init mapped mem ...
	bleDevice->mapped_mem.filename = "wind";
	bleDevice->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -1;	
	}
	
	return 0;
}

// First thing to do is unload pduLength, handle and data
int Wind_parseData(datagram_t* datagram, int *i, char* mm_str)
{
	char pduLength = unload_8_bit(datagram->data, i);
	if(pduLength < 2+sizeof(float)) {
		fprintf(stderr, "ERROR: Wind - Not enough data in datagram, pduLength=%d\n", pduLength);
		return -1;
	}
	if(pduLength > 2+sizeof(float)) {
		fprintf(stderr, "WARNING: Wind - Data length mismatch, pduLength=%d\n", pduLength); // Attempt to continue anyway.
	}

	long handle = unload_16_bit(datagram->data, i, 1);

	int idx;
	float_u dataPoint;
	for(idx=0; idx<sizeof(float); idx++) {
		dataPoint.c[idx] = unload_8_bit(datagram->data, i);
	}
	
	for(idx=0; idx<Wind_nServiceHdls; idx++) {
		if(handle+1 == Wind_serviceHdls[idx])
			break;
	}
	if(idx == Wind_nServiceHdls) {
		fprintf(stderr, "ERROR: Wind - Could not find requested serviceHandle 0x%04X\n", handle);
		return -1;
	}
	
	int h;
	char d[10], str[10];
	snprintf(d, 10, "%3.1f", dataPoint.f);
	format_time_of_day(mm_str, &datagram->timestamp);

	for(h=0; h<Wind_nServiceHdls; h++) {
		sprintf(str, ",%s", idx==h?d:"");
		strcat(mm_str, str);
	}

	strcat(mm_str, "\n");
	mm_append(mm_str, &bleDevice->mapped_mem);

	return 0;
}

