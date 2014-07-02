/**
 * @file GS_API_network.c
 *
 * Methods for handling network related functionality 
 */

#include "GS_API.h"
#include "GS_API_private.h"
#include "../AT/AtCmdLib.h"
#include "../hardware/GS_HAL.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/** Private Defines **/
#define DEFAULT_SSID        "GS_NETWORK" ///< Default SSID if not stored in non-volatile storage
#define DEFAULT_CHANNEL     "0" ///< Default channel if not stored in non-volatile storage
#define DEFAULT_SECURITY    "0" ///< Default security mode if not stored in non-volatile storage
#define DEFAULT_DHCP_ENABLE "1" ///< Default dhcp mode if not stored in non-volatile storage

#define CID_COUNT 16 ///< Number of possible connection IDs

/** Private Macros **/
#define CID_INT_TO_HEX(cidInt) (cidIntToHex[cidInt]) ///< Converts integer value to hex ascii character

/** Private Variables **/
static const char cidIntToHex[17] = "0123456789ABCDEF"; ///< Lookup table for integer to hex ascii conversion
static HOST_APP_NETWORK_CONFIG_T apiNetworkConfig; ///< Network configuration structure for default network
static GS_API_DataHandler cidDataHandlers[CID_COUNT]; ///< Array of function pointers for CID data handlers
static char tcpServerClientIp[IP_STRING_LENGTH]; ///< IP address of incoming TCP server client connection
static char tcpServerClientPort[PORT_STRING_LENGTH]; ///< port of incoming TCP server client connection

/** Private Method Declaration **/
static uint8_t gs_api_parseCidStr(uint8_t* cidStr);
static void gs_api_setCidDataHandler(uint8_t cid, GS_API_DataHandler cidDataHandler);
static GS_API_DataHandler gs_api_getCidDataHandler(uint8_t cid);
static bool gs_api_handle_cmd_resp(HOST_APP_MSG_ID_E msg);

/** Public Method Implementation **/
void GS_API_DisconnectNetwork(void){
     AtLibGs_DisAssoc();
}

void GS_API_PrintNetworkInfo(void){
     GS_API_Printf("Network Info");
     GS_API_Printf("SSID: %s", apiNetworkConfig.ssid);
     GS_API_Printf("Channel: %s", apiNetworkConfig.channel);
}

bool GS_API_JoinNetwork() {
     bool joined = false;
     uint8_t security = atoi(apiNetworkConfig.security);
     // Set all the configuration options

     // Check for DHCP Enabled
     if (atoi(apiNetworkConfig.dhcpEnabled)) {
          if (!gs_api_handle_cmd_resp(AtLibGs_DHCPSet(1)))
               return joined;
     } else {
          if (!gs_api_handle_cmd_resp(AtLibGs_DHCPSet(0)))
               return joined;

          if (!gs_api_handle_cmd_resp(
                   AtLibGs_IPSet((int8_t*)apiNetworkConfig.staticIp,
                                 (int8_t*)apiNetworkConfig.subnetMask,
                                 (int8_t*)apiNetworkConfig.gatewayIp)))
               return joined;
     }

     // Check security
     switch (security) {
     case 0:
          // Set all
          // Don't check these since some might not be valid anyway
          AtLibGs_SetPassPhrase((int8_t*)apiNetworkConfig.passphrase);

          AtLibGs_SetWepKey(atoi(apiNetworkConfig.wepId), apiNetworkConfig.wepKey);


          AtLibGs_SetAuthMode(0);
          break;

     case 1:
          // Set none
          break;

     case 2:
          // Set WEP
          if (!gs_api_handle_cmd_resp(
                   AtLibGs_SetWepKey(atoi(apiNetworkConfig.wepId),
                                     apiNetworkConfig.wepKey)))
               return joined;

          if (!gs_api_handle_cmd_resp(AtLibGs_SetAuthMode(0)))
               return joined;
          break;

     case 4:
     case 8:
     case 16:
     case 32:
     case 64:
          // Set WPA
          if (!gs_api_handle_cmd_resp(AtLibGs_SetPassPhrase((int8_t*)apiNetworkConfig.passphrase)))
               return joined;
          break;

     default:
          // nothing
          break;
     }

     // Set the security mode
     if(!gs_api_handle_cmd_resp(AtLibGs_SetSecurity(security)))
          return joined;

     // Set adhoc or infrastructure
     if(!gs_api_handle_cmd_resp(AtLibGs_Mode(atoi(apiNetworkConfig.connType))))
          return joined;


     // Associate
     if(gs_api_handle_cmd_resp(AtLibGs_Assoc((int8_t*)apiNetworkConfig.ssid,(int8_t*)"", (int8_t*)apiNetworkConfig.channel))){
          joined = true;
          // Clear all data handler functions
          memset(cidDataHandlers, 0, sizeof(cidDataHandlers));
     }

     return joined;
}

void GS_API_StartProvisioning(char* provSsid, char* provChannel, char* username,
                              char* password, char* ip, char* subnetMask, char* hostName) {


     // Disable DHCP
     if (!gs_api_handle_cmd_resp(AtLibGs_DHCPSet(0)))
          return;

     // Set Static IP
     if (!gs_api_handle_cmd_resp(
              AtLibGs_IPSet((int8_t*) ip, (int8_t*) subnetMask, (int8_t*) ip)))
          return;

     // Enable Limited AP
     if (!gs_api_handle_cmd_resp(AtLibGs_Mode(2)))
          return;

     // Set SSID and Channel
     if (!gs_api_handle_cmd_resp(
              AtLibGs_Assoc((int8_t*) provSsid, (int8_t*) "",
                            (int8_t*) provChannel)))
          return;

     // Enable DHCP Server
     gs_api_handle_cmd_resp(AtLibGs_SetDHCPServerMode(1));

     // Enable DNS Server
     //if (!gs_api_handle_cmd_resp(AtLibGs_SetDNSServerMode(1, hostName)))
     //     return;

     if (!gs_api_handle_cmd_resp(AtLibGs_BData (1)))
		return;
     // Enable Provisioning
     //if (!gs_api_handle_cmd_resp(AtLibGs_StartProv(username, password)))
     //     return;

     //GS_API_Printf("Prov Up SSID:%s", provSsid);
     //GS_API_Printf("%s/gsclient.html", ip);

     /* Reset the receive buffer */
     AtLib_FlushRxBuffer();

}

bool GS_API_PollProvisioningResponse(void){
     if(AtLib_ResponseHandleNoBlock() != HOST_APP_MSG_ID_NONE){
          // Get the provisioning data
          HOST_APP_NETWORK_CONFIG_T networkConfig;
          memset(&networkConfig, 0, sizeof(networkConfig));

          if (AtLib_ParseProvisionResponse(&networkConfig)) {
               // Print the Provisioning Network Options
               GS_API_Printf(networkConfig.ssid);
               GS_API_Printf(networkConfig.channel);
               GS_API_Printf(networkConfig.mode);
               GS_API_Printf(networkConfig.security);
               GS_API_Printf(networkConfig.dhcpEnabled);
               GS_API_Printf(networkConfig.passphrase);
               GS_API_Printf(networkConfig.wepKey);
               GS_API_Printf(networkConfig.wepId);
               GS_API_Printf(networkConfig.connType);
               GS_API_Printf(networkConfig.staticIp);
               GS_API_Printf(networkConfig.subnetMask);
               GS_API_Printf(networkConfig.gatewayIp);
               GS_API_Printf(networkConfig.autoDnsEnabled);
               GS_API_Printf(networkConfig.primaryDns);
               GS_API_Printf(networkConfig.secondaryDns);

               // Save the network configuration to non volatile storage 
               memcpy(&apiNetworkConfig, &networkConfig, sizeof(apiNetworkConfig));
               //gs_api_writeNetworkConfig();

               return true;
          }
     }
     return false;
}

void GS_API_StopProvisioning(void){
     // Web client can't be shut off, so just reset the device
     gs_api_handle_cmd_resp(AtLibGs_Reset());     
}

bool GS_API_IsAssociated(void){
     if (!gs_api_handle_cmd_resp(AtLibGs_WlanConnStat()))
          return false;

     return AtLib_ParseWlanConnStat();
}

uint8_t GS_API_CreateUdpServerConnection(char* port, GS_API_DataHandler cidDataHandler){
     uint8_t cidStr[] = " ";
     uint16_t cid = GS_API_INVALID_CID;

     if(!gs_api_handle_cmd_resp(AtLibGs_UdpServer_Start((int8_t*)port)))
          return cid;

     if(AtLib_ParseUdpServerStartResponse(cidStr)){
          // need to convert from ASCII to numeric
          cid = gs_api_parseCidStr(cidStr);
          if(cid != GS_API_INVALID_CID){
               cidDataHandlers[cid] = cidDataHandler;
          }
     }

     return cid;
}

uint8_t GS_API_CreateUdpClientConnection(char* serverIp, char* serverPort, char* localPort, GS_API_DataHandler cidDataHandler){
     uint8_t cidStr[] = " ";
     uint16_t cid = GS_API_INVALID_CID;

     if(!gs_api_handle_cmd_resp(AtLibGs_UdpClientStart((int8_t*) serverIp, (int8_t*) serverPort, (int8_t*) localPort)))
          return cid;

     if(AtLib_ParseUdpServerStartResponse(cidStr)){
          // need to convert from ASCII to numeric
          cid = gs_api_parseCidStr(cidStr);
          if(cid != GS_API_INVALID_CID){
               cidDataHandlers[cid] = cidDataHandler;
          }
     }

     return cid;
}

uint8_t GS_API_CreateTcpServerConnection(char* port, GS_API_DataHandler cidDataHandler){
     uint8_t cidStr[] = " ";
     uint16_t cid = GS_API_INVALID_CID;

     if(!gs_api_handle_cmd_resp(AtLibGs_TcpServer_Start((int8_t*)port))) {
          return cid;
	}
   
     if(AtLib_ParseTcpServerStartResponse(cidStr)){
          // need to convert from ASCII to numeric
          cid = gs_api_parseCidStr(cidStr);
          if(cid != GS_API_INVALID_CID){
               cidDataHandlers[cid] = cidDataHandler;
         }
     }
  
     return cid;
}


uint8_t GS_API_CreateTcpClientConnection(char* serverIp, char* serverPort, GS_API_DataHandler cidDataHandler){
     uint8_t cidStr[] = " ";
     uint16_t cid = GS_API_INVALID_CID;

     if(!gs_api_handle_cmd_resp(AtLibGs_TcpClientStart((int8_t*) serverIp, (int8_t*) serverPort))) {
	  printf("ERROR: WIFI TCP CLIENT SETUP ERROR.\n");
          return cid;
	}

     if(AtLib_ParseTcpServerStartResponse(cidStr)){
          // need to convert from ASCII to numeric
          cid = gs_api_parseCidStr(cidStr);
          if(cid != GS_API_INVALID_CID){
               cidDataHandlers[cid] = cidDataHandler;
          }
     }

     return cid;
}

bool GS_API_SendUdpClientData(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength){
     return AtLib_BulkDataTransfer(CID_INT_TO_HEX(cid), dataBuffer, dataLength) == HOST_APP_MSG_ID_ESC_CMD_OK;
}

bool GS_API_SendTcpData(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength){
     return AtLib_BulkDataTransfer(CID_INT_TO_HEX(cid), dataBuffer, dataLength) == HOST_APP_MSG_ID_ESC_CMD_OK;
}

bool GS_API_SendUdpServerDataToLastClient(uint8_t cid, uint8_t* dataBuffer, uint16_t dataLength){
     char ipAddress[HOST_APP_RX_IP_MAX_SIZE];
     char port[HOST_APP_RX_PORT_MAX_SIZE];

     // Get the last UDP client information
     AtLib_GetUdpServerClientConnection(ipAddress, port);
     // Send the data to the UDP client
     AtLib_UdpServerBulkDataTransfer(cid, ipAddress, port, dataBuffer, dataLength);
     return true;
}


void GS_API_CloseConnection(uint8_t cid){
     AtLibGs_Close(CID_INT_TO_HEX(cid));
     cidDataHandlers[cid] = NULL;
}

void GS_API_CloseAllConnections(){
     AtLibGs_CloseAll();
     memset(cidDataHandlers, 0, sizeof(cidDataHandlers));
}

void GS_API_CheckForData(void){
     uint8_t rxData;

     /* Read one byte at a time - Use non-blocking call */
     while (GS_HAL_recv(&rxData, 1, 0)) {
          /* Process the received data */
          switch(AtLib_ReceiveDataProcess(rxData)){
          case HOST_APP_MSG_ID_TCP_SERVER_CLIENT_CONNECTION:
          {
               uint8_t cidServerStr[] = " ";
               uint8_t cidClientStr[] = " ";
               uint8_t cidServer = GS_API_INVALID_CID;
               uint8_t cidClient = GS_API_INVALID_CID;

               if(AtLib_ParseTcpServerClientConnection((char *)cidServerStr, (char *)cidClientStr, tcpServerClientIp, tcpServerClientPort)){
                    cidServer = gs_api_parseCidStr(cidServerStr);
                    cidClient = gs_api_parseCidStr(cidClientStr);
                    gs_api_setCidDataHandler(cidClient, gs_api_getCidDataHandler(cidServer));
               }
               GS_API_Printf("TCP Server Client Connection %d, %d, %s, %s\r\n", cidServer, cidClient, tcpServerClientIp, tcpServerClientPort);
          }
          break;

          case HOST_APP_MSG_ID_RESPONSE_TIMEOUT:
          case HOST_APP_MSG_ID_NONE:
               // Do nothing
               break;

          default:
               break;
          }
     }
}

bool GS_API_GetIPAddress(uint8_t* ipAddr){
     if(AtLibGs_WlanConnStat() != HOST_APP_MSG_ID_OK){
          return false;
     }
  
     return AtLib_ParseIpAddress((char*)ipAddr);
}
  

/** Private Method Implementation **/

/**
   @brief AtCmdLib calls this function for all incoming data

   @param cid Incoming data connection ID
   @param rxData Incoming data value
*/
void App_ProcessIncomingData(uint8_t cid, uint8_t rxData) {
     // Check for and call handler for incoming CID and Data
     if(cid < CID_COUNT && cidDataHandlers[cid])
          cidDataHandlers[cid](cid, rxData);
     else
          GS_API_Printf("RX Data with no handler for cid %d\r\n", cid);
}

/**
   @brief Reads the network configuration from non-volatile storage
   @private
*/
void gs_api_readNetworkConfig(void) {
     // Try to read the saved network information
    // if (!GS_HAL_read_data(NETWORK_SAVE_ADDRESS, (void*) &apiNetworkConfig,
    //                       sizeof(apiNetworkConfig))) {
          GS_API_Printf("No Saved Network");
          // Set default data here
          memcpy(apiNetworkConfig.ssid, DEFAULT_SSID, sizeof(DEFAULT_SSID));
          memcpy(apiNetworkConfig.channel, DEFAULT_CHANNEL,
                 sizeof(DEFAULT_CHANNEL));
          memcpy(apiNetworkConfig.dhcpEnabled, DEFAULT_DHCP_ENABLE,
                 sizeof(DEFAULT_DHCP_ENABLE));
          memcpy(apiNetworkConfig.security, DEFAULT_SECURITY,
                 sizeof(DEFAULT_SECURITY));
     //}
}

/**
   @brief Writes the current network configuration to non-volatile storage
   @private
*/
void gs_api_writeNetworkConfig(void) {
     // Try to save the network information
     //if (!GS_HAL_store_data(NETWORK_SAVE_ADDRESS, (void*) &apiNetworkConfig,
     //                       sizeof(apiNetworkConfig))) {
          GS_API_Printf("Network save failed");
     //}
}

/**
   @brief Retrieves the data handler for the connection ID
   @private
   @param cid Connection ID to retrieve the data handler function pointer for 
   @return Data Handler function pointer for CID, or null if it doesn't exist
*/
static GS_API_DataHandler gs_api_getCidDataHandler(uint8_t cid){
     if(cid < CID_COUNT){
          return cidDataHandlers[cid];
     } else {
          return NULL;
     }
}

/**
   @brief Sets the data handler for the connection ID
   @private
   @param cid Connection ID to set the data handler function pointer for 
   @param cidDataHandler Function pointer for handling CID data
*/
static void gs_api_setCidDataHandler(uint8_t cid, GS_API_DataHandler cidDataHandler){
     if(cid < CID_COUNT){
          cidDataHandlers[cid] = cidDataHandler;
     }
}

/**
   @brief Converts a cid in ascii to an integer
   
   For example, converts 'e' or 'E' to 14
   @private
   @param cidStr CID in ascii
   @return CID as an integer, or GS_API_INVALID_CID if it couldn't be converted
*/
static uint8_t gs_api_parseCidStr(uint8_t* cidStr){
     int8_t cid = GS_API_INVALID_CID;

     printf("cidStr: %s\n", (char *)cidStr);

     //if(sscanf((char*)cidStr, "%x", &cid)){
     if((cid = atoi((char *)cidStr)) && *cidStr == '0') { 
          if(cid >= CID_COUNT){
               cid = GS_API_INVALID_CID;
          }
     }
      printf("CID num: %d\n",cid);
     return cid;
}

/**
   @brief Checks for a OK response to a command
   @private
   @param msg Message returned by command sent to Gainspan module
   @return true if command response was OK, false if not
*/
static bool gs_api_handle_cmd_resp(HOST_APP_MSG_ID_E msg) {

     if (msg != HOST_APP_MSG_ID_OK) {
          GS_API_Printf("CMD ERR %d", msg);
          return false;
     } else {
          return true;
     }

}
