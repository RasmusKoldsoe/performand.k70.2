/*
 * wind.h
 *
 *   Created on: Nov 18, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
/*
typedef struct {
	long handle;
	char length;
	char *description;
} attribute_t;
*/

static int Wind_ServiceCount = 4;
static attribute_t Wind_Services[] = {{0x0040, 4, "\tSpeed"},
                                      {0x0044, 4, "\tDirection"},
                                      {0x0048, 4, "\tTemperature"},
                                      {0x0037, 1, "\tBattery"}
                                     };

int Wind_initialize(BLE_Peripheral_t *ble_device);
int Wind_parseData(datagram_t* datagram, int *i);
