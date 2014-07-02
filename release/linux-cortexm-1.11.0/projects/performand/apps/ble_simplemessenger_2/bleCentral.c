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
#include "APP.h"
#include "../Common/GPIO/gpio_api.h"

#include "Sensors/speedLog.h"
#include "Sensors/wind.h"
#include "Sensors/compass.h"

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



int main (int argc, char *argv[])
{
	if( argc < 2 ){
		debug(0, "Need serial port as input param:\n\t%s /dev/ttyS4\n", argv[0]);
		return -1;
	}

	if( !reset_BLE(BLE_0_RESET) )
		return -1;
	if( !reset_LED(LED_IND3) )
		return -1;
	debug(2, "Device is reset successfully\n");

	int rt, wt;
	pthread_t read_thread, write_thread;
	pthread_attr_t thread_attr;
	BLE_Central_t bleCentral;
	memset(&bleCentral, 0, sizeof(BLE_Central_t));

	bleCentral.port = argv[1]; //"/dev/ttyS4"; // "/dev/ttyS3"
	bleCentral._run = 1;
	bleCentral.rxQueue = queueCreate();
	bleCentral.txQueue = queueCreate();
	BLE_Peripheral_t dev[] = { {.ID = 0x1 << 16, // bitwise id - lowest id available
	                            .connHandle = -1,
	                            .connMAC = {0x78, 0xC5, 0xE5, 0xA0, 0x14, 0x12},
		                        .serviceHdls = Log_Services,
	                            .serviceHdlsCount = Log_ServiceCount,
	                            .initialize = Log_initialize,
	                            .parseDataCB = Log_parseData,
		                        ._connected = 0,
		                        ._defined = 1
		                       },
		                       {.ID = 0x1 << 17,
	                            .connHandle = -1,
	                            //.connMAC = {0xBC, 0x6A, 0x29, 0xAB, 0x18, 0xD8}, // Kiel
	                            .connMAC = {0xBC, 0x6A, 0x29, 0xAB, 0x17, 0x60},
		                        .serviceHdls = Compass_Services,
	                            .serviceHdlsCount = Compass_ServiceCount,
	                            .initialize = Compass_initialize,
	                            .parseDataCB = Compass_parseData,
		                        ._connected = 0,
		                        ._defined = 1
		                       },
		                       {.ID = 0x1 << 18,
	                            .connHandle = -1,
	                            .connMAC = {0x90, 0x59, 0xaf, 0x09, 0xd5, 0x9f},
		                        .serviceHdls = Wind_Services,
	                            .serviceHdlsCount = Wind_ServiceCount,
	                            .initialize = Wind_initialize,
							    .parseDataCB = Wind_parseData,
		                        ._connected = 0,
		                        ._defined = 1
		                       }
		                     };
		
	bleCentral.devices = dev;
	bleCentral.fd = open_serial(bleCentral.port, O_RDWR);
	if(bleCentral.fd < 0){
		printf("ERROR Opening port: %d\n", bleCentral.fd);
		return -1;
	}
	initNetworkStat();

/*	devices |= bleCentral->devices[0].ID; // Log
	devices |= bleCentral->devices[1].ID; // Compass
	devices |= bleCentral->devices[2].ID; // Wind sensor*/ 

	//APP_Init(&bleCentral, bleCentral.devices[0].ID|bleCentral.devices[1].ID | bleCentral.devices[2].ID); // Kiel
	APP_Init(&bleCentral, bleCentral.devices[1].ID); // Debug

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

	usleep(500000);
	APP_SetEvent(APP_TaskID, APP_STARTUP_EVENT);
	debug(1, "Program set up - Run app\n");
	while(bleCentral._run) {
		APP_Run();
	}

	pthread_join(rt, NULL);
	pthread_join(wt, NULL);

	APP_Exit();
	close_serial(bleCentral.fd);

	printNetworkStat();
	printf("tx Queue max count: %d\n", bleCentral.txQueue.maxCount);
	printf("rx Queue max count: %d\n", bleCentral.rxQueue.maxCount);
	queueDestroy(&bleCentral.txQueue);
	queueDestroy(&bleCentral.rxQueue);

	printf("Delayed Events Max Count: %d\n", _delayedEvents_MaxCount); 

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
	APP_SetEvent(APP_TaskID, APP_SHUTDOWN_EVENT);
}
