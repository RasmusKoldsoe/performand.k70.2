#include <stdio.h>

typedef struct {
	long connHandle;
	char connMAC[6];
	long serviceHdls[5];
	char _connected;
	char _defined;
} BLE_Peripheral_t;

typedef struct {
	int a;
	BLE_Peripheral_t *devices;
} BLE_Central_t;


BLE_Peripheral_t dev[] = { { .connMAC = {1,2,3,4,5,6}, 
                         .serviceHdls = {1,2,3,4,5},
                         ._connected = 0, ._defined = 1
                       },
                       { .connMAC = {6,5,4,3,2,1}, 
                         .serviceHdls = {2,3,4,5,6},
                         ._connected = 0, ._defined = 1
                       }
                     };

int main(void)
{
	int i,j;
	
	BLE_Central_t bleCentral;

	bleCentral.devices = dev;
	printf("sizeof(dev) = %d\n", sizeof(bleCentral.devices)/sizeof(BLE_Peripheral_t));

	for(j=0; j<2; j++) {
		printf("connHandle = %d\n", bleCentral.devices[j].connHandle);
		printf("connMAC[6] = "); for(i=0; i<6; i++) printf("%02X%c", bleCentral.devices[j].connMAC[i], (i<5)?':':'\n');
	}
	return 0;
}
