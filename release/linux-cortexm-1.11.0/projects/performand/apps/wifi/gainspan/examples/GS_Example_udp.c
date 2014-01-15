/**
 * @file GS_Example_udp.c
 *
 * Method implementation for UDP server and client task examples
 */

#include "GS_Examples.h"
#include "../API/GS_API.h"
#include "../hardware/GS_HAL.h"
#include <stdio.h>
#include <string.h>


/** Private Defines **/
#define UDP_SERVER_PORT  "44444" ///< Port UDP server will use
#define UDP_CLIENT_LOCAL_PORT  "55555" ///< Local port UDP client will use
#define UDP_CLIENT_SEND_INTERVAL 1000 ///< Time to wait between sending temperature
#define UDP_CLIENT_DATA_STR  "TEMPERATURE: %3dC\r\n" ///< Formatted string for sending temperature
#define START_UDP_CLIENT_STR "STARTUDPCLIENT" ///< Start UDP client command string
#define STOP_UDP_CLIENT_STR  "STOPUDPCLIENT" ///< Stop UDP client command string
#define UDP_COMMAND_ACK_STR  "OK\r\n" ///< Command recognized message
#define UDP_COMMAND_NAK_STR  "NOT OK\r\n" ///< Unknown command message


typedef enum{
     UDP_NO_CMD,
     START_UDP_CLIENT,
     STOP_UDP_CLIENT,
     UNKNOWN_CMD
} UDP_SERVER_COMMANDS;

/** Private Variables **/
static uint8_t udpServerCID = GS_API_INVALID_CID; ///< Connection ID for UDP server
static uint8_t udpClientCID = GS_API_INVALID_CID; ///< Connection ID for UDP client
static uint8_t line[256], command[25], value1[25], value2[25], value3[25]; ///< Commmand parsing strings 
static uint8_t argumentCount; ///< Number of arguments passed into command handler
static uint8_t currentPos; ///< Current position in line buffer
static uint32_t udpClientSentTime; ///< Timestamp of last sent UDP client temperature packet
static UDP_SERVER_COMMANDS commandToHandle = UDP_NO_CMD; ///< Current command to handle

/** Private Method Declarations **/
static UDP_SERVER_COMMANDS gs_example_scan_for_commands(void);
static void gs_example_handle_udp_server_data(uint8_t cid, uint8_t data);
static void gs_example_handle_udp_client_data(uint8_t cid, uint8_t data);
static void gs_example_send_udp_client_data(void);
static void gs_example_handle_command(void);
static void gs_example_start_udp_client(uint8_t* serverIp, uint8_t* serverPort);

/** Public Method Implementations **/
void GS_Example_start_udp_task(void){
     // Reset all variables 
     udpServerCID = GS_API_INVALID_CID;
     udpClientCID = GS_API_INVALID_CID;
     commandToHandle = UDP_NO_CMD;

     // Create a UDP server connection 
     udpServerCID = GS_API_CreateUdpServerConnection(UDP_SERVER_PORT, gs_example_handle_udp_server_data);
     if(udpServerCID != GS_API_INVALID_CID){
          GS_API_Printf("UDP PORT: %s", UDP_SERVER_PORT);
     }
}

void GS_Example_stop_udp_task(void){
     // Close the server connection
     if(udpServerCID != GS_API_INVALID_CID){
          GS_API_CloseConnection(udpServerCID);
     }

     // Close the client connection
     if(udpClientCID != GS_API_INVALID_CID){
          GS_API_CloseConnection(udpClientCID);
     }
}

void GS_Example_udp_task(void){
     // Check for a valid server connection
     if(udpServerCID != GS_API_INVALID_CID){
          // Check for incoming data
          GS_API_CheckForData();
          // Check for a command to handle
          gs_example_handle_command();
          // Send UDP client data
          gs_example_send_udp_client_data();
     }
}

/** Private Method Implementations **/

/**
   @brief Starts UDP client to connect to specified UDP server
   
   @private
   @param serverIp IP address of UDP server to connect to
   @param serverPort Port of UDP server to connect to
*/
static void gs_example_start_udp_client(uint8_t* serverIp, uint8_t* serverPort){
     // Check for an existing UDP client connection 
     if(udpClientCID != GS_API_INVALID_CID){
          GS_API_Printf("UDP Client Already Started");
          return;
     }

     // We're not connected, so lets create a UDP client connection
     udpClientCID = GS_API_CreateUdpClientConnection((char*)serverIp, (char*)serverPort, UDP_CLIENT_LOCAL_PORT, gs_example_handle_udp_client_data);

     // Now lets send the data for the first time
     gs_example_send_udp_client_data();
}

/**
   @brief Handles incoming data for the UDP Client

   Does nothing, as we are using the UDP client connection as outgoing only

   @private
   @param cid Connection ID the data is coming from
   @param data Byte of data coming from the UDP connection
*/
static void gs_example_handle_udp_client_data(uint8_t cid, uint8_t data){

}

/**
   @brief Parses command sent to TCP server

   This method will act upon the command sent to the UDP server
*/
static void gs_example_handle_command(void){
     switch(commandToHandle){
     case UDP_NO_CMD:
          // Nothing to do
          break;

     case START_UDP_CLIENT:
          // Start UDP client
          //GS_API_SendUdpServerDataToLastClient(udpServerCID, UDP_COMMAND_ACK_STR, (sizeof(UDP_COMMAND_ACK_STR) - 1));
          gs_example_start_udp_client(value1, value2);
          break;

     case STOP_UDP_CLIENT:
          // Stop UDP client
          //GS_API_SendUdpServerDataToLastClient(udpServerCID, UDP_COMMAND_ACK_STR, (sizeof(UDP_COMMAND_ACK_STR) - 1));
          GS_API_CloseConnection(udpClientCID);
          udpClientCID = GS_API_INVALID_CID;
          break;

     default:
          // Unknown command, print it out
          //GS_API_SendUdpServerDataToLastClient(udpServerCID, UDP_COMMAND_NAK_STR, (sizeof(UDP_COMMAND_NAK_STR) - 1));
          GS_API_Printf((char*)line);
          break;
     }
     // Clear the command so we don't act on it again 
     commandToHandle = UDP_NO_CMD;
}

/**
   @brief Sends periodic temperature using TCP client connection
   @private
*/
static void gs_example_send_udp_client_data(){
     static uint8_t temperatureStr[] = UDP_CLIENT_DATA_STR;
     // Check for a valid connection and timer interval
     if(udpClientCID != GS_API_INVALID_CID && MSTimerDelta(udpClientSentTime) > UDP_CLIENT_SEND_INTERVAL ){
          // Format string with temperature
          sprintf((char*)temperatureStr, UDP_CLIENT_DATA_STR, GS_Example_GetTemperature());
          // Send the temperature string
          GS_API_SendUdpClientData(udpClientCID, temperatureStr, sizeof(UDP_CLIENT_DATA_STR) - 1);
          // Reset the sending interval
          udpClientSentTime = MSTimerGet();
     }
}

/**
   @brief Handles incoming data for the UDP Server
   
   This method handles incoming data the UDP Server connection.
   It will look for an end of line character, parse the line sent and 
   look for a valid incoming command.
   @private
   @param cid Connection ID the data is coming from
   @param data Byte of data coming from the UDP connection
*/
static void gs_example_handle_udp_server_data(uint8_t cid, uint8_t data){
     // Check that the incoming data is coming from the expected connection ID
     if(udpServerCID != cid){
          GS_API_Printf("UDP Server CID didn't match");
          return;
     }

     // Save the data to the line buffer
     line[currentPos++] = data;
     if(data == '\n'){
          // null terminate the string so sscanf works
          line[currentPos++] = 0;
          // Scan the line for commands and arguments
          argumentCount = sscanf((char*)line, "%s %s %s %s",(char*) command, (char*)value1, (char*)value2, (char*)value3);
          if(argumentCount > 0){
               // Scan for commands
               commandToHandle = gs_example_scan_for_commands();
          }
          currentPos = 0;
     }
}

/**
   @brief Scans command string for valid command
   @param
*/
static UDP_SERVER_COMMANDS gs_example_scan_for_commands(void){
     if(argumentCount == 3 && strstr((char*)command, START_UDP_CLIENT_STR)){
          return START_UDP_CLIENT;
     }

     if(argumentCount == 1 && strstr((char*)command, STOP_UDP_CLIENT_STR)){
          return STOP_UDP_CLIENT;
     }

     return UNKNOWN_CMD;
}
