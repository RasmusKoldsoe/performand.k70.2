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
#include "../../Common/logging.h"
#include "../../Common/utils.h"
#include "compass.h"

BLE_Peripheral_t *Compass_Device;

int Compass_initialize(BLE_Peripheral_t *ble_device)
{
	Compass_Device = ble_device;

	Compass_Device->log->runtime_count = read_rt_count();

	if(creat_log(Compass_Device->log) < 0) {
		fprintf(stderr, "[BLUETOOTH] WARNING Could not open compass log file\n");
		return -1;
	}

	//Init mapped mem ...
	Compass_Device->mapped_mem.filename = "/sensors/compass";
	Compass_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -2;	
	}
	
	return 0;
}

void Compass_finalize( void )
{
	close_log(Compass_Device->log);
}

// First thing to do is unload pduLength, handle and data
int Compass_parseData(datagram_t* datagram, int *i)
{
	//char pduLength = unload_8_bit(datagram->data, i);
	char pduLength = unload_8_bit(datagram->data, i);
	unsigned short handle = unload_16_bit(datagram->data, i, 1);
	int hdl_index = getIndexInAttributeArray(Compass_Services, Compass_ServiceCount, handle+1);

	if(hdl_index < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: Compass - Handle %#04X not found\n", (unsigned short)handle);
		return hdl_index;
	}

#if (defined VERBOSITY) && (VERBOSITY >= 3)
	debug(3, "PduLen:\t(0x%02X) %d\n", (unsigned int)pduLength & 0xFF, pduLength);
	debug(3, "Handle:\t(0x%04X)\n", (unsigned int)handle & 0xFFFF);
	debug(3, "Value:\t"); print_byte_array(datagram->data, 13, 4);
#endif

	int j;
	char mm_str[100];
	memset(mm_str, '\0', sizeof(mm_str));

	format_timespec(mm_str, &datagram->timestamp);
	strcat(mm_str, ",");
	if( hdl_index == 0) {
		char index = unload_8_bit(datagram->data, i);
		char tmp_str[8];
		signed short value;

		if(index & 0x10) { // Compensated compass
			memset(tmp_str, '\0', sizeof(tmp_str));

			float f = unload_float(datagram->data, i);
			snprintf(tmp_str, sizeof(tmp_str), "%0.2f,", f);
			
			strcat(mm_str, tmp_str);
		}
		else  {
			strcat(mm_str, ",");
		}

		if(index & 0x04) { // Magnetometer value array
			for(j=0; j<3; j++) {
				memset(tmp_str, '\0', sizeof(tmp_str));
				value = (signed short)unload_16_bit(datagram->data, i, 0);
				snprintf(tmp_str, sizeof(tmp_str), "%d,", value);
				strcat(mm_str, tmp_str);
			}
		}
		else {
			strcat(mm_str, ",,,");
		}

		if(index & 0x01) { // Accelerometer value array
			for(j=0; j<3; j++) {
				memset(tmp_str, '\0', sizeof(tmp_str));
				value = (signed short)unload_16_bit(datagram->data, i, 1);
				snprintf(tmp_str, sizeof(tmp_str), "%d,", value);
				strcat(mm_str, tmp_str);
			}
		}
		else { 
			strcat(mm_str, ",,,");
		}
	}
	else if(hdl_index == 1) { // Battery attribute of type char
		char tmp_str[11];
		snprintf(tmp_str, 11, ",,,,,,,%d", unload_8_bit(datagram->data, i));
		strcat(mm_str, tmp_str);
	}
	else { // Invalid attribute - go puke!
		snprintf(mm_str, 7, ",,,,,,,");
	}

	strcat(mm_str, "\n");
	debug(1, "[%s] %s", Compass_Device->log->name, mm_str);
	mm_append(mm_str, &Compass_Device->mapped_mem);


	//write_log_file("compass", runtime_count, &file_idx, mm_str);
	append_log(Compass_Device->log, mm_str, strlen(mm_str));

	return 0;
}











