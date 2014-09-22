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
#include "../../Common/logging.h"
#include "../../Common/utils.h"
#include "wind.h"

typedef union {
	float f;
	char c[sizeof(float)];
} float_u;

BLE_Peripheral_t *Wind_Device;

int Wind_initialize(BLE_Peripheral_t *ble_device)
{
	Wind_Device = ble_device;

	Wind_Device->log->runtime_count = read_rt_count();

	if(creat_log(Wind_Device->log) < 0) {
		fprintf(stderr, "[BLUETOOTH] WARNING Could not open wind log file\n");
		return -1;
	}

	//Init mapped mem ...
	Wind_Device->mapped_mem.filename = "/sensors/wind";
	Wind_Device->mapped_mem.size = DEFAULT_FILE_LENGTH;

	//Prepare the mapped Memory file
	if((mm_prepare_mapped_mem(&ble_device->mapped_mem)) < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: While mapping %s file.\n",ble_device->mapped_mem.filename);
		return -2;	
	}

	return 0;
}

void Wind_finalize( void )
{
	close_log(Wind_Device->log);
}

// First thing to do is unload pduLength, handle and data
int Wind_parseData(datagram_t* datagram, int *i)
{
	char pduLength = unload_8_bit(datagram->data, i);
	long handle = unload_16_bit(datagram->data, i, 1);
	char hdl_index = getIndexInAttributeArray(Wind_Services, Wind_ServiceCount, handle);

	if(hdl_index < 0) {
		fprintf(stderr, "[Bluetooth] ERROR: Wind - Handle %#04X not found\n", (unsigned int)handle);
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
	if( hdl_index == 0 ) { // Wind attribute of type float_u
		char tmp_str[10];
		float_u dataPoint;
		for(j=0; j<Wind_Services[(ssize_t)hdl_index].length; j++) {
			dataPoint.c[j] = unload_8_bit(datagram->data, i); // Unload Wind Speed
		}
		snprintf(tmp_str, 10, "%3.1f,,,", dataPoint.f);
		strcat(mm_str, tmp_str);
	}
	else if( hdl_index == 1 ) { // Wind attribute of type float_u
		char tmp_str[20];
		float_u speed, direction;
		for(j=0; j<Wind_Services[(ssize_t)hdl_index].length/2; j++) {
			speed.c[j] = unload_8_bit(datagram->data, i); // Unload Wind Speed
		}

		for(j=0; j<Wind_Services[(size_t)hdl_index].length/2; j++) {
			direction.c[j] = unload_8_bit(datagram->data, i); // Unload Wind Direction
		}
		snprintf(tmp_str, 20, ",%3.1f,%3.1f,", speed.f, direction.f);
		strcat(mm_str, tmp_str);
	}
	else if( hdl_index == 2 ) { // Battery attribute of type char
		char tmp_str[8];
		snprintf(tmp_str, 8, ",,,,%d", (unsigned int)unload_8_bit(datagram->data, i));
		strcat(mm_str, tmp_str);
	}
	else { // Invalid attribute - go puke!
		strcat(mm_str, ",,,,");
	}

	strcat(mm_str, "\n");
	debug(1, "[%s] %s", Wind_Device->log->name, mm_str);
	mm_append(mm_str, &Wind_Device->mapped_mem);

	//write_log_file("wind", runtime_count, &file_idx, mm_str);
	append_log(Wind_Device->log, mm_str, strlen(mm_str));


	return 0;
}
