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
#include <time.h>

#include "ble_device.h"
#include "dev_tools.h"
#include "HCI_Parser/HCI_Parser.h"
#include "GATT_Parser/GATT_Parser.h"
#include "APP.h"

void processTimeouts(void);
#define eventHandlerFcn_Count 3
const eventHandlerFcn taskArr[] = {
	HCI_ProcessEvent,
	GATT_ProcessEvent,
	APP_ProcessEvent
};

#define delayedEvents_MaxCount 10
struct delayedEvent_t {
	int _enabled;
	long taskID;
	long events;
	struct timespec timestamp;
} delayedEvents[delayedEvents_MaxCount];

pthread_mutex_t app_event_mutex;
BLE_Central_t *bleCentral;
long AppEvents[sizeof(taskArr)];
int APP_TaskID;
static long devices;

void APP_Run( void )
{
	int idx = 0;
	processTimeouts();

	do {
		if(APP_GetEvent(idx) & 0x0000FFFF) {
			break;
		}
	} while(++idx < eventHandlerFcn_Count);

	if(idx < eventHandlerFcn_Count) {
		long events;

		events = APP_GetEvent(idx);
		APP_ClearEvent(idx);
		debug(2, "Task ID %d events before call: %08X\n", idx, events);
		events = (taskArr[idx])(idx, events);
		debug(2, "Task ID %d events after call: %08X\n", idx, events);
		APP_SetEvent(idx, events);

		//usleep(10*STD_WAIT_TIME);
	}
	else {
		usleep(10*STD_WAIT_TIME); // 100ms
	}
}

int _shutdown = 0;
long APP_ProcessEvent(int taskID, long events)
{
	(void)taskID;
	if(events & APP_STARTUP_EVENT) {
	// When system is starting up, initialize hardware. Assume serial port is already open
		APP_SetEvent(GATT_TaskID, GATT_INITIALIZE);
		return events ^ APP_STARTUP_EVENT;
	}

	if(events & APP_SHUTDOWN_EVENT) {
	// Request to shutdown program, sign off and disconnect to all connected peripheral devices.
		APP_SetEvent(GATT_TaskID, GATT_TERMINATE_CONNECTION | devices);
		_shutdown = 1;
		return events ^ APP_SHUTDOWN_EVENT;
	}

	if(events & APP_DEVICE_INIT_DONE_EVENT) {
	// When system is initialized, connect to all devices
		printf("Devices: %08X\n",devices);
		APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | devices);
		return events ^ APP_DEVICE_INIT_DONE_EVENT;
	}

	if(events & APP_CONNECTION_ESTABLISHED) {
	// When a device is connected, enable all services
		APP_SetEvent(GATT_TaskID, GATT_ENABLE_SERVICES | (events & 0xFFFF0000));
		return events ^ (APP_CONNECTION_ESTABLISHED | (events & 0xFFFF0000));
	}

	if(events & APP_CONNECTION_TERMINATED) {
	// When a device is disconnected, do nothing right now (TODO: Reconnect if not in process of terminating)
		debug(2, "CONNECTION TERMINATED %08X\n", events);

		if(_shutdown) {
			int i;
			int stillConnected = 0;
			for(i=0; i<sizeof(*bleCentral->devices)/sizeof(BLE_Peripheral_t); i++) {
				stillConnected += bleCentral->devices[i]._connected;
			}

			if(stillConnected == 0) {
				printf("ALL DEVICES DISCONNECTED - TERMINATING PROGRAM NOW!\n");
				bleCentral->_run = 0;
			}
		}
		else {
			APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | events & 0xFFFF0000);
		}

		return events ^ APP_CONNECTION_TERMINATED;
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

	memset(delayedEvents, 0, sizeof(delayedEvents));


	devices = 0;
	//devices |= bleCentral->devices[0].ID; // Log
	//devices |= bleCentral->devices[1].ID; // KeyFob
	devices |= bleCentral->devices[0].ID; // Wind sensor

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

// if first is larger than last return 1
int tscomp(struct timespec *first, struct timespec *last)
{
	long first_ms = (first->tv_sec*1000 + first->tv_nsec/1000000.0) + 0.5;
	long last_ms  = ( last->tv_sec*1000 +  last->tv_nsec/1000000.0) + 0.5;
	if(first_ms > last_ms) return 1;
	return 0;
}

void processTimeouts(void)
{
	int idx;
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	for(idx=0; idx < delayedEvents_MaxCount; idx++) {
		if(delayedEvents[idx]._enabled > 0) { // if enabled
			if(tscomp(&now, &delayedEvents[idx].timestamp)) {
				APP_SetEvent(delayedEvents[idx].taskID, delayedEvents[idx].events);
				APP_ClearTimer(idx);
			}
		}
	}
}

// Timeout in ms
int APP_StartTimer(int taskID, long events, int timeout)
{
	if(taskID > eventHandlerFcn_Count) return -1; // not a valid task number

	int newIndex;
	for(newIndex=0; newIndex < delayedEvents_MaxCount; newIndex++) {
		if(delayedEvents[newIndex]._enabled == 0) break;
	}
	
	clock_gettime(CLOCK_REALTIME, &delayedEvents[newIndex].timestamp);
	delayedEvents[newIndex].timestamp.tv_sec += timeout/1000;
	delayedEvents[newIndex].timestamp.tv_nsec += (timeout % 1000) * 1000000;
	delayedEvents[newIndex].events = events;
	delayedEvents[newIndex].taskID = taskID;
	delayedEvents[newIndex]._enabled = 1;

#if (defined VERBOSITY) && (VERBOSITY >= 2)
	char str_a[100], str_b[100];
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	format_time_of_day(str_a, &ts);
	format_time_of_day(str_b, &delayedEvents[newIndex].timestamp);
	printf("%s Timer %d Started with timeout %s\n", str_a, newIndex, str_b);
#endif
	return 0;
}

int APP_ClearTimer(int index)
{
	if(index > delayedEvents_MaxCount) return -1;

	delayedEvents[index]._enabled = 0;

#if (defined VERBOSITY) && (VERBOSITY >= 2)
	struct timespec ts;
	char str[100];
	clock_gettime(CLOCK_REALTIME, &ts);
	format_time_of_day(str, &ts);
	printf("%s Timer %d Removed\n", str, index);
#endif
	return 0;
}

int  APP_ClearTimerByEvent(int taskID, long events)
{
	int i;
	for(i=0; i<delayedEvents_MaxCount; i++) {
		if(delayedEvents[i].taskID == taskID && (delayedEvents[i].events & 0x00FF) == (events & 0x00FF) && delayedEvents[i]._enabled > 0) {
			APP_ClearTimer(i);
			return 0;
		}
	}
	return -1;
}















