/**
 * @file GS_API.h
 *
 * Public methods for controlling the Gainspan Module using the Serial to Wifi protocol
 */

#ifndef GS_API_H_
#define GS_API_H_

#include <stdint.h>
#include <stdbool.h>

#define GS_API_INVALID_CID  0xFF  ///< Invalid CID returned when TCP or UDP connection fails
#define GS_API_IP_STR_LENGTH  16  ///< Length of string needed to hold longest IP value (ie. 255.255.255.255)

typedef void (*GS_API_DataHandler)(uint8_t cid, uint8_t byte); ///< TCP and UDP connection data handling function pointer. cid is Connection ID of connection sending the data, byte is the data received.


/**
   @brief Checks for working communication.

   Sends a command to the GS module and looks for valid response

   @return true if communication is working
 */
bool GS_API_CommWorking(void);

/**
   @brief Prints module information.

   Prints APP Version, GEPS Version and WLAN Version information
 */
void GS_API_PrintModuleInformation(void);

/**
   @brief Updates module firmware.

   Configures the module to connect to the specified HTML server 
   and retrive the firmware binary files at the specified paths.
   
   Example for files located here:
   WLAN - http://192.168.1.50:8080/fw/wlan.bin
   APP0 - http://192.168.1.50:8080/fw/s2w-app1.bin
   APP1 - http://192.168.1.50:8080/fw/s2w-app2.bin
   Set parameters to:
   updateIp   = "192.168.1.50"
   updatePort = "8080"
   wlanPath   = "/fw/wlan.bin"
   app0Path   = "/fw/s2w-app1.bin"
   app1Path   = "/fw/s2w-app2.bin"
   
   @param updateIp IP address of the HTML server
   @param updatePort Port to connect to the HTML server
   @param wlanPath Path to WLAN firmware location
   @param app0Path Path to APP0 firmware location
   @param app1Path Path to APP1 firmware location
 */
bool GS_API_UpdateFirmware(char* updateIp, char* updatePort, char* wlanPath, char* app0Path, char* app1Path);

/**
   @brief Puts module in low power sleep.

   In deep sleep mode memory is retained
 */
void GS_API_GotoDeepSleep(void);

/**
   @brief Wakes module up from sleep.

   Sends a command until module wakes up
 */
void GS_API_WakeupDeepSleep(void);

/**
   @brief Starts Provisioning mode to allow network configuration.

   Configures module in Limited AP mode based on provided settings. This will 
   create a wireless network that can be joined by a PC to configure the default network.
   When created go to http://(ip)/gsclient.html to configure module.
   Must periodically call GS_API_PollProvisioningResponse to check if configuration is complete.

   @param provSsid SSID of the limited AP network to create
   @param provChannel Channel of limited AP network
   @param username Username of login to web config page
   @param password Password of login to web config page
   @param ip IP of the device
   @param subnetMask subnet mask of the network to create
   @param hostName Hostname of the device
 */
void GS_API_StartProvisioning(char* provSsid, char* provChannel, char* username,
		char* password, char* ip, char* subnetMask, char* hostName);

/**
   @brief Checks to see if a user has setup a network using the web config page.

   Must be called periodically after GS_API_StartProvisioning until it returns true
   Saves the network selected by the config page as the default network to join

   @return true if provisioning web config page was completed
 */
bool GS_API_PollProvisioningResponse(void);


/**
    @brief Stops the Limited AP provisioning network.

    Need to call GS_API_Init after this to reset the module and turn off the 
    web config page.
 */
void GS_API_StopProvisioning(void);

/**
   @brief Joins the default saved network

   Joins the network that was saved during provisioning

   @return true if network connection was made
 */
bool GS_API_JoinNetwork(void);

/**
   @brief Prints the default network configuration

   Prints the network configuration that is currently saved.
 */
void GS_API_PrintNetworkInfo(void);

/**
   @brief Disconnects from network 
 */
void GS_API_DisconnectNetwork(void);

/**
   @brief Checks for network association

   @return true if device is attached to a network
 */
bool GS_API_IsAssociated(void);

/**
   @brief Creates a UDP server listening for incoming connections

   The UDP server is setup on the local port as specified, listening for incoming UDP data packets
   
   @param port Local port to listen for UDP data
   @param cidDataHandler Function pointer to handle incoming data for this connection
   @return CID of created connection. If connection fails GS_API_INVALID_CID will be returned
 */
uint8_t GS_API_CreateUdpServerConnection(char* port, GS_API_DataHandler cidDataHandler);

/**
   @brief Creates a UDP client to connect to a remote address

   The UDP client is setup to connect a local port to a remote port at a remote IP address.
   
   @param serverIp The IP address of the UDP server to make a connection to
   @param serverPort Remote port of the UDP server to make a connection to
   @param localPort Local port to send and receive UDP data
   @param cidDataHandler Function pointer to handle incoming data for this connection
   @return CID of created connection. If connection fails GS_API_INVALID_CID will be returned
 */
uint8_t GS_API_CreateUdpClientConnection(char* serverIp, char* serverPort, char* localPort, GS_API_DataHandler cidDataHandler);

/**
   @brief Creates a TCP server listening for incoming connections

   The TCP server is setup on the local port as specified, listening for incoming TCP connections.
   Everytime a new client connects, a new CID will be created to handle that connections data.
   The cidDataHandler function is used for all client connections. Clients can be differentiated by 
   the CID passed into the cidDataHandler function. 
   
   @param port Local port to listen for incoming TCP connections
   @param cidDataHandler Function pointer to handle incoming client data
   @return CID of created connection. If connection fails GS_API_INVALID_CID will be returned
 */
uint8_t GS_API_CreateTcpServerConnection(char* port, GS_API_DataHandler cidDataHandler);

/**
   @brief Creates a TCP client to connect to a remote address.

   The TCP client is setup to connect to a remote port at a remote IP address.
   
   @param serverIp The IP address of the TCP server to make a connection to
   @param serverPort Remote port of the TCP server to make a connection to
   @param cidDataHandler Function pointer to handle incoming data from the TCP server
   @return CID of created connection. If connection fails GS_API_INVALID_CID will be returned
 */
uint8_t GS_API_CreateTcpClientConnection(char* serverIp, char* serverPort, GS_API_DataHandler cidDataHandler);

/**
   @brief Sends a UDP data packet to a client from UDP Server connection

   Sends a UDP data packet using the specified CID to the last UDP client to send data to the server.

   @param cid Connection ID of the UDP Server to send data from
   @param dataBuffer Pointer to data that will be sent
   @param dataLength Number of bytes of data to send
   @return true if data send was a success. false indicates failure and CID should no longer be used
 */
bool GS_API_SendUdpServerDataToLastClient(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength);

/**
   @brief Sends a UDP data packet

   Sends a UDP data packet using the specified CID to the UDP server.

   @param cid Connection ID of the UDP Client to send data to
   @param dataBuffer Pointer to data that will be sent
   @param dataLength Number of bytes of data to send
   @return true if data send was a success. false indicates failure and CID should no longer be used
 */
bool GS_API_SendUdpClientData(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength);

/**
   @brief Sends a TCP data packet

   Sends a TCP data packet using the specified CID. This can be used for both TCP Server and TCP Client
   connections.

   @param cid Connection ID of the TCP conenction to send data to
   @param dataBuffer Pointer to data that will be sent
   @param dataLength Number of bytes of data to send
   @return true if data send was a success. false indicates failure and CID should no longer be used
 */
bool GS_API_SendTcpData(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength);

/**
   @brief Handles incoming data

   Directs incoming data to the GS_API_DataHandler function pointers for each CID. Checks for data
   coming from module. Must be called periodically whenever there is an open TCP or UDP connection.
 */
void GS_API_CheckForData(void);

/**
   @brief Closes specified CID

   Closes the connection ID for a UDP or TCP connection. The connection ID should be discarded or set to GS_API_INVALID_CID after this is called. 
   
   @param cid Connection ID that should be closed.
 */
void GS_API_CloseConnection(uint8_t cid);

/**
   @brief Closes all connections

   All connections on the module will be closed after this is called. Connection IDs should be discared or
   set to GS_API_INVALID_CID.
 */
void GS_API_CloseAllConnections(void);

/**
   @brief Gets the modules IP address

   Retrieves the IP address if the module is associated to a network.
   
   @param ipAddr Pointer to store IP address as a string. Needs to be at least GS_API_IP_STR_LENGTH long.
   @return true if IP address was retrieved, false if ipAddr was not modified
 */
bool GS_API_GetIPAddress(uint8_t* ipAddr);

/**
   @brief Initializes the Gainspan module

   Initializes low level hardware communicatin, syncs module communication, 
   resets module and reads the default network config. Must be called at startup, and to reset module if  
   provisioning mode is cancelled.
 */  
void GS_API_Init(char *port);

/**
   @brief Prints a formatted string

   Prints a formatted string using the platform specific print capabilty, for example an LCD or UART.
   
   @param format String to print with standard format specifiers as placeholders
   @param ... Variable length argument to fill placeholders
 */
//void GS_API_Printf(const char *format, ...);

void GS_API_Close(char *port);

#endif /* GS_API_H_ */
