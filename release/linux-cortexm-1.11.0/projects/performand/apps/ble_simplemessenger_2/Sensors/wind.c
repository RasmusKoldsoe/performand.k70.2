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
#include "../HCI_Parser/HCI_Defs.h"
#include "../../Common/common_tools.h"
#include "../../Common/MemoryMapping/memory_map.h"
#include "../../Common/utils.h"
#include "wind.h"

typedef union {
	float f;
	char c[sizeof(float)];
} float_u;

BLE_Peripheral_t *Wind_Device;
int runtime_count, file_idx;

int Wind_initialize(BLE_Peripheral_t *ble_device)
{
	Wind_Device = ble_device;

	//Init mapped mem ...
	Wind_Device->mapped_mem.filename = "/sensors/wind";
	Wind_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -1;	
	}

	runtime_count = read_rt_count();
	
	return 0;
}

// First thing to do is unload pduLength, handle and data
int Wind_parseData(datagram_t* datagram, int *i)
{
	char pduLength = unload_8_bit(datagram->data, i);
	long handle = unload_16_bit(datagram->data, i, 1);
	char index = getIndexInAttributeArray(Wind_Services, Wind_ServiceCount, handle+1);

	if(index < 0) {
		fprintf(stderr, "ERROR: Wind - Handle %#04X not found\n", handle);
		return index;
	}

	if(pduLength < 2+Wind_Services[index].length) {
		fprintf(stderr, "ERROR: Wind - Not enough data in datagram, pduLength=%d, expecting=%d for handle %#04X\n", pduLength, Wind_Services[index].length+2, handle);
		return -1;
	}
	else if(pduLength > 2+Wind_Services[index].length) {
		fprintf(stderr, "WARNING: Wind - Data length mismatch, pduLength=%d, expecting=%d for handle %#04X\n", pduLength, Wind_Services[index].length+2, handle); 
		// Attempt to continue anyway.
	}

	char mm_str[100], d_str[12];
	memset(mm_str, '\0', sizeof(mm_str));
	memset(d_str, '\0', sizeof(d_str));
	
	format_timespec(mm_str+strlen(mm_str), &datagram->timestamp);
//	strcat(mm_str, ",$WIND");

	if( index < 3 ) { // Wind attribute of type float_u
		int j;
		float_u dataPoint;
		for(j=0; j<Wind_Services[index].length; j++) {
			dataPoint.c[j] = unload_8_bit(datagram->data, i);
		}
		snprintf(d_str, sizeof(d_str), "%3.1f", dataPoint.f);
	}
	else if( index < 4 ) { // Battery attribute of type char
		char dataPoint = unload_8_bit(datagram->data, i);
		snprintf(d_str, sizeof(d_str), "%d", (unsigned int)dataPoint & 0xFF);
	}
	else { // Invalid attribute - go puke!
		snprintf(d_str, sizeof(d_str), "Invalid Hdl");
		index = Wind_ServiceCount - 1;
	}

	int j;
	char tmp_str[sizeof(d_str)+1];
	memset(tmp_str, '\0', sizeof(tmp_str));

	for(j=0; j<Wind_ServiceCount; j++) {
		sprintf(tmp_str, ",%s", index==j?d_str:"");
		strcat(mm_str, tmp_str);
	}
	//strcat(mm_str, Wind_Services[index].description);
	strcat(mm_str, "\n");
	debug(1, "%s", mm_str);
	mm_append(mm_str, &Wind_Device->mapped_mem);
	file_idx = write_log_file("wind", runtime_count, file_idx, mm_str);

	return 0;
}
