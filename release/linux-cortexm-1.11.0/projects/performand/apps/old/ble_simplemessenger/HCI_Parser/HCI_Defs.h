/*
 * HCI_Defs.h
 *
 *   Created on: Sep 5, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef BLE_DEFINES_H_
#define BLE_DEFINES_H_

#include <stdio.h>
#include <sys/time.h>


#define STD_DATAGRAM_SIZE 64
#define MAX_DATAGRAM_QUEUE 10

enum EnableDisable {
	Disable = 0x00,
	Enable = 0x01
};

enum PackageType {
	Command = 0x01,
	Event = 0x04
};

enum HCICmdOpcode {
	GATT_ReadUsingCharUUID     = 0xFD84,
	GATT_DiscCharsByUUID       = 0xFD88,
	GATT_ReadCharValue         = 0xFD8A,
	GATT_ReadMultiCharValues   = 0xFD8E,
	GATT_WriteCharValue        = 0xFD92,
	GAP_DeviceInit             = 0xFE00,
	GAP_DeviceDiscoveryRequest = 0xFE04,
	GAP_DeviceDiscoveryCancel  = 0xFE05,
	GAP_EstablishLinkRequest   = 0xFE09,
	GAP_TerminateLinkRequest   = 0xFE0A,
	GAP_SetParam               = 0xFE30,
	GAP_GetParam               = 0xFE31
};

enum HCI_EvtOpCode {
	ATT_ErrorRsp                   = 0x0501,
	ATT_WriteRsp                   = 0x0513,
	ATT_HandleValueNotification    = 0x051B,
	GAP_DeviceInitDone             = 0x0600,
	GAP_DeviceDiscoveryDone        = 0x0601,
	GAP_EstablishLink              = 0x0605,
	GAP_TerminateLink              = 0x0606,
	GAP_HCI_ExtentionCommandStatus = 0x067F
};

enum HCI_EventCode {
	HCI_NumberOfCompletePacketsEvent = 0x0013,
	HCI_LE_ExtEvent = 0x00FF
};

enum GAP_TerminationReason {
	GAP_LL_SUPERVISION_TIMEOUT     = 0x08,
	GAP_LL_PEER_REQUESTED_TERM     = 0x13,
	GAP_LL_HOST_REQUESTED_TERM     = 0x16,
	GAP_LL_FAILED_TO_ESTABLISH     = 0x3E
};

enum HCIExt_StatusCodes {
	HCI_SUCCESS = 0x00,
	HCI_ERR_UNKNOWN_CONN_ID = 0x02,
	HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VAL = 0x11
};

typedef struct {
	struct timeval timestamp;
	char type;
	long opcode;
	unsigned int data_length;
	char data[ STD_DATAGRAM_SIZE - 4 ];
} datagram_t;

// Datagram structures
char* getSuccessString(char status);
char* getTerminateString(char reason);
int get_GAP_DeviceInit(datagram_t *datagram);
int get_GAP_EstablishLinkRequest(datagram_t *datagram, char *MAC);
int get_GAP_TerminateLinkRequest(datagram_t *datagram, long connHandle);
int get_GATT_WriteCharValue(datagram_t *datagram, long connHandle, long handle, char *data, int length);


#endif /* BLE_DEFINES_H_ */
