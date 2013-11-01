/*
 * GATT_Server.h
 *
 *   Created on: Oct 28, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "../HCI_Parser/HCI_Defs.h"
#include "../ble_device.h"
#include "../dev_tools.h"


int GATT_Enqueue(datagram_t *datagram);

