#include <stdio.h>

#define LINK_INTERFACE_UDP_IDENTIFIER 1

#define LINK_UDP_DEFAULT_PORT 10325

#ifndef LINK_UDP_H

#define LINK_UDP_H

T_LINK_INTERFACE link_udp_interface();

void link_udp_init(T_INTERFACE interfaces[], int *new_connection, char *ignore_interface);

T_INTERFACE* link_receive_udp(T_INTERFACE *current_interface, char** message);

void link_send_udp(T_INTERFACE *interface, char *message);

void link_udp_destroy();

#endif
