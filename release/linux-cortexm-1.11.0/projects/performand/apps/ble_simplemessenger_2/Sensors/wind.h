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

static int Wind_ServiceCount = 3;
static attribute_t Wind_Services[] = {{0x002E, 4, "\tTemperature"},
                                      {0x002A, 8, "\tData"},
                                      {0x0026, 1, "\tBattery"}
                                     };

static log_t Wind_SdLog = {.name="wind"};

int Wind_initialize(BLE_Peripheral_t *ble_device);
int Wind_parseData(datagram_t* datagram, int *i);
void Wind_finalize( void );
