/**
 * @file GS_uart_STM32.c
 *
 * Method implmentation for sending data to module over UART on the STM32 platform 
 */

#include <linux/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

/** Private Defines **/
typedef char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned int uint32_t;

#define GAINSPAN_USART   /dev/ttyS3

/** Private Variables **/
static volatile uint8_t rxBuffer[256];
static volatile uint8_t rxHead;
static volatile uint8_t rxTail;

static volatile uint8_t tty_fd;

/** Private Method Declaration **/
static uint16_t ReadRxBuffer(uint8_t* rxBuf, uint16_t numToRead);

/** Public Method Implementation **/
void GS_HAL_uart_set_comm_port(char *port) {

	struct termios tio;
//	fd_set rdset;

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag &= ~OPOST; //0;
	tio.c_cflag &= CREAD|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // (ICANON | ECHO | ECHOE);

	tty_fd=open(port, O_RDWR);
	cfsetospeed(&tio,B115200);
	cfsetispeed(&tio,B115200);
	tcsetattr(tty_fd,TCSANOW,&tio);

	// Reset RX buffer variables
	rxTail = 0;
	rxHead = 0;
}

void GS_HAL_uart_close_comm_port(char *port) {
	close(tty_fd);
}

static void print_char_array(char *buff, int length, int offset)
{
	int i;
	for(i=offset; i<length; i++) {
		printf("%c", buff[i] & 0xff);
	}
	printf("\n");
}

int GS_HAL_uart_send(const uint8_t* txBuf, uint16_t numToWrite) {
	int i=0;     
	//while(numToWrite--){
          // Check for empty TX buffer
          //while (USART_GetFlagStatus(GAINSPAN_USART, USART_FLAG_TXE) == RESET); 
          if ((i=write(tty_fd, txBuf,numToWrite)) < numToWrite) {
		printf("ERROR writing to port.\n");
		return -1;
	}
	//print_char_array(txBuf, numToWrite, 0);
	  	   
     //}
	return 0;

}

uint16_t GS_HAL_uart_recv(uint8_t* rxBuf, uint16_t numToRead, int block) {
    if (block) {
          // Blocking mode, so don't return until we read all the bytes requested
          uint16_t bytesRead = 0;
          // Keep getting data if we have a number of bytes to fetch 
          while (numToRead) {
               bytesRead = read(tty_fd,rxBuf,numToRead);//ReadRxBuffer(rxBuf, numToRead);
               if (bytesRead) {
                    rxBuf += bytesRead;
                    numToRead -= bytesRead;
               }
          }
	
          return bytesRead;
     } else {
          // Non-blocking mode, just read what is available in buffer 
          return read(tty_fd,rxBuf,numToRead);
     }
     

}

/** Private Method Implementation **/
/**
   @brief Interrupt handler for UART RX data
   
   This interrupt handler is set into the vector table by startupStm32l1xxMd.s
   Places bytes received into RX buffer for later retrival
   @private
*/
void USART2_IRQHandler(void) {
     // Check for value in RX buffer
 /*    if(USART_GetITStatus(GAINSPAN_USART, USART_IT_RXNE)){
          // Check for room in circular RX buffer and save the RX byte
          if((rxHead + 1) != rxTail){
               rxBuffer[rxHead++] = USART_ReceiveData(GAINSPAN_USART);
          }
          USART_ClearITPendingBit(GAINSPAN_USART, USART_IT_RXNE);
     }
*/
}


/**
   @brief Reads bytes from RX buffer
   @private 
   @param rxBuf Location to store data read from Rx buffer
   @param numToRead Number of bytes to read from Rx buffer
   @return number of bytes actually read from Rx buffer
*/
static uint16_t ReadRxBuffer(uint8_t* rxBuf, uint16_t numToRead){
 /*    uint16_t bytesRead = 0;
	
     while(rxTail != rxHead && bytesRead < numToRead){
          *rxBuf++ = rxBuffer[rxTail++];
          bytesRead++;
     }	
	*/
     return 0;//bytesRead;

}
