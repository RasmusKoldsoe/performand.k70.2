/**
 * @file GS_Example_provisioning.c
 *
 * Public Method implementation for provisioning example
 */

#include "GS_Examples.h"
#include "../API/GS_API.h"

/** Private Defines **/
#define PROVISION_SSID "GS_PROV" ///< SSID for limited AP network that will be created
#define PROVISION_CHANNEL "6" ///< Channel for limited AP network that will be created
#define PROVISION_USERNAME "gainspan" ///< Username for web configuration client login
#define PROVISION_PASSWORD "gainspan" ///< Password for web configuration client login
#define PROVISION_IP  "192.168.141.1" ///< IP address of the module 
#define PROVISION_SUBNET "255.255.255.0" ///< Subnet of the network module will create, devices that join the network will be assigned IP addresses within the subnet of PROVISION_IP
#define PROVISION_HOSTNAME "gsprov" ///< Host name of the module for DNS

/** Public Method Implementations **/
void GS_Example_start_provisioning_task(void){
     // Activate Provisioning
     GS_API_StartProvisioning(
			PROVISION_SSID,
			PROVISION_CHANNEL,
			PROVISION_USERNAME,
			PROVISION_PASSWORD,
			PROVISION_IP,
			PROVISION_SUBNET,
			PROVISION_HOSTNAME);
}

bool GS_Example_provisioning_task(void){
     return GS_API_PollProvisioningResponse();
}

void GS_Example_stop_provisioning_task(void){
     GS_API_StopProvisioning();
}
