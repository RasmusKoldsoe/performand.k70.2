/**
 * @file GS_Examples.h
 *
 * Public methods to show how to use the Gainspan Serial2Wifi module. 
 */

#ifndef GS_EXAMPLES_H_
#define GS_EXAMPLES_H_

#include <stdint.h>
#include <stdbool.h>

#define GS_Example_GetTemperature LM75_ReadTemp ///< Platform specific method to read a temperature for client demo

/**
   @brief Gets the current temperature in C

   This is a platform specific function. Returns some sort of temperature. In platforms
   without a temperature sensor, this can return random values. The actual function to be
   called should be set by the GS_Example_GetTemperature define
   @return Temperature in celsius
*/
uint16_t GS_Example_GetTemperature(void);


/**
   @brief Starts provisioning web client
   
   Starts the provisioning web client with these settings:
   - SSID: GS_PROV
   - Channel: 6
   - Username: gainspan
   - Password: gainspan
   - IP: 192.168.141.1
   - Subnet: 255.255.255.0
   - Hostname: gsprov

   To use the provisioning web client after calling this function,
   join the wireless network GS_PROV, and then go to 
   http://192.168.141.1/gsclient.html with a browser and enter the 
   username and password "gainspan" for both.

   GS_Example_provisioning_task needs to be called periodically to 
   check if the web client has been configured. This can be cancelled
   by calling GS_Example_stop_provisioning_task
*/
void GS_Example_start_provisioning_task(void);

/**
   @brief Monitors the provisioning web client

   This must be called periodically to check if the provisioning
   web client has been configured.
   @return true if client has been configured and provisioning is finished
*/
bool GS_Example_provisioning_task(void);

/**
   @brief Stops the provisioning web client
*/
void GS_Example_stop_provisioning_task(void);

/**
   @brief Joins default network

   This example prints out the current network information
   and then tries to join the network
   @return true if network join was successful, false if network join failed
*/
bool GS_Example_join_network(void);

/**
   @brief Attempts to update the firmware

   This example shows how to call the update firmware function with these settings:
   - Server IP: 192.168.1.10
   - Server Port: 8080
   - WLAN firmware path: /wlan.bin
   - APP0 firmware path: /s2w-app1.bin
   - APP1 firmware path: /s2w-app2.bin

   The firmware update will be tried immediately, so the module should be preconfigured on a network with
   a http server at the specified IP and port hosting the firmware files.
   @return true if firmware update succeeded, false if it failed
*/ 
bool GS_Example_update_firmware(void);

/**
   @brief Puts the module into deep sleep

   This shows an example of putting the module into deep sleep for 
   the requested time and then waking it back up. In a real application 
   something useful should be done while sleeping.

   @param sleepMS Time in milliseconds to put the module to sleep
*/
void GS_Example_sleep(uint32_t sleepMS);

/**
   @brief Starts a TCP server task
   
   This example starts a TCP server listening at port 22222
   After connecting to this port these commands are available:
   - STARTTCPCLIENT ip port : Creates a TCP client connection to ip:port
   - STOPTCPCLIENT : Shuts down TCP client connection
   - FWUPDATE ip port wlanPath app0Path app1Path : Starts firmware update at specified ip:port http server
   with specified paths for firmware files
*/
void GS_Example_start_tcp_task(void);

/**
   @brief Stops the TCP server task
   
   Closes the TCP server connection, and the TCP client connection if one is open
*/
void GS_Example_stop_tcp_task(void);

/**
   @brief Handles TCP server and client tasks

   This example checks for incoming data, handles commands passed to the TCP server
   and sends periodic data using the TCP client connection if one is active

   @return true if firmware was updated, so the device was reset
*/
bool GS_Example_tcp_task(void);

/**
   @brief Starts the UDP server task
   
   This example starts a UDP server listening at port 44444
   After connecting to ths port these commands are available:
   - STARTUDPCLIENT ip port : Creates a UDP client connection to ip:port
   - STOPUDPCLIENT : Shuts down UDP client connection
*/
void GS_Example_start_udp_task(void);

/**
   @brief Stops the UDP server task
   
   Closes the UDP server connection, and the UDP client connection if one is open
*/
void GS_Example_stop_udp_task(void);

/**
   @brief Handles UDP server and client tasks

   This example checks for incoming data, handles commands passed to the UDP server
   and sends periodic data using the UDP client connection if one is active
*/
void GS_Example_udp_task(void);

#endif /* GS_EXAMPLES_H_ */
