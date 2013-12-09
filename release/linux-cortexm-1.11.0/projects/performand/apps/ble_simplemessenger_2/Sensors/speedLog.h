/*
 * speedLog.h
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include "../HCI_Parser/HCI_Defs.h"

static long Log_serviceHdls[] = {0x004D, 0x0030};
static int  Log_nServiceHdls = 2;

int Log_initialize(BLE_Peripheral_t *ble_device);
int Log_parseData(datagram_t* datagram, int *i, char* mm_str);
