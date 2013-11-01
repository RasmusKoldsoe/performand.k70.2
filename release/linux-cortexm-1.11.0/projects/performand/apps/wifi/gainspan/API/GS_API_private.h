/**
 * @file GS_API_private.h
 * INTERNAL ONLY
 *
 * Private methods and defines for GS_API
 */

#ifndef GS_API_PRIVATE_H_
#define GS_API_PRIVATE_H_

#define NETWORK_SAVE_ADDRESS 0
#define IP_STRING_LENGTH     16
#define PORT_STRING_LENGTH   6

void gs_api_readNetworkConfig(void);
void gs_api_writeNetworkConfig(void);

#endif /* GS_API_PRIVATE_H_ */
