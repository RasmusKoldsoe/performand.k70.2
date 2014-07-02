/*
 * GATT_Server.h
 *
 *   Created on: Oct 28, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef GATT_SERVER_H_
#define GATT_SERVER_H_

#define GATT_DATA_READY                  0x0001
#define GATT_INITIALIZE                  0x0002
#define GATT_ESTABLISH_CONNECTION        0x0004
#define GATT_TERMINATE_CONNECTION        0x0008
#define GATT_ENABLE_SERVICES             0x0010
#define GATT_DISABLE_SERVICES            0x0020
#define GATT_HDL_BY_UUID				 0x0040
#define GATT_READ_CHARACTERISTIC         0x0080
#define GATT_CANCEL_COMMAND              0x4000
#define GATT_COMMAND_TIMEOUT             0x8000


#include "../Queue/Queue.h"

extern int GATT_TaskID;
extern queue_t GATT_Rx_Queue;

extern void GATT_Init(int taskID, BLE_Central_t *b);
extern long GATT_ProcessEvent(int taskID, long events);

#endif /* GATT_SERVER_H_ */
