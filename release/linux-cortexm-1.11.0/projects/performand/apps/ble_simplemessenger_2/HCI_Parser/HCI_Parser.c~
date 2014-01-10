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
queue_t HCI_Tx_Queue;
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

	if(events & HCI_COMMAND_TIMEOUT) {
		APP_SetEvent(APP_TaskID, APP_HCI_ERROR);
		fprintf(stderr, "Bluetooth controller not responding\n");
		return events ^ (HCI_COMMAND_TIMEOUT | HCI_TX_DATA_READY);
	}

	if(events & HCI_RX_DATA_READY) {
		while(queueCount(&bleCentral->rxQueue) > 0) {
			HCI_Parse();
		}
		return events ^ HCI_RX_DATA_READY;
	}

	if(events & HCI_TX_DATA_READY) {
		if(queueCount(&HCI_Tx_Queue) > 0) {
			APP_StartTimer(HCI_TaskID, HCI_COMMAND_TIMEOUT | HCI_TX_DATA_READY, 10000); // 10 sec
		}
		while(queueCount(&HCI_Tx_Queue) > 0) {
			dequeue(&HCI_Tx_Queue, &datagram);
			enqueue(&bleCentral->txQueue, &datagram);
		}
		return events ^ HCI_TX_DATA_READY;
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
			int esg = (int)(evtCode & 0x380);
			esg = (int)(esg >> 7);

			debug(3, "Event:\t(0x%02X%02X)\n", (unsigned int)evtCode >> 8 & 0xFF, (unsigned int)evtCode & 0xFF);
			debug(3, "Status:\t(0x%02X) %s\n", (unsigned int)success & 0xFF, getSuccessString(success, esg));

			switch(evtCode) {
			case GAP_HCI_ExtentionCommandStatus:
				{
					debug(1, "GAP_HCI_ExtentionCommandStatus return %s for ", getSuccessString(success, esg));

					long opCode = 0;
					opCode = unload_16_bit(datagram.data, &i, 1);
					debug(3, "OpCode:\t(%02X %02X)\n", (unsigned int)opCode >> 8 & 0xFF, (unsigned int)opCode & 0xFF);

					switch(opCode) {
					case GATT_WriteCharValue:
						debug(1, "WriteCharValue\n");
						break;
					case GAP_DeviceInit:
						debug(1, "DeviceInit\n");
						break;
					case GAP_EstablishLinkRequest:
						debug(1, "EstablishLinkRequest\n");
						break;
					case GAP_TerminateLinkRequest:
						debug(1, "TerminateLinkRequest\n");
						break;
					default:
						debug(1, "unknown opcode %04X\n", (unsigned int)opCode);
					}
					APP_ClearTimerByEvent(HCI_TaskID, HCI_COMMAND_TIMEOUT | HCI_TX_DATA_READY);
					break;
				}
			default:
			// If HCI don't support the OpCode, try GATT/GAP
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





















