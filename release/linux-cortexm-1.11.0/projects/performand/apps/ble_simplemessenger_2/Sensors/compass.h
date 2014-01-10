/*
 * compass.h
 *
 *   Created on: Jan 7, 2014
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

static int  Compass_ServiceCount = 4;
static attribute_t Compass_Services[] = {{0x0041, 6, "\tMagnetometer"},
                                         {0x0045, 6, "\tAccelerometer"},
                                         {0x0037, 1, "\tBattery"},
                                         {0x003a, 1, ""}
                                        };

int Compass_initialize(BLE_Peripheral_t *ble_device);
int Compass_parseData(datagram_t* datagram, int *i);
