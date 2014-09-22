/*
 * compass.h
 *
 *   Created on: Jan 7, 2014
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

static int  Compass_ServiceCount = 2;
static attribute_t Compass_Services[] = {{0x002a, 17, "\tCompass Data"},
										 {0x0026,  1, "\tBattery"}
                                        };

static log_t Compass_SdLog = {.name="compass"};

int Compass_initialize(BLE_Peripheral_t *ble_device);
int Compass_parseData(datagram_t* datagram, int *i);
void Compass_finalize( void );
