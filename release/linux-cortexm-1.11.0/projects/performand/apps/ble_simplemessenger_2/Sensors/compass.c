/*
 * compass.c
 *
 *   Created on: Jan 7, 2014
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
#include "compass.h"

BLE_Peripheral_t *Compass_Device;
int runtime_count, file_idx;

int Compass_initialize(BLE_Peripheral_t *ble_device)
{
	Compass_Device = ble_device;

	//Init mapped mem ...
	Compass_Device->mapped_mem.filename = "/sensors/compass";
	Compass_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -1;	
	}
	
	runtime_count = read_rt_count();

	return 0;
}

// First thing to do is unload pduLength, handle and data
int Compass_parseData(datagram_t* datagram, int *i)
{
	char pduLength = unload_8_bit(datagram->data, i);
	long handle = unload_16_bit(datagram->data, i, 1);
	char index = getIndexInAttributeArray(Compass_Services, Compass_ServiceCount, handle+1);
	int j;

#if 3 <= VERBOSITY
	debug(3, "PduLen:\t(0x%02X) %d\n", (unsigned int)pduLength & 0xFF, pduLength);
	debug(3, "Handle:\t(0x%04X)\n", (unsigned int)handle & 0xFFFF);
	debug(3, "Value:\t");
	print_byte_array(datagram->data, pduLength, 0);
#endif

	if(index < 0) {
		fprintf(stderr, "ERROR: Compass - Handle %#04X not found\n", handle);
		return index;
	}

	if(pduLength < 2+Compass_Services[index].length) {
		fprintf(stderr, "ERROR: Compass - Not enough data in datagram, pduLength=%d, expected %d for handle %#04X\n", pduLength, Compass_Services[index].length+2, handle);
		return -1;
	}
	if(pduLength > 2+Compass_Services[index].length) {
		fprintf(stderr, "WARNING: Compass - Data length mismatch, pduLength=%d, expected %d for handle %#04X\n", pduLength, Compass_Services[index].length+2, handle); 
		// Attempt to continue anyway.
	}

	char mm_str[100], d_str[25];
	memset(mm_str, '\0', sizeof(mm_str));
	memset(d_str, '\0', sizeof(d_str));

	format_timespec(mm_str+strlen(mm_str), &datagram->timestamp);
//	strcat(mm_str, ",$COMPASS");

	if(index == 0) { // Magnetometer value array
		char tmp_str[8];
		signed short value;
		for(j=0; j<3; j++) {
			memset(tmp_str, '\0', sizeof(tmp_str));
			value = (signed short)unload_16_bit(datagram->data, i, 0);
			snprintf(tmp_str, sizeof(tmp_str), "%d%s", value, j<2?",":"");
			strcat(d_str, tmp_str);
		}
		strcat(d_str, ",,,");
	}
	else if(index == 1) { // Accelerometer value array
		char tmp_str[8];
		signed short value;
		strcat(d_str, ",,");
		for(j=0; j<3; j++) {
			memset(tmp_str, '\0', sizeof(tmp_str));
			value = (signed short)unload_16_bit(datagram->data, i, 1);
			snprintf(tmp_str, sizeof(tmp_str), "%d%s", value, j<2?",":"");
			strcat(d_str, tmp_str);
		}
		strcat(d_str, ",");
	}
	else if(index == 2) { // Battery attribute of type char
		snprintf(d_str, sizeof(d_str), ",,,,,,%d", unload_8_bit(datagram->data, i));
	}
	else { // Invalid attribute - go puke!
		snprintf(d_str, sizeof(d_str), "Invalid Hdl");
		index = Compass_ServiceCount - 1;
	}

	char tmp_str[sizeof(d_str)+1];
	for( j=0; j<Compass_ServiceCount-1; j++ ) {
		sprintf(tmp_str, ",%s", index==j?d_str:"");
		strcat(mm_str, tmp_str);
	}
//	strcat(mm_str, Compass_Services[index].description);
	strcat(mm_str, "\n");
	debug(1, "%s", mm_str);

	mm_append(mm_str, &Compass_Device->mapped_mem);
	file_idx = write_log_file("compass", runtime_count, file_idx, mm_str);

	return 0;
}
