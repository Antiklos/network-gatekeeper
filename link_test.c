#include <stdio.h>

#include "link_test.h"

T_LINK_INTERFACE link_test_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_test_init;
  interface.send_request = &send_request_test;
  interface.send_propose = &send_propose_test;
  interface.send_accept = &send_accept_test;
  interface.send_reject = &send_reject_test;
  interface.send_begin = &send_begin_test;
  interface.link_destroy = &link_test_destroy;
  return interface;
}

void link_test_init() {
  printf("Executing link_test_init\n");
}

void send_request_test(char *interface_id, char *address) {
  printf("Executing send_request_test on interface %s for address %s\n", interface_id, address);
}

void send_propose_test(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration) {
  printf("Executing send_propose_test on interface %s and address %s with price %i advance %i expiration %i\n",
    interface_id, address, (int)price, (int)payment_advance, (int)time_expiration);
}

void send_accept_test(char *interface_id, char *address) {
  printf("Executing send_accept_test on interface %s and address %s\n", interface_id, address);
}

void send_reject_test(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration) {
  printf("Executing send_reject_test on interface %s and address %s with price %i advance %i expiration %i\n", 
    interface_id, address, (int)price, (int)payment_advance, (int)time_expiration);
}

void send_begin_test(char *interface_id, char *address) {
  printf("Executing send_begin_test on interface %s and address %s\n", interface_id, address);
}

void link_test_destroy() {
  printf("Executing link_stop_destroy\n");
}


