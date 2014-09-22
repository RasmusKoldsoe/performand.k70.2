/*
 * APP.h
 *
 *   Created on: Oct 29, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef APP_H_
#define APP_H_

#define APP_STARTUP_EVENT          0x0001
#define APP_SHUTDOWN_EVENT         0x0002
#define APP_DEVICE_INIT_DONE_EVENT 0x0004
#define APP_CONNECT_NEXT_DEVICE    0x0008
#define APP_CONNECTION_ESTABLISHED 0x0010
#define APP_CONNECTION_TERMINATED  0x0020
#define APP_CONNECTION_CANCLED     0x0040
#define APP_HCI_ERROR              0x0080

/********************************************************************
 * long events:                                                     *
 * Structured like following                                        *
 * - bytes  0-15: Event ID                                          *
 * - bytes 16-32: Bitwise Connection ID |= 0x1 << (16 + connHandle) *
 ********************************************************************/

extern int APP_TaskID;
int _delayedEvents_MaxCount;

typedef long (*eventHandlerFcn) (int taskID, long events);

extern long HCI_ProcessEvent (int taskID, long events);
extern long GATT_ProcessEvent(int taskID, long events);

extern void APP_Run(void);
extern long APP_ProcessEvent(int taskID, long events);
extern int  APP_Init(BLE_Central_t *b, long dev);
extern void  APP_Exit(void);
extern long APP_SetEvent(int taskID, long events);
extern long APP_GetEvent(int taskID);
extern void APP_ClearEvent(int taskID);
extern int  APP_StartTimer(int taskID, long events, int timeout); //ms
extern int  APP_FindTimerByEvent(int taskID, long events);
extern int  APP_ClearTimer(int index);
extern int  APP_ClearTimerByEvent(int taskID, long events);


#endif /* APP_H_ */
