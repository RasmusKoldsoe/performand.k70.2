/*
 * HCI_Parser.h
 *
 *   Created on: Sep 20, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef COMPARSER_H_
#define COMPARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "HCI_Defs.h"
#include "../ble_device.h"
#include "../dev_tools.h"
#include "../Queue/Queue.h"
#include "../GATT_Device/GATT_Server.h"
#include "../../Common/GPIO/gpio_api.h"
#include "../../Common/MemoryMapping/memory_map.h"

void *HCIParser(void *_bleCentral);

#endif /* COMPARSER_H_ */
