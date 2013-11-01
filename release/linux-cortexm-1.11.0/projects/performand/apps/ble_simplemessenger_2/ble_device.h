/*
 * ble_device.h
 *
 *  Created on: Sep 20, 2013
 *      Author: rasmus
 */

#ifndef BLE_DEVICE_H_
#define BLE_DEVICE_H_

#define STD_WAIT_TIME   10000 // us -> 10 ms
#define STD_TIMEOUT     5    // sec
#define STD_TIMEOUT_CNT (STD_TIMEOUT * 1000000)/STD_WAIT_TIME

#define MAX_PERIPHERAL_DEV 2 // Hardware don't support more than 5 simultanious connections
//#define STD_BUF_SIZE 64

#define MAC_ADDR_SIZE 6
#define SERVICE_HDL_SIZE 2

#include "Queue/Queue.h"
#include "../Common/common_tools.h"

typedef struct {
		long connHandle;
		char connMAC[6];
		long serviceHDLS[5];
		char _connected;
		char _defined;
} BLE_Peripheral_t;

typedef struct {
		char *port;
		int fd;
		char MAC[6];
		int _run;
		BLE_Peripheral_t *devices;
		queue_t txQueue;
		queue_t rxQueue;

		char *mapped_mem;
		int rt_count;
} BLE_Central_t;

#endif /* BLE_DEVICE_H_ */
