/*
 * HCI_Parser.h
 *
 *   Created on: Sep 20, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef COMPARSER_H_
#define COMPARSER_H_

#define HCI_DATA_READY 0x0001

#include "../ble_device.h"

extern int HCI_TaskID;

extern void HCI_Init(int taskID, BLE_Central_t *b);
extern long HCI_ProcessEvent(int taskID, long events);

#endif /* COMPARSER_H_ */
