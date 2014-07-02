/*
 * speedLog.h
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

static int  Log_ServiceCount = 2;
static attribute_t Log_Services[] = {{0x002F, 2, "\tPeriod"},
                                     {0x002A, 1, "\tBattery"}
                                    };

int Log_initialize(BLE_Peripheral_t *ble_device);
int Log_parseData(datagram_t* datagram, int *i);
