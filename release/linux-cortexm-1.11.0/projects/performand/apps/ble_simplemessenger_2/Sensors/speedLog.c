/*
 * speedLog.c
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>

#include "../dev_tools.h"
#include "../HCI_Parser/HCI_Defs.h"
#include "../../Common/common_tools.h"
#include "../../Common/MemoryMapping/memory_map.h"
#include "../../Common/utils.h"
#include "speedLog.h"

BLE_Peripheral_t *Log_Device;
int runtime_count, file_idx;

int Log_initialize(BLE_Peripheral_t *ble_device)
{
	Log_Device = ble_device;

	//Init mapped mem ...
	Log_Device->mapped_mem.filename = "speedlog";
	Log_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -1;	
	}
	
	runtime_count = read_rt_count();

	return 0;
}

// First thing to do is unload pduLength, handle and data
int Log_parseData(datagram_t* datagram, int *i)
{
	char pduLength = unload_8_bit(datagram->data, i);
	long handle = unload_16_bit(datagram->data, i, 1);
	char index = getIndexInAttributeArray(Log_Services, Log_ServiceCount, handle+1);

	if(index < 0) {
		fprintf(stderr, "ERROR: Log - Handle %#04X not found\n", handle);
		return index;
	}

	if(pduLength < 2+Log_Services[index].length) {
		fprintf(stderr, "ERROR: Log - Not enough data in datagram, pduLength=%d, expected %d for handle %#04X\n", pduLength, Log_Services[index].length+2, handle);
		return -1;
	}
	if(pduLength > 2+Log_Services[index].length) {
		fprintf(stderr, "WARNING: Log - Data length mismatch, pduLength=%d, expected %d for handle %#04X\n", pduLength, Log_Services[index].length+2, handle); 
		// Attempt to continue anyway.
	}

	char mm_str[100], d_str[12];
	memset(mm_str, '\0', sizeof(mm_str));
	memset(d_str, '\0', sizeof(d_str));
	format_timespec(mm_str, &datagram->timestamp);
	
	if(index == 0) { // Period attribute of type uint_16
		snprintf(d_str, sizeof(d_str), "%d", unload_16_bit(datagram->data, i, 1));
	}
	else if(index == 1) { // Battery attribute of type char
		snprintf(d_str, sizeof(d_str), "%d", unload_8_bit(datagram->data, i));
	}
	else { // Invalid attribute - go puke!
		snprintf(d_str, sizeof(d_str), "Invalid Hdl");
		index = Log_ServiceCount - 1;
	}

	char tmp_str[sizeof(d_str)+1];
	int j;
	for( j=0; j<Log_ServiceCount; j++ ) {
		sprintf(tmp_str, ",%s", index==j?d_str:"");
		strcat(mm_str, tmp_str);
	}
	//strcat(mm_str, Log_Services[index].description);
	strcat(mm_str, "\n");
	debug(1, "%s", mm_str);

	mm_append(mm_str, &Log_Device->mapped_mem);
	file_idx = write_log_file("log", runtime_count, file_idx, mm_str);

	return 0;
}
