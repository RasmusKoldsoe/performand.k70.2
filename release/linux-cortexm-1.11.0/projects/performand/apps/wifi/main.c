#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include "utils/gpio_api.h"

#define FILE_LENGTH 	0x1000	//1KB File emory

#define HOST_APP_DEBUG_ENABLE

/** Private Defines **/
#define PROVISION_SSID "SailBoy2.0" ///< SSID for limited AP network that will be created
#define PROVISION_CHANNEL "8" ///< Channel for limited AP network that will be created
#define PROVISION_USERNAME "performand" ///< Username for web configuration client login
#define PROVISION_PASSWORD "performand" ///< Password for web configuration client login
#define PROVISION_IP  "192.168.10.1" ///< IP address of the module 
#define PROVISION_SUBNET "255.255.255.0" ///< Subnet of the network module will create, devices that join the network will be assigned IP addresses within the subnet of PROVISION_IP
#define PROVISION_HOSTNAME "sailboy" ///< Host name of the module for DNS

#define GS_API_INVALID_CID  0xFF  ///< Invalid CID returned when TCP or UDP connection fails
#define GS_API_IP_STR_LENGTH  16  ///< Length of string needed to hold longest IP value (ie. 255.255.255.255)

/** Private Defines **/
#define TCP_SERVER_PORT "2000" ///< Port TCP Server will use
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

static char tcpServerCID = GS_API_INVALID_CID; ///< Connection ID for TCP server
static char line[256], command[25], value1[25], value2[25], value3[25], value4[25], value5[25]; ///< Command parsing strings
static char argumentCount; ///< Number of arguments passed into command handler
static char currentPos; ///< Current position in the line buffer
static int tcpClientSentTime; ///< Timestamp of last sent TCP Client temperature packet
static int firmwareUpdated; ///< Firmware Update complete flag
static TCP_SERVER_COMMANDS commandToHandle = TCP_NO_CMD; ///< Current command to handle
static char commandCID = GS_API_INVALID_CID; ///< Connection ID last command came from


/** Private Method Declarations **/
static void gs_handle_tcp_server_data(char cid, char data);
static TCP_SERVER_COMMANDS gs_tcp_scan_for_commands(void);
/**
@brief Sends periodic temperature using TCP client connection
@private
*/
static void gs_send_tcp_client_data(){
  //static uint8_t temperatureStr[] = TCP_CLIENT_DATA_STR;
  // Check for a valid connection and timer interval
  //if(tcpClientCID != GS_API_INVALID_CID && MSTimerDelta(tcpClientSentTime) > TCP_CLIENT_SEND_INTERVAL ){
    // Format string with temperature
   
    // Reset sending interval
   //tcpClientSentTime = MSTimerGet();
 // }
  
}


static void gs_handle_tcp_server_data(char cid, char data){
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
			commandToHandle = gs_tcp_scan_for_commands();
			commandCID = cid;
		}
		currentPos = 0;
	}
}

/**
@brief Scans command string for valid command
@param
*/
static TCP_SERVER_COMMANDS gs_tcp_scan_for_commands(void){
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

int main(int argc, char **argv)
{
	char tcpClientCID = 1;
	char buffer[1024];

	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int timeout=0;

	line = (char *)malloc(256);

	printf("GS1500m Daemon v0.1\n");

	if(!gpio_export(WIFI_RESET))
		printf("ERROR: Exporting gpio port.");
	
	//Check if the module is ready.
	if(!gpio_setDirection(WIFI_RESET,1))
		printf("ERROR: Exporting gpio port.");	

	
	if(!gpio_getValue(WIFI_RESET)) {
		printf("ERROR: Wifi not ready.");
		return -1;
	}

	//Reset the module is ready.
	if(!gpio_setDirection(WIFI_RESET,0))
		printf("ERROR: Exporting gpio port.");	
	if(!gpio_setValue(WIFI_RESET,0))
		printf("ERROR: Exporting gpio port.");	

	usleep(500000);

	//Now use port as input again.
	if(!gpio_setDirection(WIFI_RESET,1))
		printf("ERROR: Exporting gpio port.");	

	timeout=0;
	while(!gpio_getValue(WIFI_RESET)) {
		printf("WARN: Wifi not ready.");
		usleep(100000);
		timeout++;
		if(timeout>100) {
			printf("TIMEOUT ERROR: Can't reset Wifi chip.\n");
			return -1;
		}
	}

	// Initialize Gainspan module, print module information and switch to client mode 
        GS_API_Init("/dev/ttyS3");
        GS_API_PrintModuleInformation();
	//GS_API_StopProvisioning();
	//GS_API_DisconnectNetwork();
     	GS_API_StartProvisioning(
			PROVISION_SSID,
			PROVISION_CHANNEL,
			PROVISION_USERNAME,
			PROVISION_PASSWORD,
			PROVISION_IP,
			PROVISION_SUBNET,
			PROVISION_HOSTNAME);
  	
	// Create the TCP Server connection
  	tcpServerCID = GS_API_CreateTcpServerConnection(TCP_SERVER_PORT, gs_handle_tcp_server_data);
	printf("TCP Server CID: %d \n", tcpServerCID);
  	if(tcpServerCID != GS_API_INVALID_CID){
    		printf("TCP PORT: %s", TCP_SERVER_PORT);
  	}

/*	while(1) {
		fp = fopen("gps", "r");
		
		if (fp == NULL)
	  		exit(-1);
		while ((read = getline(&line, &len, fp)) != -1) {		   
			AtLib_BulkDataTransfer(0x31, line, strlen(line));
		}
	
		fclose(fp);
		usleep(500000);
	}
*/
//	while(1)
//	{
	//	usleep(1500000);
	//	printf("**** ****\n",file_memory);

	/*	len = *file_memory;
		len = (len<<8) + *(file_memory+1);

		print_char_array(file_memory, len, 2);

		*file_memory=0;
		*(file_memory+1)=0;

		/* Flush file */
//		memset(file_memory, '\0', sizeof(char)*FILE_LENGTH);

		//printf("**** ****\n",file_memory);
//		gs1500m_sendTCPIPstring(" Z10005Hello\n",12);
//		usleep(1000000);
//	}
	exit(0);
}


