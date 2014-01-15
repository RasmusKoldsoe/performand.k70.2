/**
 * @file GS_HAL.h
 *
 * Method definitions for the Hardware Abstraction Layer 
 */

#ifndef GS_HAL_H_
#define GS_HAL_H_

#include <stdint.h>
#include <stdbool.h>

//#define	GS_HAL_printf	printf
#define	GS_API_Printf	printf

#define GS_HAL_send  GS_HAL_uart_send
#define GS_HAL_recv  GS_HAL_uart_recv

/**
   @brief Initializes hardware

   Intializes the hardware. This method is the same for all platforms
 */
void GS_HAL_init(char *port);

void GS_HAL_Close(char *port);



/**
   @brief Sends data using UART to module
   
   This method is platform specific. 
   @param txBuf Data to transmit to the module
   @param numToWrite Number of bytes of data to transmit
 */
void GS_HAL_uart_send(const uint8_t* txBuf, uint16_t numToWrite);

/**
   @brief Reads data using UART from module
   
   This method is platform specific.
   @param rxBuf Data buffer to hold incoming data
   @param numToRead Number of bytes to attempt to read
   @param block Set to true if read should wait until numToRead bytes have been read, 
   false to read only available bytes and return immediately
   @return Number of bytes received from module
 */
uint16_t GS_HAL_uart_recv(uint8_t* rxBuf, uint16_t numToRead, bool block);

/**
   @brief Set UART port to specified baudRate

   This method is platform specific
   @param baudRate Baud Rate UART should be set to
 */
//void GS_HAL_uart_set_comm_port(uint32_t baudRate);
void GS_HAL_uart_set_comm_port(char* port);

/**
   @brief Prints string

   This method is platform specific. The output can be whatever is appropriate for the 
   platform: LCD, spare UART, debug console, etc.
   @param strBuf null terminated string to print
 */
//void GS_HAL_print(char* strBuf);

/**
   @brief Stores data in a non-volatile location
   
   This method is platform specific. Stores data at address in a non-volatile location.
   Example storage: File on SD card, external EEPROM, internal data flash.
   @param address Address to store data to with an offset of 0
   @param writeBuf Data to store
   @param numToWrite Number of bytes to store
   @return true if successfully written to storage
 */
bool GS_HAL_store_data(uint16_t address, uint8_t* writeBuf, uint8_t numToWrite);

/**
   @brief Reads data from a non-volatile location

   This method is platform specific. Method should be able to tell if storage
   is empty or has been written to.
   @param address Address to read data from with an offset of 0
   @param readBuf Data buffer to store read data
   @param numToRead Number of bytes to read
   @return true if data was successfully read from storage. false indicates data has
   not been written yet, or read failure
 */
bool GS_HAL_read_data(uint16_t address, uint8_t* readBuf, uint8_t numToRead);

/**
   @brief Initializes millisecond timer
   
   This method is platform specific. 
 */ 
void MSTimerInit(void);

/**
   @brief Gets current millisecond timer count

   This method is platform specific.
   @return Milliseconds since the timer was initialized
 */
uint32_t MSTimerGet(void);

/**
   @brief Calculates the number of milliseconds between current time and value
   
   This method is platform specific.
   @param start Millisecond value to subtract from current time
   @return Milliseconds elapsed between start value and current time
 */
uint32_t MSTimerDelta(uint32_t start);

/**
   @brief Blocks for specified number of milliseconds
   
   This method is platform specific.
   @param ms Number of milliseconds to wait before returning
 */
void MSTimerDelay(uint32_t ms);

#endif /* GS_HAL_H_ */
