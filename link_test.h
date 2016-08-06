#include <stdio.h>

#define LINK_INTERFACE_TEST_IDENTIFIER 0

#ifndef LINK_TEST_H

#define LINK_TEST_H

T_LINK_INTERFACE link_test_interface();

void link_test_init();

struct interface_id_udp* link_find_interface_test(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char *ip_addr_src, char *ip_addr_dst);

struct interface_id_udp* link_receive_test(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char** message);

void link_send_test(struct interface_id_udp *interface_id, char *message);

void link_test_destroy();

#endif
