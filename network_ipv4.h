#include <stdio.h>

#define NETWORK_INTERFACE_IPV4_IDENTIFIER 1

#define PACKET_BUFFER_SIZE 65536

#ifndef NETWORK_IPV4_H

#define NETWORK_IPV4_H

T_NETWORK_INTERFACE network_ipv4_interface();

pid_t network_ipv4_init();

void gate_interface_ipv4(char *src_addr, char *dst_addr, time_t time_expiration, long int bytes);

void network_ipv4_destroy(pid_t net_pid);

#endif
