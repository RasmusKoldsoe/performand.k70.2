
#include <stdio.h>
#include "gpio_api.h"


int main (void)
{
	printf("GPIO TOGGLE PORT TEST PROGRAM\n");

//init GPIO e.g.:
	if(!gpio_export(BLE_0_RESET)) {
		printf("ERROR: Exporting gpio port: %d\n", BLE_0_RESET);
		return 0;
	}

//set direction 1: in 0:out
	if(!gpio_setDirection(BLE_0_RESET, GPIO_DIR_IN)) {
		printf("ERROR: Set gpio port direction.\n");
		return 0;
	}

//get value:
	int val = gpio_getValue(BLE_0_RESET);
	if(val < 0) {
		printf("ERROR: Read gpio port state.\n");
		return 0;
	}
	printf("GPIO Port (%d) is set %d.\n", BLE_0_RESET, val);

//set direction 1: in 0:out
	if(!gpio_setDirection(BLE_0_RESET, GPIO_DIR_OUT)) {
		printf("ERROR: Set gpio port direction.\n");
		return 0;
	}

//set value
	val++;
	val %= 2;
	printf("GPIO port (%d) set value %d\n.", BLE_0_RESET, val);
	if( !gpio_setValue(BLE_0_RESET, val) )
		printf("ERROR: Exporting gpio port.");

	return 0;
}
