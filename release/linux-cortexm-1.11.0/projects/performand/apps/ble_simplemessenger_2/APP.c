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
#include "../Common/utils.h"
#include "APP.h"

#define max(a,b) (a>b?a:b)
#define BLE_DEVICE_COUNT 3 //sizeof(*bleCentral->devices)/sizeof(BLE_Peripheral_t)

void processTimeouts(void);

#define eventHandlerFcn_Count 3
const eventHandlerFcn taskArr[eventHandlerFcn_Count] = {
	HCI_ProcessEvent,
	GATT_ProcessEvent,
	APP_ProcessEvent
};

#define MAX_DELAYED_EVENTS 20
struct delayedEvent_t {
	int _enabled;
	long taskID;
	long events;
	struct timespec timestamp;
} delayedEvents[MAX_DELAYED_EVENTS];


pthread_mutex_t app_event_mutex;
BLE_Central_t *bleCentral;

int APP_TaskID;
long AppEvents[eventHandlerFcn_Count];
long devices;
int deviceIterator;

//static struct timespec idle, stop;
//static int worktime, idletime;

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
//		debug(1, "Task ID %d events before call: %08X\n", idx, events);
		events = (taskArr[idx])(idx, events);
//		debug(1, "Task ID %d events after call: %08X\n", idx, events);
		APP_SetEvent(idx, events);
	}
	else {
//clock_gettime(CLOCK_REALTIME, &idle);
//worktime = timespec_subtract(&idle, &stop);
		usleep(50000); // 50ms
//clock_gettime(CLOCK_REALTIME, &stop);
//idletime = timespec_subtract(&stop, &idle);
//printf("worktime= %dus, idletime= %dus, Workload %0.4f\n", worktime, idletime, (float)((float)worktime/((float)worktime+(float)idletime)*100.0));
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
	// Request to shutdown program, disconnect to all connected peripheral devices.
		APP_SetEvent(GATT_TaskID, GATT_TERMINATE_CONNECTION | devices);
		_shutdown = 1;
		return events ^ APP_SHUTDOWN_EVENT;
	}

	if(events & APP_DEVICE_INIT_DONE_EVENT) {
	// When system is initialized, connect to the first unconnected device.
		APP_SetEvent(APP_TaskID, APP_CONNECT_NEXT_DEVICE);
		return events ^ APP_DEVICE_INIT_DONE_EVENT;
	}

	if(events & APP_CONNECT_NEXT_DEVICE) {
		int j;
		int timer_index = APP_FindTimerByEvent(GATT_TaskID, GATT_COMMAND_TIMEOUT | GATT_ESTABLISH_CONNECTION);

		if( timer_index >= 0 ) {
			// Already performing Establish Connection. System is waiting for reply from device.
			// If device does not answer before timeout, APP_CONNECT_NEXT_DEVICE will be called again. Thus exit without doing anything
			return events ^ APP_CONNECT_NEXT_DEVICE;
		}

		for(j = 0; j <= BLE_DEVICE_COUNT; j++) {
			deviceIterator++;
			if(deviceIterator >= BLE_DEVICE_COUNT)
				deviceIterator = 0;

			if(bleCentral->devices[deviceIterator].ID & devices && bleCentral->devices[deviceIterator]._connected == 0) {
				APP_SetEvent(GATT_TaskID, GATT_ESTABLISH_CONNECTION | bleCentral->devices[deviceIterator].ID);
				break;
			}
		}
		return events ^ APP_CONNECT_NEXT_DEVICE;
	}

	if(events & APP_CONNECTION_ESTABLISHED) {
	// When a device is connected, enable all services

		APP_SetEvent(GATT_TaskID, GATT_ENABLE_SERVICES | (events & 0xFFFF0000));
		APP_SetEvent(APP_TaskID, APP_CONNECT_NEXT_DEVICE);
		return events ^ (APP_CONNECTION_ESTABLISHED | (events & 0xFFFF0000));
	}

	if(events & APP_CONNECTION_TERMINATED) {
	// When a device is disconnected, reconnect if not terminating
		debug(2, "CONNECTION TERMINATED %08X\n", (unsigned int)events);

		if(_shutdown == 1) {
			int i;
			int stillConnected = 0;
			for(i=0; i<BLE_DEVICE_COUNT; i++) {
				stillConnected += bleCentral->devices[i]._connected;
			}

			if(stillConnected == 0) {
				printf("ALL DEVICES DISCONNECTED - TERMINATING PROGRAM NOW!\n");
				bleCentral->_run = 0;
			}
		}
		else {
			APP_SetEvent(APP_TaskID, APP_CONNECT_NEXT_DEVICE);
		}

		return events ^ APP_CONNECTION_TERMINATED;
	}

	if(events & APP_CONNECTION_CANCLED) {
		APP_SetEvent(APP_TaskID, APP_CONNECT_NEXT_DEVICE);
		return events ^ APP_CONNECTION_CANCLED;
	}

	return 0;
}

int APP_Init(BLE_Central_t *b, long dev)
{
	bleCentral = b;
	devices = dev;
	deviceIterator = BLE_DEVICE_COUNT;
	_delayedEvents_MaxCount = 0;
	int i;

	i = pthread_mutex_init(&app_event_mutex, NULL);
	if(i < 0) {
		fprintf(stderr, "ERROR: APP - Initializing event mutex\n");
		return i;
	}

	memset(AppEvents, 0, sizeof(AppEvents));
	memset(delayedEvents, 0, sizeof(delayedEvents));

	int t_id=0;

	HCI_Init(t_id++, bleCentral);
	GATT_Init(t_id++, bleCentral);
	APP_TaskID = t_id;

	for(i=0; i<BLE_DEVICE_COUNT; i++) {
		if(devices & bleCentral->devices[i].ID) {
			bleCentral->devices[i].initialize(&bleCentral->devices[i]);
		}
	}
	return 0;
}

void APP_Exit(void)
{
	int i;
	for(i=0; i<BLE_DEVICE_COUNT; i++) {
		if(devices & bleCentral->devices[i].ID) {
			bleCentral->devices[i].finalize();
		}
	}

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

/*
 * lhs < rhs:  return <0
 * lhs == rhs: return 0
 * lhs > rhs:  return >0
 */
int tscomp(const struct timespec *lhs, const struct timespec *rhs)
{
	if (lhs->tv_sec < rhs->tv_sec)
		return -1;
	if (lhs->tv_sec > rhs->tv_sec)
		return 1;
	return lhs->tv_nsec - rhs->tv_nsec;
}

void processTimeouts(void)
{
	int idx;
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	for(idx=0; idx < MAX_DELAYED_EVENTS; idx++) {
		if(delayedEvents[idx]._enabled > 0) { // if enabled
			if(tscomp(&now, &delayedEvents[idx].timestamp) > 0) {
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
	for(newIndex=0; newIndex < MAX_DELAYED_EVENTS; newIndex++) {
		if(delayedEvents[newIndex]._enabled == 0) 
			break;
	}
	if(newIndex >= MAX_DELAYED_EVENTS) {
		fprintf(stderr, "[BLUETOOTO] ERROR APP - No available timers ready.\n");
		return -1;
	}
	_delayedEvents_MaxCount = max(_delayedEvents_MaxCount, newIndex);

	clock_gettime(CLOCK_REALTIME, &delayedEvents[newIndex].timestamp);
	delayedEvents[newIndex].timestamp.tv_sec += timeout / 1000;
	delayedEvents[newIndex].timestamp.tv_nsec += (timeout % 1000) * 1000000;
	delayedEvents[newIndex].events = events;
	delayedEvents[newIndex].taskID = taskID;
	delayedEvents[newIndex]._enabled = 1;

#if (defined VERBOSITY) && (VERBOSITY >= 2)
	char str_a[100], str_b[100];
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	format_timespec(str_a, &ts);
	format_timespec(str_b, &delayedEvents[newIndex].timestamp);
	printf("%s Timer %d Started with timeout %s\n", str_a, newIndex, str_b);
#endif
	return 0;
}

int APP_FindTimerByEvent(int taskID, long events)
{
	int i;
	for(i=0; i<MAX_DELAYED_EVENTS; i++) {
		if( (delayedEvents[i].taskID == taskID) && ((delayedEvents[i].events & 0x0000FFFF) == (events & 0x0000FFFF)) && (delayedEvents[i]._enabled > 0) ) {
			return i;
		}
	}
	return -1;
}

int APP_ClearTimer(int index)
{
	if(index > MAX_DELAYED_EVENTS) return -1;

	delayedEvents[index]._enabled = 0;

#if (defined VERBOSITY) && (VERBOSITY >= 2)
	struct timespec ts;
	char str[100];
	clock_gettime(CLOCK_REALTIME, &ts);
	format_timespec(str, &ts);
	printf("%s Timer %d Removed\n", str, index);
#endif
	return 0;
}

int APP_ClearTimerByEvent(int taskID, long events)
{
	int i;
	for(i=0; i<MAX_DELAYED_EVENTS; i++) {
		if(delayedEvents[i].taskID == taskID && (delayedEvents[i].events & 0x00FF) == (events & 0x00FF) && delayedEvents[i]._enabled > 0) {
			APP_ClearTimer(i);
			return 0;
		}
	}
	return -1;
}















