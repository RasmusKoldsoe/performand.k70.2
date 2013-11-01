/*
 * APP.c
 *
 *   Created on: Oct 29, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include <sys/resource.h>

#include "ble_device.h"
#include "dev_tools.h"
#include "HCI_Parser/HCI_Parser.h"
#include "GATT_Parser/GATT_Parser.h"
#include "APP.h"

const eventHandlerFcn taskArr[] = {
	HCI_ProcessEvent,
	GATT_ProcessEvent,
	APP_ProcessEvent
};

pthread_mutex_t app_event_mutex;
BLE_Central_t *bleCentral;
long AppEvents[sizeof(taskArr)];
int timeoutCounter;
int APP_TaskID;

void APP_Run( void )
{
	timeoutCounter = 0;
	int idx = 0;
	
	do {
printf("APP_GetEvent(%d) = %d\n", idx, APP_GetEvent(idx) & 0xFFFF);
		if(APP_GetEvent(idx) & 0x0000FFFF) {
			break;
		}
	} while(++idx < 3);

	if(idx < 3) {
		long events;
		events = APP_GetEvent(idx);
		APP_ClearEvent(idx);
printf("id %d events before call: %08X\n", idx, events);
		events = (taskArr[idx])(idx, events);
printf("id %d events after call: %08X\n", idx, events);
		APP_SetEvent(idx, events);
	}
	else {
		sleep(1);
	}
sleep(2);
}

long APP_ProcessEvent(int taskID, long events)
{
	(void)taskID;
	if(events & APP_STARTUP_EVENT) {
	// When system is starting up, initialize hardware. Assume serial port is already open
		APP_SetEvent(GATT_TaskID, GATT_INITIALIZE);
		return events ^ APP_STARTUP_EVENT;
	}

	if(events & APP_SHUTDOWN_EVENT) {
	// Request to shutdown program, sign off and disconnect to peripheral devices.
		return events ^ APP_SHUTDOWN_EVENT;
	}

	if(events & APP_DEVICE_INIT_DONE_EVENT) {
	// When system is initialized, connect to all devices
		APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | 0x3 << 16);
		return events ^ APP_DEVICE_INIT_DONE_EVENT;
	}

	if(events & APP_CONNECTION_ESTABLISHED) {
	// When a device is connected, enable all services
		APP_SetEvent(GATT_TaskID, GATT_ENABLE_SERVICES | (events & 0x3 << 16));
		return events ^ APP_CONNECTION_ESTABLISHED;
	}

	if(events & APP_CONNECTION_TERMINATED) {
	// When a device is disconnected, do nothing right now (TODO: Reconnect)
		//APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | 0x11 << connid + 16);
		return events ^ APP_CONNECTION_ESTABLISHED;
	}

	return 0;
}

int APP_Init(BLE_Central_t *b)
{
	bleCentral = b;
	int i;

	i = pthread_mutex_init(&app_event_mutex, NULL);
	if(i < 0) {
		perror("ERROR: APP Initializing event mutex");
		return i;
	}

	memset(AppEvents, 0, sizeof(AppEvents));

	int t_id=0;
	HCI_Init(t_id++, bleCentral);
	GATT_Init(t_id++, bleCentral);
	APP_TaskID = t_id;

	return 0;
}

int APP_Exit(void)
{
	pthread_mutex_lock(&app_event_mutex);
	pthread_mutex_destroy(&app_event_mutex);
}

long APP_SetEvent(int taskID, long events)
{
	long rval;
	pthread_mutex_lock(&app_event_mutex);
	AppEvents[taskID] |= events;
	rval = AppEvents[taskID];
	pthread_mutex_unlock(&app_event_mutex);

	return rval;
}

long APP_GetEvent(int taskID)
{
	long rval;
	pthread_mutex_lock(&app_event_mutex);
	rval = AppEvents[taskID];
	pthread_mutex_unlock(&app_event_mutex);

	return rval;
}

void APP_ClearEvent(int taskID)
{
	pthread_mutex_lock(&app_event_mutex);
	AppEvents[taskID] = 0;
	pthread_mutex_unlock(&app_event_mutex);
}









