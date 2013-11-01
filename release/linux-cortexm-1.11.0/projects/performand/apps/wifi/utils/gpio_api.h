#ifndef _GPIO_API_H_
#define _GPIO_API_H_

#define WIFI_RESET	38
#define BLE_0_RESET	35
#define GPS_ENABLE	37
#define LED_IND1	64
#define LED_IND2	65
#define LED_IND3	66

int gpio_export(int gpio);
int gpio_setDirection(int gpio, int dir);
int gpio_setValue(int gpio, int value);
int gpio_getValue(int gpio);

#endif /* _GPIO_API_H_ */
