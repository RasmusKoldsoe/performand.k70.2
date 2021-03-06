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
#include "COM_Parser/COM_Parser.h"
#include "HCI_Parser/HCI_Parser.h"
#include "../Common/GPIO/gpio_api.h"

char BLE_SensorsHdl[] = {0x78, 0xc5, 0xe5, 0xa0, 0x14, 0x12, 0x00, 0x4D, // MAC (6 bytes) + Service Hdl (2 bytes)
                         0xBC, 0x6A, 0x29, 0xAB, 0x18, 0xD8, 0x00, 0x48}; 

void register_sig_handler();
void sigint_handler(int sig);

int stop = 0;

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


void run_app(BLE_Central_t *bleCentral)
{
	sleep(1);
	datagram_t datagram;
	BLE_Peripheral_t *dev;
// Init Device
	memset(&datagram, 0, sizeof(&datagram));
	get_GAP_DeviceInit(&datagram);
	enqueue(&bleCentral->txQueue, &datagram);
	sleep(1);

	if( !gpio_setValue(LED_IND3, 1) ) {
		printf("ERROR: Setting gpio port value high\n");
	}

// Establish Link
	int i, j;
	memset(&datagram, 0, sizeof(datagram));
	for(i=0; i < sizeof(BLE_SensorsHdl) / (MAC_ADDR_SIZE + SERVICE_HDL_SIZE); i++)
	{
		char* MAC_ptr = BLE_SensorsHdl + i*(MAC_ADDR_SIZE + SERVICE_HDL_SIZE);
		char* HDL_ptr = BLE_SensorsHdl + MAC_ADDR_SIZE + i * (MAC_ADDR_SIZE + SERVICE_HDL_SIZE);

		get_GAP_EstablishLinkRequest(&datagram, MAC_ptr);
		enqueue(&bleCentral->txQueue, &datagram);
		sleep(2);

		dev = findDeviceByMAC(bleCentral, MAC_ptr);
		if(dev->_connected) {
			memset(&datagram, 0, sizeof(datagram));
			char d[] = {0x00, 0x01};
			long hdl = (*HDL_ptr)<<8 + *(HDL_ptr+1);
			get_GATT_WriteCharValue(&datagram, dev->connHandle, hdl, d, sizeof(d));
			enqueue(&bleCentral->txQueue, &datagram);
		}
	}

// Wait for kill signal here
	while(!stop)
		usleep(500000);

// Sign off services and terminate
	for(i=0; i < sizeof(BLE_SensorsHdl) / (MAC_ADDR_SIZE + SERVICE_HDL_SIZE); i++)
	{
		char* MAC_ptr = BLE_SensorsHdl + i*(MAC_ADDR_SIZE + SERVICE_HDL_SIZE);
		char* HDL_ptr = BLE_SensorsHdl + MAC_ADDR_SIZE + i * (MAC_ADDR_SIZE + SERVICE_HDL_SIZE);

		dev = findDeviceByMAC(bleCentral, MAC_ptr);
		if(dev->_connected) {
			memset(&datagram, 0, sizeof(datagram));
			char d[] = {0x00, 0x00};
			long hdl = (*HDL_ptr)*256 + *(HDL_ptr+1);
			get_GATT_WriteCharValue(&datagram, dev->connHandle, hdl, d, sizeof(d));
			enqueue(&bleCentral->txQueue, &datagram);
			sleep(1);

			memset(&datagram, 0, sizeof(datagram));
			get_GAP_TerminateLinkRequest(&datagram, dev->connHandle);
			enqueue(&bleCentral->txQueue, &datagram);
			sleep(1);
		}
	}

	if( !gpio_setValue(LED_IND3, 0) ) {
		printf("ERROR: Setting gpio port value low\n");
	}

	bleCentral->_run = 0;
}




int main (void)
{
	if( !reset_BLE(BLE_0_RESET) )
		return -1;
	if( !reset_LED(LED_IND3) )
		return -1;
	debug(2, "Device is reset successfully\n");

	int rt, wt, hci;
	pthread_t read_thread, write_thread, HCI_thread;
	pthread_attr_t thread_attr;
	BLE_Central_t bleCentral; memset(&bleCentral, 0, sizeof(BLE_Central_t));

	bleCentral.port = "/dev/ttyS3"; // "/dev/ttyS4"
	bleCentral._run = 1;
	bleCentral.rxQueue = queueCreate();
	bleCentral.txQueue = queueCreate();
	bleCentral.fd = open_serial(bleCentral.port, O_RDWR);
	if(bleCentral.fd < 0){
		printf("ERROR Opening port: %d\n", bleCentral.fd);
		return -1;
	}
	initNetworkStat();

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

	hci = pthread_create( &HCI_thread, &thread_attr, HCIParser, (void *)&bleCentral);
	if( hci ) {
	  printf("ERROR HCI thread: Return code from pthread_create(): %d\n", hci);
	  return -1;
	}

	while(bleCentral._run) {
		run_app(&bleCentral);
	}

	pthread_join(hci, NULL);
	pthread_join(rt, NULL);
	pthread_join(wt, NULL);

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
	stop = 1;
}
