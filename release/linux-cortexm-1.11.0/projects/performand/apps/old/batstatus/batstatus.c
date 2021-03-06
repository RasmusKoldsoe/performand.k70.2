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

	return ret_val;
}

ioctl_kinetisadc_enable_adc(int file_desc, unsigned long arg)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_ENABLE_ADC, arg);

	//if (ret_val < 0) {
		//printf("ioctl_kinetisadc_enable_adc failed:%d\n", ret_val);
		ret_val =-1;
	//}

	return ret_val;
}

ioctl_kinetisadc_measure(int file_desc, struct kinetis_adc_request *req)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_MEASURE, req);

	if (ret_val < 0) {
		printf("ioctl_kinetisadc_measure failed: %d\n", ret_val);
		//exit(-1);
	}

	return ret_val;	
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

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_ENABLE_ADC, 0);
		
	adc_req.adc_module=0;
	adc_req.channel_id=3;

	ioctl_kinetisadc_measure(file_desc, &adc_req);

	printf("%d\n", (int)adc_req.result);

	ret_val = ioctl(file_desc, IOCTL_KINETISADC_DISABLE_ADC, 0);

	close(file_desc);
}
