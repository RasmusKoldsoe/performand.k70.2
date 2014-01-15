/**
@file GS_Example_tcp.c

Method implementation for TCP server and client task examples
*/
#include "GS_Examples.h"
#include "../API/GS_API.h"
#include "../hardware/GS_HAL.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/** Private Defines **/
#define TCP_SERVER_PORT  "22222" ///< Port TCP Server will use
#define TCP_CLIENT_SEND_INTERVAL 1000 ///< Time to wait between sending temperature
#define TCP_MAX_CLIENT_CONNECTION (15) ///< Maximum possible TCP Server client connections (16 available - 1 TCP server connection)
#define TCP_CLIENT_DATA_STR  "Temperature: %3dC\r\n" ///< Formatted string for sending Temperature
#define START_TCP_CLIENT_STR "STARTTCPCLIENT" ///< Start TCP client command string
#define STOP_TCP_CLIENT_STR  "STOPTCPCLIENT"  ///< Stop TCP client command string
#define TCP_FIRMWARE_UPGRADE_STR "FWUPDATE" ///< Firmware Update command string
#define TCP_COMMAND_ACK_STR  "OK\r\n" ///< Command recognized message
#define TCP_COMMAND_NAK_STR  "NOT OK\r\n" ///< Unknown command message

typedef enum{
  TCP_NO_CMD,
  START_TCP_CLIENT,
  STOP_TCP_CLIENT,
  TCP_FIRMWARE_UPGRADE,
  TCP_UNKNOWN_CMD
} TCP_SERVER_COMMANDS;

/** Private Variables **/
static uint8_t tcpServerCID = GS_API_INVALID_CID; ///< Connection ID for TCP server
static uint8_t tcpServerClientCids[TCP_MAX_CLIENT_CONNECTION]; ///< Connection IDs for clients that made a connection to the TCP server
static uint8_t tcpClientCID = GS_API_INVALID_CID; ///< Connection ID for TCP client
static uint8_t line[256], command[25], value1[25], value2[25], value3[25], value4[25], value5[25]; ///< Command parsing strings
static uint8_t argumentCount; ///< Number of arguments passed into command handler
static uint8_t currentPos; ///< Current position in the line buffer
static uint32_t tcpClientSentTime; ///< Timestamp of last sent TCP Client temperature packet
static bool firmwareUpdated; ///< Firmware Update complete flag
static TCP_SERVER_COMMANDS commandToHandle = TCP_NO_CMD; ///< Current command to handle
static uint8_t commandCID = GS_API_INVALID_CID; ///< Connection ID last command came from

/** Private Method Declarations **/
static void gs_example_handle_tcp_server_data(uint8_t cid, uint8_t data);
static void gs_example_start_tcp_client(uint8_t* serverIp, uint8_t* serverPort);
static void gs_example_handle_tcp_client_data(uint8_t cid, uint8_t data);
static TCP_SERVER_COMMANDS gs_example_tcp_scan_for_commands(void);
static void gs_example_send_tcp_client_data(void);
static void gs_example_tcp_handle_command(void);


/** Public Method Implmentation **/
void GS_Example_start_tcp_task(void){
  // Reset all variables 
  memset(tcpServerClientCids, GS_API_INVALID_CID, sizeof(tcpServerClientCids));
  tcpServerCID = GS_API_INVALID_CID;
  commandToHandle = TCP_NO_CMD;
  commandCID = GS_API_INVALID_CID;
  firmwareUpdated = false;
  
  
  // Create the TCP Server connection
  tcpServerCID = GS_API_CreateTcpServerConnection(TCP_SERVER_PORT, gs_example_handle_tcp_server_data);
  if(tcpServerCID != GS_API_INVALID_CID){
    GS_API_Printf("TCP PORT: %s", TCP_SERVER_PORT);
  }
}

void GS_Example_stop_tcp_task(void){
  uint8_t index;
  // Close the server CID
  if(tcpServerCID != GS_API_INVALID_CID){
    GS_API_CloseConnection(tcpServerCID);
  }
  
  // Close the client CID
  if(tcpClientCID != GS_API_INVALID_CID){
    GS_API_CloseConnection(tcpClientCID);
  }
  
  // Close any server client connections
  for(index = 0; index < TCP_MAX_CLIENT_CONNECTION; index++){
    if(tcpServerClientCids[index] != GS_API_INVALID_CID){
      GS_API_CloseConnection(tcpServerClientCids[index]);
    }
  }
  
  
}

bool GS_Example_tcp_task(void){
  // Check for  a valid server connection
  if(tcpServerCID != GS_API_INVALID_CID){
    // Check for incoming data
    GS_API_CheckForData();
    // Check for commands to handle
    gs_example_tcp_handle_command();
    // Send TCP client data
    gs_example_send_tcp_client_data();
    return firmwareUpdated;
  }
  return true;
}

/** Private Method Implementations **/

/**
@brief Handles incoming data for the TCP Server

This method handles incoming data for all TCP Server client connections
It will look for an end of line character, parse the line sent and 
look for a valid incoming command.
@private
@param cid Connection ID the data is coming from
@param data Byte of data coming from the TCP connection
*/
static void gs_example_handle_tcp_server_data(uint8_t cid, uint8_t data){
  // Save the data to the line buffer
  line[currentPos++] = data;
  
  // Check for a newline character
  if(data == '\n'){
    // null terminate the string so sscanf works
    line[currentPos++] = 0;
    // Scan the line for commands and arguments
    argumentCount = sscanf((char*)line, "%s %s %s %s %s %s",(char*) command, (char*)value1, (char*)value2, (char*)value3, (char*)value4, (char*)value5);
    if(argumentCount > 0){
      // Scan for commands
      commandToHandle = gs_example_tcp_scan_for_commands();
      commandCID = cid;
    }
    currentPos = 0;
  }
}

/**
@brief Starts a TCP client to connect to the specified server

Tries to connect using TCP to the IP and port passed in.
@private
@param serverIp IP of TCP server to connect to
@param serverPort port of TCP server to connect to
*/
static void gs_example_start_tcp_client(uint8_t* serverIp, uint8_t* serverPort){
  
  // Check to see if we are already connected
  if(tcpClientCID != GS_API_INVALID_CID){
    GS_API_Printf("TCP Client Already Started");
    return;
  }
  
  // We're not connected, so lets create a TCP client connection
  tcpClientCID = GS_API_CreateTcpClientConnection((char*)serverIp, (char*)serverPort, gs_example_handle_tcp_client_data);
  
  // Now lets send data for the first time
  gs_example_send_tcp_client_data();
}

/**
@brief Handles incoming data for the TCP Client

Does nothing, as we are using the TCP client connection as outgoing only

@private
@param cid Connection ID the data is coming from
@param data Byte of data coming from the TCP connection
*/
static void gs_example_handle_tcp_client_data(uint8_t cid, uint8_t data){
}

/**
@brief Parses command sent to TCP server

This method will act upon the command sent to the TCP server, and send a response to the
client that made the request to notify if the command was recognized or not.

@private
*/
static void gs_example_tcp_handle_command(void){
  switch(commandToHandle){
  case TCP_NO_CMD:
    // Nothing to do
    break;
    
  case START_TCP_CLIENT:
    // Acknowledge command and start TCP client
    GS_API_SendTcpData(commandCID, TCP_COMMAND_ACK_STR, (sizeof(TCP_COMMAND_ACK_STR) - 1));
    gs_example_start_tcp_client(value1, value2);
    break;
    
  case STOP_TCP_CLIENT:
    // Acknowledge command and stop TCP client
    GS_API_SendTcpData(commandCID, TCP_COMMAND_ACK_STR, (sizeof(TCP_COMMAND_ACK_STR) - 1));
    GS_API_CloseConnection(tcpClientCID);
    tcpClientCID = GS_API_INVALID_CID;
    break;
    
  case TCP_FIRMWARE_UPGRADE:
    // Acknowledge command and attempt firmware update
    GS_API_SendTcpData(commandCID, TCP_COMMAND_ACK_STR, (sizeof(TCP_COMMAND_ACK_STR) - 1));
    if(GS_API_UpdateFirmware((char*)value1, (char*)value2, (char*)value3, (char*)value4, (char*)value5)){
      GS_API_Printf("Firmware Updated");
      GS_Example_stop_tcp_task();
      firmwareUpdated = true;
    } else {
      GS_API_Printf("Firmware Update Failed");
      firmwareUpdated = true;
    }
    break;
    
  default:
    // Unknown command, send not ok and print the unrecognized command
    GS_API_Printf((char*)line);
    GS_API_SendTcpData(commandCID, TCP_COMMAND_NAK_STR, (sizeof(TCP_COMMAND_NAK_STR) - 1));
    break;
  }
  
  // Clear the command so we don't act on it again
  commandToHandle = TCP_NO_CMD;
}

/**
@brief Sends periodic temperature using TCP client connection
@private
*/
static void gs_example_send_tcp_client_data(){
  static uint8_t temperatureStr[] = TCP_CLIENT_DATA_STR;
  // Check for a valid connection and timer interval
  if(tcpClientCID != GS_API_INVALID_CID && MSTimerDelta(tcpClientSentTime) > TCP_CLIENT_SEND_INTERVAL ){
    // Format string with temperature
    sprintf((char*)temperatureStr, TCP_CLIENT_DATA_STR, GS_Example_GetTemperature());
    // Try to send the temperature string
    if(!GS_API_SendTcpData(tcpClientCID, temperatureStr, sizeof(TCP_CLIENT_DATA_STR) - 1)){
      // Sending failed, disable this connection ID
      tcpClientCID = GS_API_INVALID_CID;
      GS_API_Printf("Send TCP Data failed");
    }
    // Reset sending interval
    tcpClientSentTime = MSTimerGet();
  }
  
}

/**
@brief Scans command string for valid command
@param
*/
static TCP_SERVER_COMMANDS gs_example_tcp_scan_for_commands(void){
  if(argumentCount == 3 && strstr((char*)command, START_TCP_CLIENT_STR)){
    return START_TCP_CLIENT;
  }
  
  if(argumentCount == 1 && strstr((char*)command, STOP_TCP_CLIENT_STR)){
    return STOP_TCP_CLIENT;
  }
  
  if(argumentCount == 6 && strstr((char*)command, TCP_FIRMWARE_UPGRADE_STR)){
    return TCP_FIRMWARE_UPGRADE;
  }
  
  // Command unknown
  return TCP_UNKNOWN_CMD;
}
