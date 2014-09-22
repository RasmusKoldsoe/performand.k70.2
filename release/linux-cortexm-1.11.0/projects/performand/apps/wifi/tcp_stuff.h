#include <stdint.h>

#define TCP_MAX_LEN 1400

int tcp_init(char *port);
int tcp_send_data(char *data, int len);
void tcp_end();
