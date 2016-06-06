#include <stdio.h>

#define NETWORK_INTERFACE_IPV4_IDENTIFIER 1

#ifndef NETWORK_IPV4_H

#define NETWORK_IPV4_H

T_NETWORK_INTERFACE network_ipv4_interface();

void network_ipv4_init();

void gate_interface_ipv4(char *interface_id, char *address, bool open);

void network_ipv4_destroy();

#endif