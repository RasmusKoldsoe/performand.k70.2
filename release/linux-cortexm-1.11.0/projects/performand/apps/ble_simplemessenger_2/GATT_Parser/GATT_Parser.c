/*
 * GATT_Server.c
 *
 *   Created on: Oct 28, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <string.h>

#include "../ble_device.h"
#include "../dev_tools.h"
#include "../Queue/Queue.h"
#include "../HCI_Parser/HCI_Defs.h"
#include "../HCI_Parser/HCI_Parser.h"
#include "../APP.h"
#include "../../Common/GPIO/gpio_api.h"
#include "../../Common/MemoryMapping/memory_map.h"
#include "GATT_Parser.h"


#define BLE_DEVICE_COUNT MAX_PERIPHERAL_DEV

int GATT_Parse(void);

BLE_Central_t *bleCentral;
datagram_t datagram; // Queue require this to be an instance, NOT a pointer !!
queue_t GATT_Rx_Queue;
int GATT_TaskID;
long backupOfDevicesToConnect;
long lastEstablishDeviceID;
int deviceIterator;



//TODO: Remove this function !!!
void GATT_pretty_print_datagram(datagram_t *datagram)
{
	int i;

	printf("Type:\t(%02X) %s\n", datagram->type & 0xff, datagram->type==Command?"Command":"Event");

	printf("Opcode:\t(");
	if(datagram->type == Command)
		printf("%02X ", (char)(datagram->opcode >> 8) & 0xff);
	printf("%02X)\n", (char)datagram->opcode & 0xff);

	printf("Length:\t(%02X) %d bytes\n", datagram->data_length & 0xff, datagram->data_length);

	printf("Data:\t(");
	for(i=0; i<datagram->data_length; i++)
		printf("%02X%s", datagram->data[i] & 0xff, i<datagram->data_length-1?" ":")\n");

}

void GATT_Init(int taskID, BLE_Central_t *b)
{
	GATT_TaskID = taskID;
	bleCentral = b;
	GATT_Rx_Queue = queueCreate();
	backupOfDevicesToConnect = 0;
	lastEstablishDeviceID = 0;
	deviceIterator = 0;
}

void GATT_Transmit_Datagram(datagram_t *dgm, long event)
{
	if(dgm == NULL) {
		return;
	}
	if(enqueue(&HCI_Tx_Queue, dgm) < 0) {
		fprintf(stderr, "[BLUETOOTH] WARNING GATT_Parser HCI_Tx_Queue was full");
	}
	APP_SetEvent(HCI_TaskID, HCI_TX_DATA_READY);
	if(APP_StartTimer(GATT_TaskID, GATT_COMMAND_TIMEOUT | event, 10000) < 0) {
		fprintf(stderr, "[BLUETOOTH] ERROR caused by GATT_Transmit_Datagram\n");
	}
}

long GATT_ProcessEvent(int taskID, long events)
{
	(void)taskID;

	memset(&datagram, 0, sizeof(datagram_t));

	if( events & GATT_COMMAND_TIMEOUT ) {
		// If CANCEL COMMAND timeout -> Do nothing
		if( events & GATT_CANCEL_COMMAND) {
			debug(1, "Cancel command timed out: %08X\n", (unsigned int)events);
			//APP_SetEvent(APP_TaskID, APP_SHUTDOWN_EVENT);
			events ^= GATT_CANCEL_COMMAND;
		}

		// If INITIALIZE timeout - Shut down
		if( events & GATT_INITIALIZE) {
			APP_SetEvent(APP_TaskID, APP_SHUTDOWN_EVENT);
			debug(1, "Initialize timed out.\n");
			events ^= GATT_INITIALIZE;
		}

		// If ESTABLISH CONNECTION timeout -> cancel request
		if( events & GATT_ESTABLISH_CONNECTION ) {
			APP_SetEvent(GATT_TaskID, GATT_CANCEL_COMMAND | GATT_ESTABLISH_CONNECTION);
			debug(1, "Establish Connection timed out.\n");
			events ^= GATT_ESTABLISH_CONNECTION;
			events &= ~0xFFFF0000;
		}
		
		// If ENABLE SERVICES timeout, schedule for later
		if(events & GATT_ENABLE_SERVICES){}

		return events ^ GATT_COMMAND_TIMEOUT;
	}

	if( events & GATT_CANCEL_COMMAND ) {
		if( events & GATT_ESTABLISH_CONNECTION ) {
			get_GAP_TerminateLinkRequest(&datagram, 0xFFFE); // Cancel establish connection command
			GATT_Transmit_Datagram(&datagram, (GATT_CANCEL_COMMAND | GATT_ESTABLISH_CONNECTION) & 0x0000FFFF);
			events ^= GATT_ESTABLISH_CONNECTION;
		}
		
		return events ^ GATT_CANCEL_COMMAND;
	}

	if( events & GATT_DATA_READY ) {
		while(queueCount(&GATT_Rx_Queue) > 0) {
			GATT_Parse();
		}

		return events ^ GATT_DATA_READY;
	}

	if( events & GATT_INITIALIZE ) {
		get_GAP_DeviceInit(&datagram);
		GATT_Transmit_Datagram(&datagram, GATT_INITIALIZE);
		return events ^ GATT_INITIALIZE;
	}

	/************************************************************************************************
	 * Establish connection.                                                                        *
	 * For all defined peripherals, request link if selected in 'events'. Timeout after 30 seconds  *
	 ************************************************************************************************/
	if( events & GATT_ESTABLISH_CONNECTION ) {
		int j;

		if( events == GATT_ESTABLISH_CONNECTION) // meaning no device ID set
			return events ^ GATT_ESTABLISH_CONNECTION;

		for(j=0; j<BLE_DEVICE_COUNT; j++) {
			if( (events & bleCentral->devices[j].ID) && (bleCentral->devices[j]._connected == 0) && (bleCentral->devices[j]._defined > 0)) {
				break;
			}
		}
		if(j == BLE_DEVICE_COUNT)
			return events &= ~GATT_ESTABLISH_CONNECTION;

		BLE_Peripheral_t *device = findDeviceByID(bleCentral, bleCentral->devices[j].ID); 

		if( device != NULL ) { // Device ID not found
			get_GAP_EstablishLinkRequest(&datagram, device->connMAC);
			GATT_Transmit_Datagram(&datagram, GATT_ESTABLISH_CONNECTION | device->ID);

#if (defined VERBOSITY) && (VERBOSITY >= 1)
			char mac_s[19]; int m;
			for(m=0; m<6; m++){
				sprintf(mac_s+(3*m), "%02X%c", (unsigned int)device->connMAC[m]&0xFF, (m<5)?':':'\n');
			}
			printf("GATT_EstablishConnection: Connecting to device %s", mac_s);
#endif
			events &= ~device->ID;
		}
		else {
			fprintf(stderr, "[Bluetooth] GATT ERROR: Device ID %#08X (e %#08X) not found\n", (unsigned int)bleCentral->devices[j].ID, (unsigned int)(events & 0xFFFF0000));
			return events &= ~GATT_ESTABLISH_CONNECTION;
		}
	
		if((events & 0xFFFF0000) != 0)
			APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | (events & 0xFFFF0000));

		return events &= ~GATT_ESTABLISH_CONNECTION;

	}

	if(events & GATT_ENABLE_SERVICES) {
		int j;

		BLE_Peripheral_t *device = findDeviceByID(bleCentral, events & 0xFFFF0000);

		if( device != NULL ) { // Device ID found
			char data[] = {0x00, 0x01};

			for(j=0; j<device->serviceHdlsCount; j++) {
				get_GATT_WriteCharValue(&datagram, device->connHandle, device->serviceHdls[j].handle, data, 2);
				GATT_Transmit_Datagram(&datagram, GATT_ENABLE_SERVICES | device->ID);
			}
			events &= ~device->ID;
		}
		return events ^ GATT_ENABLE_SERVICES;
	}

	if(events & GATT_DISABLE_SERVICES) {
		int i, j;

		for(i=0; i<=BLE_DEVICE_COUNT; i++) {
			if( events & bleCentral->devices[i].ID) {
				char data[] = {0x00, 0x00};
				for(j=0; j<bleCentral->devices[i].serviceHdlsCount; j++) {
					get_GATT_WriteCharValue(&datagram, bleCentral->devices[i].connHandle, bleCentral->devices[i].serviceHdls[j].handle, data, 2);
					GATT_Transmit_Datagram(&datagram, GATT_DISABLE_SERVICES | bleCentral->devices[i].ID);
				}
				return events ^ bleCentral->devices[i].ID;
			}
		}
		return events ^ GATT_DISABLE_SERVICES;
	}

	if( events & GATT_READ_CHARACTERISTIC ) { // ONLY READS THE LAST HANDLE IN ATTRIBUTE LIST. (BATTERY VALUE)
		BLE_Peripheral_t *device = findDeviceByID(bleCentral, events & 0xFFFF0000);

		if( device != NULL ) { // Device ID found
			get_GATT_ReadCharValue( &datagram, device->connHandle, device->serviceHdls[device->serviceHdlsCount-1].handle-1 );
			GATT_Transmit_Datagram( &datagram, GATT_READ_CHARACTERISTIC | device->ID);
		}

		return events ^ GATT_READ_CHARACTERISTIC;
	}

	if( events & GATT_TERMINATE_CONNECTION ) {
		int i;

		for(i=0; i<=BLE_DEVICE_COUNT; i++) {
			if( events & bleCentral->devices[i].ID && bleCentral->devices[i]._connected) {
				get_GAP_TerminateLinkRequest(&datagram, bleCentral->devices[i].connHandle);
				GATT_Transmit_Datagram(&datagram, GATT_TERMINATE_CONNECTION | bleCentral->devices[i].ID);
				events ^= bleCentral->devices[i].ID;
			}
		}

	// No devices to terminate, schedule APP_CONNECTION_TERMINATED with no devices
		//APP_SetEvent(APP_TaskID, APP_CONNECTION_TERMINATED);

		return events ^ GATT_TERMINATE_CONNECTION;
	}

	return 0;
}

void print_s_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%c", buff[i] & 0xff);
	}
	printf("\n");
}

int GATT_Parse(void)
{
// We can be sure that datagrams dequeued here is of type Event and opcode is HCI_LE_ExtEvent. Therefore we don't need to check again
	dequeue(&GATT_Rx_Queue, &datagram);
	int i = 0;

	long evtCode = unload_16_bit(datagram.data, &i, 1);
	char success = unload_8_bit(datagram.data, &i);
	int esg = (int)(evtCode & 0x380);
	esg = (int)(esg >> 7);

//	debug(3, "Event:\t(%02X %02X)\n", (unsigned int)evtCode >> 8 & 0xFF, (unsigned int)evtCode & 0xFF);
//	debug(3, "Status:\t(%02X) %s\n",(unsigned int)success & 0xFF, getSuccessString(success, esg));

	switch(evtCode) {
	case GAP_DeviceInitDone:
		{
			APP_ClearTimerByEvent(GATT_TaskID, GATT_INITIALIZE);
			debug(1, "GAP_DeviceInitDone return %s with MAC addr: ", getSuccessString(success, esg));
			
			int j;
			for(j=5; j>=0; j--) {
				bleCentral->MAC[j] = datagram.data[i++];
			}
			for(j=0; j<6; j++) {
				debug(1, "%02X%c", (unsigned int)bleCentral->MAC[j] & 0xFF, (j<5)?':':'\n');
			}

			APP_SetEvent(APP_TaskID, APP_DEVICE_INIT_DONE_EVENT);
			break;
		}
	case GAP_EstablishLink:
		{
			debug(1, "GAP_EstablishLink return %s. ", getSuccessString(success, esg));

			char address_type = unload_8_bit(datagram.data, &i);
			char connMAC[6];
			memset(connMAC, 0, sizeof(connMAC));
			unload_MAC(datagram.data, &i, connMAC);

			if(success == HCI_SUCCESS) {
				/**************************************************************************
				 * If a device with the given MAC address is already defined this will
				 * be returned, otherwise the first unused device handle will be returned
				 **************************************************************************/
				BLE_Peripheral_t* device = findDeviceByMAC(bleCentral, connMAC);
				if(device == NULL) { // Device ID not found
					fprintf(stderr, "[Bluetooth] ERROR: GATT Establish - No available device handles\n");
					break;
				}
#if (defined VERBOSITY) && (VERBOSITY >= 1)
				debug(1, "Connected to device ");

				int j;
				for(j=0; j<6; j++) {
					debug(1, "%02X%c", (unsigned int)connMAC[j] & 0xFF, (j<5)?':':' ');
				}
#endif

				device->_connected = 1;
				device->connHandle = unload_16_bit(datagram.data, &i, 1);
				debug(1, "with connHandle 0x%04X.\n", (unsigned int)device->connHandle);

				APP_ClearTimerByEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | device->ID);
				APP_SetEvent(APP_TaskID, APP_CONNECTION_ESTABLISHED | device->ID);
			}
			else if(success == HCI_ERR_RESERVED2) { // Establish link canceled
				// Remember to clear device ID part of event. This has been set to 0xFFFE to get here.
				debug(1, "Link Request Canceled.\n");
				APP_ClearTimerByEvent(GATT_TaskID, GATT_CANCEL_COMMAND | GATT_ESTABLISH_CONNECTION);
				APP_SetEvent(APP_TaskID, APP_CONNECTION_CANCLED);
			}
			else {
				fprintf(stderr, "[Bluetooth] WARNING: GATT EstablishLink return unknown status code\n");
			}

			break;
		}
	case GAP_TerminateLink:
		{
			debug(1, "GAP_TerminateLink ");

			long connHandle = unload_16_bit(datagram.data, &i, 1);
			char reason = unload_8_bit(datagram.data, &i);

			debug(1, "connHandle 0x%04X with reason %s\n", (unsigned int)connHandle & 0xFFFF, getTerminateString(reason));

			BLE_Peripheral_t* device = findDeviceByConnHandle(bleCentral, connHandle);
			if(device != NULL) {
				device->_connected = 0;
				APP_ClearTimerByEvent(GATT_TaskID, GATT_TERMINATE_CONNECTION | device->ID);
				APP_SetEvent(APP_TaskID, APP_CONNECTION_TERMINATED);
			}
			break;
		}
	case ATT_ErrorRsp:
		{	
			debug(1, "ATT_ErrorResponce return %s\n", getSuccessString(success, esg));
			break;
		}
	case ATT_ReadRsp:
		{
			long connHandle = unload_16_bit(datagram.data, &i, 1);
			debug(1, "ATT_ReadResponce return %s for connHandle 0x%04X\n", getSuccessString(success, esg), (unsigned int)connHandle);
			
			BLE_Peripheral_t* device = findDeviceByConnHandle(bleCentral, connHandle);
			if(device != NULL) {
				APP_SetEvent(APP_TaskID, GATT_READ_CHARACTERISTIC | device->ID);
				if( device->parseDataCB(&datagram, &i) < 0 ) {
					fprintf(stderr, "[Bluetooth] ERROR: GATT ReadCharacteristicValue - Parse Data failed\n");
				}
			}
			break;
		}
	case ATT_WriteRsp:
		{
			long connHandle = unload_16_bit(datagram.data, &i, 1);
			debug(1, "ATT_WriteResponce return %s for connHandle 0x%04X\n", getSuccessString(success, esg), (unsigned int)connHandle);
			
			BLE_Peripheral_t* device = findDeviceByConnHandle(bleCentral, connHandle);
			if(device != NULL) {
				APP_ClearTimerByEvent(GATT_TaskID, GATT_ENABLE_SERVICES | device->ID);
			}
			
			break;
		}
	case ATT_HandleValueNotification:
		{
			//debug(1, "ATT_HandleValueNotifiction return %s\n", getSuccessString(success, esg));

			long connHandle = unload_16_bit(datagram.data, &i, 1);
			//debug(3, "ConnHandle:\t(%#04X) %d\n", (unsigned int)connHandle & 0xFFFF, connHandle);

			BLE_Peripheral_t *curr_device;
			curr_device = findDeviceByConnHandle(bleCentral, connHandle);

			if(curr_device != NULL) {
				//debug(1, "from connHandle %04X, handle 0x%04X at %s with data: ", \
							(unsigned int)connHandle & 0xFFFF, (unsigned int)handle & 0xFFFF, str);

				if( curr_device->parseDataCB(&datagram, &i) < 0 ) {
					fprintf(stderr, "[Bluetooth] ERROR: GATT HandleValueNotification - Parse Data failed\n");
				}
			}
			else {
				fprintf(stderr, "[Bluetooth] ERROR: GATT HandleValueNotification - Connection handle not identified\n");
			}
			break;
		}
	default:
		debug(1, "HCI_LE_ExtEvent OpCode %04X not supported by HCI or GATT\n", (unsigned int)datagram.opcode);
	}
	return 0;
}










