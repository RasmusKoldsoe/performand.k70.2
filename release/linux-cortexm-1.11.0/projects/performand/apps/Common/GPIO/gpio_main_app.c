//init GPIO e.g.:

     if(!gpio_export(WIFI_RESET))
         printf("ERROR: Exporting gpio port: %d\n", WIFI_RESET);

//set direction 1: in 0:out
     if(!gpio_setDirection(WIFI_RESET,1))
         printf("ERROR: Exporting gpio port.");


//get value:
if(!gpio_getValue(WIFI_RESET)) {
         printf("ERROR: Wifi not ready.");
         return -1;
     }

//set value
     if(!gpio_setValue(WIFI_RESET,0))
         printf("ERROR: Exporting gpio port.");
