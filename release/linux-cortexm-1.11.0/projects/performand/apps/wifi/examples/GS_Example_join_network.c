/**
 * @file GS_Example_join_network.c
 *
 * Method implementation for join network example
 */

#include "GS_Examples.h"
#include "../API/GS_API.h"

bool GS_Example_join_network(void) {
        
     // Test the module first
     if (!GS_API_CommWorking()) {
          GS_API_Printf("No Comm");
          return false;
     } else {
          GS_API_Printf("Comm OK");
     }

     // Notify we are attempting to join network
     GS_API_Printf("Joining Network");
     // Print the network info of the network we are trying to join
     GS_API_PrintNetworkInfo();
     if(GS_API_JoinNetwork()){
          // Join network suceeded, print out the IP address we received
          GS_API_Printf("Join success");
          uint8_t ipAddr[GS_API_IP_STR_LENGTH];
          if(GS_API_GetIPAddress(ipAddr)){
               GS_API_Printf("IP: %s", ipAddr);
          }
          return true;
     } else {
          // Join network failed
          GS_API_Printf("Join failure");
          return false;
     }
}
