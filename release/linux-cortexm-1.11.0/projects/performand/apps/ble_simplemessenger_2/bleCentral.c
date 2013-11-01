/*
 * bleCentral.c
 *
 *   Created on: Sep 12, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/resource.h>

#include "HCI_Parser/HCI_Defs.h"
#include "ble_device.h"
#include "dev_tools.h"
#include "Queue/Queue.h"
#include "NetworkStat/NetworkStatistics.h"
#include "SerialLogic/serialLogic.h"
//#include "COM_Parser/COM_Parser.h"
//#include "HCI_Parser/HCI_Parser.h"
#include "APP.h"
#include "../Common/GPIO/gpio_api.h"

void register_sig_handler();
void sigint_handler(int sig);

int reset_BLE(int pin)
{
//init GPIO:
	if( !gpio_export(pin) ) {
		printf("ERROR: Exporting gpio port: %d\n", pin);
		return 0;
	}
//set direction 1=in 0=out
	if( !gpio_setDirection(pin, GPIO_DIR_OUT) ) {
		printf("ERROR: Set gpio port direction\n");
		return 0;
	}
//set GPIO value low:
	if( !gpio_setValue(pin, 0) ) {
		printf("ERROR: Set gpio port state low\n");
		return 0;
	}
//set GPIO value high:
	if( !gpio_setValue(pin, 1) ) {
		printf("ERROR: Set gpio port state high\n");
		return 0;
	}
// Time between setting low and high is measured to ~900us. Minimum reset_n
// low duration according to datasheet is min 1 us.
	return 1;
}

int reset_LED(int pin)
{
	if( !gpio_export(pin) ) {
		printf("ERROR: Exporting gpio port: %d\n", BLE_0_RESET);
		return 0;
	}

	if( !gpio_setDirection(pin, GPIO_DIR_OUT) ) {
		printf("ERROR: Set gpio port direction.\n");
		return 0;
	}

	if( !gpio_setValue(pin, 0) ) {
		printf("ERROR: Exporting gpio port.");
		return 0;
	}
	return 1;
}

BLE_Central_t bleCentral;
int main (void)
{
	if( !reset_BLE(BLE_0_RESET) )
		return -1;
	if( !reset_LED(LED_IND3) )
		return -1;
	debug(2, "Device is reset successfully\n");

	int rt, wt;
	pthread_t read_thread, write_thread;
	pthread_attr_t thread_attr;
	 
	memset(&bleCentral, 0, sizeof(BLE_Central_t));

	bleCentral.port = "/dev/ttyS3"; // "/dev/ttyS4"
	bleCentral._run = 1;
	bleCentral.rxQueue = queueCreate();
	bleCentral.txQueue = queueCreate();
	BLE_Peripheral_t dev[] = { {.connHandle = 0, 
	                           .connMAC = {0x78, 0xC5, 0xE5, 0xA0, 0x14, 0x12},
		                       .serviceHDLS = {0x004D, 0x0, 0x0, 0x0, 0x0},
		                       ._connected = 0,
		                       ._defined = 1 
		                      },
		                      {.connHandle = 0, 
	                           .connMAC = {0xBC, 0x6A, 0x29, 0xAB, 0x18, 0xD8},
		                       .serviceHDLS = {0x0048, 0x0, 0x0, 0x0, 0x0},
		                       ._connected = 0,
		                       ._defined = 1
		                       }
		                     };
/*
		long connHandle;
		char connMAC[6];
		long serviceHDLS[5];
		char _connected;
		char _defined;
*/
	bleCentral.devices = dev;
	bleCentral.fd = open_serial(bleCentral.port, O_RDWR);
	if(bleCentral.fd < 0){
		printf("ERROR Opening port: %d\n", bleCentral.fd);
		return -1;
	}
	initNetworkStat();
	APP_Init(&bleCentral);

	//Prepare mapped mem.
	bleCentral.mapped_mem = (char*) mm_prepare_mapped_mem("ble");
	bleCentral.rt_count = 0;

	//Register signal handler.
	register_sig_handler(); 

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	rt = pthread_create( &read_thread, &thread_attr, read_serial, (void *)&bleCentral);
	if( rt ) {
	  printf("ERROR read thread: Return code from pthread_create(): %d\n", rt);
	  return -1;
	}

	wt = pthread_create( &write_thread, &thread_attr, write_serial, (void *)&bleCentral);
	if( wt ) {
	  printf("ERROR write thread: Return code from pthread_create(): %d\n", wt);
	  return -1;
	}

	APP_SetEvent(APP_TaskID, APP_STARTUP_EVENT);
printf("Program set up - Run app\n");
	while(bleCentral._run) {
		APP_Run();
	}

	pthread_join(rt, NULL);
	pthread_join(wt, NULL);

	APP_Exit();
	queueDestroy(&bleCentral.txQueue);
	queueDestroy(&bleCentral.rxQueue);
	close_serial(bleCentral.fd);

	printNetworkStat();

	printf("Exiting main thread\n");
	pthread_exit(NULL);
}

void register_sig_handler()
{
	struct sigaction sia;

	memset(&sia, 0, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(1);
	} 
}

void sigint_handler(int sig)
{
	bleCentral._run = 0;
}
