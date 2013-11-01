/*
 * HCI_Parser.c
 *
 *   Created on: Sep 20, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../ble_device.h"
#include "../dev_tools.h"
#include "../APP.h"
#include "../Queue/Queue.h"
#include "../GATT_Parser/GATT_Parser.h"
#include "HCI_Defs.h"
#include "HCI_Parser.h"

int HCI_Parse(void);

BLE_Central_t *bleCentral;
datagram_t datagram; // Queue require this to be an instance, NOT a pointer!
int HCI_TaskID;

int led_value = 1;

void HCI_Init(int taskID, BLE_Central_t *b)
{
	HCI_TaskID = taskID;
	bleCentral = b;
}

long HCI_ProcessEvent(int taskID, long events)
{
	(void) taskID;
	memset(&datagram, 0, sizeof(datagram_t));

	if(events & HCI_DATA_READY) {
		while(queueCount(&bleCentral->rxQueue) > 0) {
			HCI_Parse();
		}
		return events ^ HCI_DATA_READY;
	}

	return 0;
}


int HCI_Parse(void)
{
	int i = 0;

	dequeue(&bleCentral->rxQueue, &datagram);
	if(datagram.type == Event) {
		if((datagram.opcode & 0xFF) == HCI_LE_ExtEvent) {
			long evtCode = unload_16_bit(datagram.data, &i, 1);
			char success = unload_8_bit(datagram.data, &i);
			debug(3, "Event:\t(%02X %02X)\n", (unsigned int)evtCode >> 8 & 0xFF, (unsigned int)evtCode & 0xFF);
			debug(3, "Status:\t(%02X) %s\n",(unsigned int)success & 0xFF, getSuccessString(success));

			switch(evtCode) {
			case GAP_DeviceInitDone:
				{
					debug(1, "GAP_DeviceInitDone return %s with MAC addr: ", getSuccessString(success));

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
					debug(1, "GAP_EstablishLink ");
					debug(1, "%s to device ", (success==0)?"successfully connected":"failed to connect");
					i++; // Device address type

					int j;
					char connMAC[6];
					for(j=5; j>=0; j--) {
						connMAC[j] = datagram.data[i++];
					}
					for(j=0; j<6; j++) {
						debug(1, "%02X%c", (unsigned int)connMAC[j] & 0xFF, (j<5)?':':' ');
					}

					if(success == HCI_SUCCESS) {
						/**************************************************************************
						 * If a device with the given MAC address is already defined this will
						 * be returned, otherwise the first unused device handle will be returned
						 **************************************************************************/
						BLE_Peripheral_t* device = findDeviceByMAC(bleCentral, connMAC);
						if(device == NULL) {//No available devices: bail out
							debug(1, "\nWARNING: HCI Connected to unknown device\n");
							break;
						}

						memcpy(device->connMAC, connMAC, 6);
						device->_defined = 1;
						device->_connected = 1;
						device->connHandle = unload_16_bit(datagram.data, &i, 1);
						debug(1, "with connHandle 0x%04X\n", device->connHandle);

						APP_SetEvent(APP_TaskID, APP_CONNECTION_ESTABLISHED | 0x1 << (device->connHandle + 16));
					}
				}
				break;
			case GAP_TerminateLink:
				{
					debug(1, "GAP_TerminateLink ");
					long connHandle = unload_16_bit(datagram.data, &i, 1);
					char reason = unload_8_bit(datagram.data, &i);

					debug(1, "connHandle 0x%04X with reason %s\n", (unsigned int)connHandle & 0xFFFF, getTerminateString(reason));

					BLE_Peripheral_t* device = findDeviceByConnHandle(bleCentral, connHandle);
					device->_connected = 0;

					APP_SetEvent(APP_TaskID, APP_CONNECTION_TERMINATED | 0x1 << (device->connHandle + 16));
					break;
				}
			case GAP_HCI_ExtentionCommandStatus:
				{
					debug(1, "GAP_HCI_ExtentionCommandStatus ");
					long opCode = 0;
					opCode = unload_16_bit(datagram.data, &i, 1);

					switch(opCode) {
					case GATT_WriteCharValue:
						debug(1, "return %s for WriteCharValue\n", getSuccessString(success));
						break;
					case GAP_DeviceInit:
						debug(1, "return %s for DeviceInit\n", getSuccessString(success));
						break;
					case GAP_EstablishLinkRequest:
						debug(1, "return %s for EstablishLinkRequest\n", getSuccessString(success));
						break;
					case GAP_TerminateLinkRequest:
						debug(1, "return %s for TerminateLinkRequest\n", getSuccessString(success));
						break;
					default:
						debug(1, "return %s for unknown opcode %04X\n", getSuccessString(success), (unsigned int)opCode);
					}
					break;
				}
			default:
			// If HCI don't support the OpCode, try GATT
				enqueue(&GATT_Rx_Queue, &datagram);
				APP_SetEvent(GATT_TaskID, GATT_DATA_READY);
			}
		}
		else {
			debug(1, "Datagram eventCode not supported: %04X\n", (unsigned int)datagram.opcode);
		}
	}
	else {
		debug(1, "Datagram type not supported: %04X\n", datagram.type);
	}
}





















