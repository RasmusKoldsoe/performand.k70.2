/*
 * wind.h
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include "../HCI_Parser/HCI_Defs.h"

static long Wind_serviceHdls[] = {0x0040, 0x0044, 0x0048, 0x0037};
static char* Wind_serviceHdlCharDesc[] = {"Battery", "Speed", "Direction", "Temperature"};
static int Wind_nServiceHdls = 4;

int Wind_initialize(BLE_Peripheral_t *ble_device);
int Wind_parseData(datagram_t* datagram, int *i, char* mm_str);
