/**
 * @file GS_Example_firmware_update.c
 *
 * Method implmentation for firmware update example
 */

#include <stdbool.h>
#include "../API/GS_API.h"

/** Private Defines **/
#define FWUP_SERVER_IP   "192.168.1.10" ///< IP address of HTTP server with firmware update
#define FWUP_SERVER_PORT "8080" ///< Port of HTTP server with firmware update
#define FWUP_WLAN_PATH   "/wlan.bin" ///< Path on the HTTP server for the WLAN firmware
#define FWUP_APP0_PATH   "/s2w-app1.bin" ///< Path on the HTTP server for the APP0 firmware
#define FWUP_APP1_PATH   "/s2w-app2.bin" ///< Path on the HTTP server for the APP1 firmware

/** Public Implementations **/
bool GS_Example_update_firmware(void){
     if (GS_API_UpdateFirmware(
              FWUP_SERVER_IP,
              FWUP_SERVER_PORT,
              FWUP_WLAN_PATH,
              FWUP_APP0_PATH,
              FWUP_APP1_PATH))
     {
          GS_API_Printf("Firmware Update OK");
          return true;
     } else {
          GS_API_Printf("Firmware Update Failed");
          return false;
     }
}
