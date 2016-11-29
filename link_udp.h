#include <stdio.h>

#define LINK_INTERFACE_UDP_IDENTIFIER 1

#define LINK_UDP_DEFAULT_PORT 10325

#ifndef LINK_UDP_H

#define LINK_UDP_H

T_LINK_INTERFACE link_udp_interface();

void link_udp_init();

struct interface_id_udp* link_find_interface_udp(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char *interface_id, char *ip_addr_src, char *ip_addr_dst);

struct interface_id_udp* link_receive_udp(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char** message);

void link_send_udp(struct interface_id_udp *interface_id, char *message);

void link_udp_destroy();

#endif
