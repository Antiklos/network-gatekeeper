#include <stdio.h>

#include "link_test.h"

T_LINK_INTERFACE link_test_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_test_init;
  interface.link_find_interface = &link_find_interface_test;
  interface.link_receive = &link_receive_test;
  interface.link_send = &link_send_test;
  interface.link_destroy = &link_test_destroy;
  return interface;
}

void link_test_init() {
  printf("Executing link_test_init\n");
}

struct interface_id_udp* link_find_interface_test(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char *ip_addr_src, char *ip_addr_dst) {
}

struct interface_id_udp* link_receive_test(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char** message) {
  //printf("Executing send_propose_test on interface %s and address %s with price %i advance %i expiration %i\n",
    //interface_id, address, (int)price, (int)payment_advance, (int)time_expiration);
}

void link_send_test(struct interface_id_udp *interface_id, char *message) {
  //printf("Executing send_request_test on interface %s for address %s\n", interface_id, address);
}

void link_test_destroy() {
  printf("Executing link_stop_destroy\n");
}


