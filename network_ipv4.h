#include <stdio.h>

#define NETWORK_INTERFACE_IPV4_IDENTIFIER 1

#define PACKET_BUFFER_SIZE 65536

#ifndef NETWORK_IPV4_H

#define NETWORK_IPV4_H

T_NETWORK_INTERFACE network_ipv4_interface();

void network_ipv4_init();

int sniff_datagram_ipv4(char *buffer, char *local_address, char *dst_address, unsigned int *packet_size);

void gate_interface_ipv4(char *interface_id, char *address);

void gate_address_ipv4(char *interface_id, char *dst_addr, time_t time_expiration);

void network_ipv4_destroy();

#endif
