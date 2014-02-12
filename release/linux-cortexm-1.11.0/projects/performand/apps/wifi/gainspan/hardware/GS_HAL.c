/**
 * @file GS_HAL.c
 *
 * Method implementations for the Hardware Abstraction Layer that are platform independent 
 */

#include "GS_HAL.h"

/** Public Method Implementation **/
void GS_HAL_init(char *port){
  GS_HAL_uart_set_comm_port(port);
  MSTimerInit();
}

void GS_HAL_Close(char *port){
  GS_HAL_uart_close_comm_port(port);
}



