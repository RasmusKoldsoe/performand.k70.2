/*
 * compass.h
 *
 *   Created on: Jan 7, 2014
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

static int  Compass_ServiceCount = 3;
static attribute_t Compass_Services[] = {{0x0036, 6, "\tMagnetometer"},
                                         {0x003A, 6, "\tAccelerometer"},
                                         {0x0026, 1, "\tBattery"},
                                        };

int Compass_initialize(BLE_Peripheral_t *ble_device);
int Compass_parseData(datagram_t* datagram, int *i);
