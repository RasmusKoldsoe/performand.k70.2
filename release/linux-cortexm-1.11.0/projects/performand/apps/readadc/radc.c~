/*
 *  ioctl.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/* 
 * device specifics, such as ioctl numbers and the
 * major device file. 
 */
#include "kinetis_adc.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

#define DEVICE_FILE_NAME 	"/dev/adc"

ioctl_kinetisadc_disable_adc(int file_desc, unsigned long arg)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_DISABLE_ADC, arg);

	if (ret_val < 0) {
		printf("ioctl_kinetisadc_disable_adc failed:%d\n", ret_val);
		//exit(-1);
	}
}

ioctl_kinetisadc_enable_adc(int file_desc, unsigned long arg)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_ENABLE_ADC, arg);

	if (ret_val < 0) {
		printf("ioctl_kinetisadc_enable_adc failed:%d\n", ret_val);
		//exit(-1);
	}
}

ioctl_kinetisadc_measure(int file_desc, struct kinetis_adc_request *req)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_MEASURE, req);

	if (ret_val < 0) {
		printf("ioctl_kinetisadc_measure failed: %d\n", ret_val);
		exit(-1);
	}

	
}

main()
{
	int file_desc, ret_val, i;
	struct kinetis_adc_request adc_req;

	file_desc = open(DEVICE_FILE_NAME, 0);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	//ioctl_kinetisadc_enable_adc(file_desc, 0);
	
	adc_req.adc_module=0;
	for(i=0;i<4;i++) {
		adc_req.channel_id=i;
		ioctl_kinetisadc_measure(file_desc, &adc_req);
		printf("Value [Ch%d]; %d\n", adc_req.channel_id, adc_req.result);
	}

	//ioctl_kinetisadc_disable_adc(file_desc, 1);

	close(file_desc);
}
