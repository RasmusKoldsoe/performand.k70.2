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
#include "../HCI_Parser/HCI_Defs.h"
#include "../Queue/Queue.h"
#include "../APP.h"
#include "../../Common/GPIO/gpio_api.h"
#include "../../Common/MemoryMapping/memory_map.h"
#include "GATT_Parser.h"

int GATT_Parse(void);

BLE_Central_t *bleCentral;
datagram_t datagram; // Queue require this to be an instance, NOT a pointer !!
queue_t GATT_Rx_Queue;
int GATT_TaskID;

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
}

long GATT_ProcessEvent(int taskID, long events)
{
	(void) taskID;
	memset(&datagram, 0, sizeof(datagram_t));

	if( events & GATT_DATA_READY ) {
		while(queueCount(&GATT_Rx_Queue) > 0) {
			GATT_Parse();
		}

		return events ^ GATT_DATA_READY;
	}

	if( events & GATT_INITIALIZE ) {
		get_GAP_DeviceInit(&datagram);
		enqueue(&bleCentral->txQueue, &datagram);

		return events ^ GATT_INITIALIZE;
	}

	if( events & GATT_ESTABLISH_CONNECTION ) {
		int i;
printf("number of devices %d\n", sizeof(*bleCentral->devices)/sizeof(BLE_Peripheral_t)+1);
		for(i=0; i<=sizeof(*bleCentral->devices)/sizeof(BLE_Peripheral_t); i++) {
printf("Selecting device %d: %08X - ", i, (0x1 << (16+i)));
			if( events & (0x1 << (16+i))) {
				get_GAP_EstablishLinkRequest(&datagram, bleCentral->devices[i].connMAC);
GATT_pretty_print_datagram(&datagram);
				enqueue(&bleCentral->txQueue, &datagram);
				events ^ (0x1 << (16+i));
			}
		}

		return events ^ GATT_ESTABLISH_CONNECTION;
	}

	if(events & GATT_ENABLE_SERVICES) {
		int i, j;

		for(i=0; i<sizeof(bleCentral->devices)/sizeof(BLE_Peripheral_t); i++) {
			if( events & (0x1 << (16+i))) {
				for(j=0; j<5; j++) {
					if(bleCentral->devices[i].serviceHDLS[j] != 0) {	
						char data[] = {0x01, 0x00};
						get_GATT_WriteCharValue(&datagram, bleCentral->devices[i].connHandle, bleCentral->devices[i].serviceHDLS[j], data, 2);
						enqueue(&bleCentral->txQueue, &datagram);
					}
				}
				events ^ (0x1 << (16+i));
			}
		}
		return events ^ GATT_ENABLE_SERVICES;
	}

	if(events & GATT_DISABLE_SERVICES) {
		int i, j;

		for(i=0; i<sizeof(bleCentral->devices)/sizeof(BLE_Peripheral_t); i++) {
			if( events & (0x1 << (16+i))) {
				for(j=0; j<5; j++) {
					if(bleCentral->devices[i].serviceHDLS[j] != 0) {	
						char data[] = {0x00, 0x00};
						get_GATT_WriteCharValue(&datagram, bleCentral->devices[i].connHandle, bleCentral->devices[i].serviceHDLS[j], data, 2);
						enqueue(&bleCentral->txQueue, &datagram);
					}
				}
				events ^ (0x1 << (16+i));
			}
		}
		return events ^ GATT_DISABLE_SERVICES;
	}

	if( events & GATT_TERMINATE_CONNECTION) {
	/* If connection to peripheral device is now terminated, start searching for device again */
		events ^ GATT_TERMINATE_CONNECTION;
	}
	return 0;
}


int GATT_Parse(void)
{
// We can be sure that datagrams dequeued here is of type Event and opcode is HCI_LE_ExtEvent. Therefore we don't need to check again
	dequeue(&GATT_Rx_Queue, &datagram);
	int i = 0;

	long evtCode = unload_16_bit(datagram.data, &i, 1);
	char success = unload_8_bit(datagram.data, &i);
	debug(3, "Event:\t(%02X %02X)\n", (unsigned int)evtCode >> 8 & 0xFF, (unsigned int)evtCode & 0xFF);
	debug(3, "Status:\t(%02X) %s\n",(unsigned int)success & 0xFF, getSuccessString(success));

	switch(evtCode) {
	case ATT_ErrorRsp:
		{	
			debug(1, "ATT_ErrorResponce return %s\n", getSuccessString(success));
			break;
		}
	case ATT_WriteRsp:
		{
			debug(1, "ATT_WriteResponce return %s\n", getSuccessString(success));
			break;
		}
	case ATT_HandleValueNotification:
		{
			int j;
			debug(1, "ATT_HandleValueNotifiction return %s ", getSuccessString(success));
			long connHandle = unload_16_bit(datagram.data, &i, 1);

			char pduLength = unload_8_bit(datagram.data, &i);
			if(pduLength < 2) {
				debug(1, "ERROR: HCI Not enough data in pdu\n");
				break;
			}

			long handle = unload_16_bit(datagram.data, &i, 1);
			char str[100];
			format_time_of_day(str, datagram.timestamp);

			debug(1, "from connHandle %04X for handle 0x%04X at %s with data: ", (unsigned int)connHandle & 0xFFFF, (unsigned int)handle & 0xFFFF, str);
			for(j=0; j<pduLength-2; j++) {
				debug(1, "%02X ", (unsigned int)datagram.data[i++] & 0xFF);
			}
			debug(1, "\n");
/*			data = ((datagram.data[9]&0xFF)<<8)+(datagram.data[8]&0xFF);
						
			//Check if there is more space in the mapped mem. We need Bytes for the
			//XML string.
			mm_get_next_available(bleCentral->mapped_mem, 2);
	
			sprintf(buf,"%d",data*2);
			mm_append_to_XMLfile(bleCentral->rt_count,buf,bleCentral->mapped_mem);
			bleCentral->rt_count++;

			if( !gpio_setValue(LED_IND3, led_value) ) {
				printf("ERROR: Exporting gpio port.");
				return 0;
			}
			led_value=1-led_value;	
*/
			break;
		}
	default:
		debug(1, "HCI_LE_ExtEvent OpCode %04X not supported by HCI or GATT\n", (unsigned int)datagram.opcode);
	}
}










