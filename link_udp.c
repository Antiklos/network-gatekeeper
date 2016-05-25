#include <stdio.h>

#include "link_udp.h"

T_LINK_INTERFACE link_udp_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_udp_init;
  interface.send_request = &send_request_udp;
  interface.send_propose = &send_propose_udp;
  interface.send_accept = &send_accept_udp;
  interface.send_reject = &send_reject_udp;
  interface.send_begin = &send_begin_udp;
  interface.link_destroy = &link_udp_destroy;
  return interface;
}

void link_udp_init() {
  printf("Executing link_test_init\n");
}

void send_request_udp(char *interface_id, char *address) {
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(10325);
  inet_aton("127.0.0.1" , &serv_addr.sin_addr);
  int result = sendto(sockfd, "test", 4, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (result < 0) {
    printf("send_request_udp failed\n");
  }
}

void send_propose_udp(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration) {
  printf("Executing send_propose_test on interface %s and address %s with price %i advance %i expiration %i\n",
    interface_id, address, (int)price, (int)payment_advance, (int)time_expiration);
}

void send_accept_udp(char *interface_id, char *address) {
  printf("Executing send_accept_test on interface %s and address %s\n", interface_id, address);
}

void send_reject_udp(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration) {
  printf("Executing send_reject_test on interface %s and address %s with price %i advance %i expiration %i\n", 
    interface_id, address, (int)price, (int)payment_advance, (int)time_expiration);
}

void send_begin_udp(char *interface_id, char *address) {
  printf("Executing send_begin_test on interface %s and address %s\n", interface_id, address);
}

void link_udp_destroy() {
  printf("Executing link_stop_destroy\n");
}


