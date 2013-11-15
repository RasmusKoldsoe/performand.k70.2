#ifndef _GPIO_API_H_
#define _GPIO_API_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>


#define WIFI_RESET	38
#define BLE_0_RESET	35
#define GPS_ENABLE	37
#define LED_IND1	64
#define LED_IND2	65
#define LED_IND3	66

#define GPIO_DIR_IN      1
#define GPIO_DIR_OUT     0


int gpio_export(int gpio);
int gpio_setDirection(int gpio, int dir);
int gpio_setValue(int gpio, int value);
int gpio_getValue(int gpio);

#endif /* _GPIO_API_H_ */
