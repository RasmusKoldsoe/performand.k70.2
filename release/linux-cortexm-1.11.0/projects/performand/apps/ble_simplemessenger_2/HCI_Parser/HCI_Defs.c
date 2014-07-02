/*
 * HCI_Defs.c
 *
 *   Created on: Oct 28, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include "HCI_Defs.h"

char* getSuccessString(char status, int esg){
	static char sStr[57];
	if(esg == 0) { // HCI Ext
		switch(status) {
		case HCI_SUCCESS:
			sprintf(sStr, "Success");
			break;
		case HCI_ERR_UNKNOWN_HCI_CMD:
			sprintf(sStr, "Unknown HCI Command");
			break;
		case HCI_ERR_UNKNOWN_CONN_ID:
			sprintf(sStr, "Unknown connection identifier");
			break;
		case HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VAL:
			sprintf(sStr, "Unsupported feature or parameter value");
			break;
		case HCI_ERR_INVALID_HCI_CMD_PARAMS:
			sprintf(sStr, "Invalid HCI Command Parameters");
			break;
		case HCI_ERR_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES:
			sprintf(sStr, "Remote device terminated connection due to low resources");
			break;
		case HCI_ERR_REMOTE_DEVICE_TERM_CONN_POWER_OFF:
			sprintf(sStr, "Remote device terminated connection due to power off");
			break;
		case HCI_ERR_CONN_TERM_BY_LOCAL_HOST:
			sprintf(sStr, "Connection terminated by local host");
			break;
		case HCI_ERR_REPEATED_ATTEMPTS:
			sprintf(sStr, "Repeated Attempts");
			break;
		default:
			sprintf(sStr, "Unknown HCI Ext Status 0x%02X", (unsigned int)status & 0xFF);
			break;
		}
	}
	else {
		switch(status) {
		case HCI_SUCCESS:
			sprintf(sStr, "Success");
			break;
		case HCI_ERR_UNKNOWN_HCI_CMD:
			sprintf(sStr, "Failure");
			break;
		case HCI_ERR_UNKNOWN_CONN_ID:
			sprintf(sStr, "Invalid parameter");
			break;
		case HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VAL:
			sprintf(sStr, "Already performing that task!");
			break;
		case HCI_ERR_INVALID_HCI_CMD_PARAMS:
			sprintf(sStr, "Not setup properly to perform that task");
			break;
		case HCI_ERR_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES:
			sprintf(sStr, "Can't perform function when not in connection");
			break;
		case HCI_ERR_REMOTE_DEVICE_TERM_CONN_POWER_OFF:
			sprintf(sStr, "Can't perform function due to device power off");
			break;
		case HCI_ERR_CONN_TERM_BY_LOCAL_HOST:
			sprintf(sStr, "Waiting");
			break;
		case HCI_ERR_RESERVED2:
			sprintf(sStr, "Connection was not accepted");
			break;
		default:
			sprintf(sStr, "Unknown Status 0x%02X", (unsigned int)status & 0xFF);
			break;
		}
	}
	return sStr;
}

char* getTerminateString(char reason) {
	static char tStr[31];

	switch (reason) {
	case GAP_LL_SUPERVISION_TIMEOUT:
		sprintf(tStr, "Supervision timeout");
		break;
	case GAP_LL_PEER_REQUESTED_TERM:
		sprintf(tStr, "Peer requested termination");
		break;
	case GAP_LL_HOST_REQUESTED_TERM:
		sprintf(tStr, "Host requested termination");
		break;
	case GAP_LL_FAILED_TO_ESTABLISH:
		sprintf(tStr, "Failed to establish connection");
		break;
	default:
		sprintf(tStr, "0x%02X", (int)reason & 0xFF);
	}

	return tStr;
}



int get_GAP_DeviceInit(datagram_t *datagram)
{
	datagram->type = Command;
	datagram->opcode = GAP_DeviceInit;
	datagram->data_length = 38;

	datagram->data[0] = 0x08;
	datagram->data[1] = 0x05;
	datagram->data[34] = 0x01;

	return 0;
}

int get_GAP_EstablishLinkRequest(datagram_t *datagram, char *MAC)
{
	datagram->type = Command;
	datagram->opcode = GAP_EstablishLinkRequest;
	datagram->data_length = 9;

	datagram->data[0] = Disable; // High Duty Cycle
	datagram->data[1] = Disable; // White List
	datagram->data[2] = 0x00;    // Address type Peer = Public

	int i;
	for(i=8; i>=3; i--) {
		datagram->data[i] = *MAC++;
	}
	return 0;
}

int get_GAP_TerminateLinkRequest(datagram_t *datagram, long connHandle)
{
	datagram->type = Command;
	datagram->opcode = GAP_TerminateLinkRequest;
	datagram->data_length = 3;

	datagram->data[0] = (char)connHandle;
	datagram->data[1] = (char)(connHandle >> 8);
	datagram->data[2] = (char)GAP_LL_PEER_REQUESTED_TERM;
	return 0;
}

int get_GATT_WriteCharValue(datagram_t *datagram, long connHandle, long handle, char *data, int length)
{
	datagram->type = Command;
	datagram->opcode = GATT_WriteCharValue;
	datagram->data_length = 4 + length;

	int i=0;
	datagram->data[i++] = (char)connHandle;
	datagram->data[i++] = (char)(connHandle >> 8);
	datagram->data[i++] = (char) handle;
	datagram->data[i++] = (char)(handle >> 8);

	int j;
	for(j=length-1; j>=0; j--) {
		datagram->data[i++] = data[j];
	}
	return 0;
}

int get_GATT_ReadCharValue( datagram_t *datagram, long connHandle, long handle) {
	datagram->type = Command;
	datagram->opcode = GATT_ReadCharValue;
	datagram->data_length = 4;

	int i=0;
	datagram->data[i++] = (char)connHandle;
	datagram->data[i++] = (char)(connHandle >> 8);
	datagram->data[i++] = (char) handle;
	datagram->data[i++] = (char)(handle >> 8);

	return 0;
}

int get_GATT_Handle_By_UUID(datagram_t *datagram, long connHandle, short UUID)
{
	datagram->type = Command;
	datagram->opcode = GATT_DiscCharsByUUID;
	datagram->data_length = 8;

	short start_hdl = 0x0000, end_hdl = 0xFFFF;
	int i=0;
	datagram->data[i++] = (char) connHandle;
	datagram->data[i++] = (char)(connHandle >> 8);
	datagram->data[i++] = (char) start_hdl;
	datagram->data[i++] = (char)(start_hdl >> 8);
	datagram->data[i++] = (char) end_hdl;
	datagram->data[i++] = (char)(end_hdl >> 8);
	datagram->data[i++] = (char) UUID;
	datagram->data[i++] = (char)(UUID >> 8);
}
