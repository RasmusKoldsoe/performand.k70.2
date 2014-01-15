/**
 * @file GS_Example_sleep.c
 *
 * Public method implementation for sleep example
 */
#include <stdint.h>
#include "../API/GS_API.h"
#include "../Hardware/GS_HAL.h"


void GS_Example_sleep(uint32_t sleepMS){
     // Put device in sleep 
     GS_API_GotoDeepSleep();

     // Do something while device is sleeping 
     MSTimerDelay(sleepMS);

     // Wake device up 
     GS_API_WakeupDeepSleep();
}

