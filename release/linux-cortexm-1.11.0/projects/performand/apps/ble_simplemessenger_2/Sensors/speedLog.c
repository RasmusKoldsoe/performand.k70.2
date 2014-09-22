/*
 * speedLog.c
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
#include "speedLog.h"

BLE_Peripheral_t *Log_Device;

int Log_initialize(BLE_Peripheral_t *ble_device)
{
	Log_Device = ble_device;

	Log_Device->log->runtime_count = read_rt_count();

	if(creat_log(Log_Device->log) < 0) {
		fprintf(stderr, "[BLUETOOTH] WARNING Could not open speedlog log file\n");
		return -1;
	}

	//Init mapped mem ...
	Log_Device->mapped_mem.filename = "/sensors/speedlog";
	Log_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -2;	
	}

	return 0;
}

void Log_finalize( void )
{
	close_log(Log_Device->log);
}

// First thing to do is unload pduLength, handle and data
int Log_parseData(datagram_t* datagram, int *i)
{
	char pduLength = unload_8_bit(datagram->data, i);
	long handle = unload_16_bit(datagram->data, i, 1);
	char index = getIndexInAttributeArray(Log_Services, Log_ServiceCount, handle+1);

	if(index < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: Log - Handle %#04X not found\n", (unsigned int)handle);
		return index;
	}

	if(pduLength < 2+Log_Services[(ssize_t)index].length) {
		fprintf(stderr, "[Bluetooth] ERROR: Log - Not enough data in datagram, pduLength=%d, expected %d for handle %#04X\n", pduLength, Log_Services[(ssize_t)index].length+2, (unsigned int)handle);
		return -1;
	}
	if(pduLength > 2+Log_Services[(ssize_t)index].length) {
		fprintf(stderr, "[Bluetooth] WARNING: Log - Data length mismatch, pduLength=%d, expected %d for handle %#04X\n", pduLength, Log_Services[(ssize_t)index].length+2, (unsigned int)handle); 
		// Attempt to continue anyway.
	}

	char mm_str[100], d_str[12];
	memset(mm_str, '\0', sizeof(mm_str));
	memset(d_str, '\0', sizeof(d_str));

	format_timespec(mm_str+strlen(mm_str), &datagram->timestamp);
//	strcat(mm_str, ",$LOG");

	if(index == 0) { // Period attribute of type uint_16
		snprintf(d_str, sizeof(d_str), "%d", (int)unload_16_bit(datagram->data, i, 1));
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
	debug(1, "[%s] %s", Log_Device->log->name, mm_str);
	mm_append(mm_str, &Log_Device->mapped_mem);

	//write_log_file("log", runtime_count, &file_idx, mm_str);
	append_log(Log_Device->log, mm_str, strlen(mm_str));


	return 0;
}
