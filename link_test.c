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

void send_request_test(char *interface_id) {
  printf("Executing send_request_test\n");
}

void send_propose_test(char *interface_id, T_CONTRACT contract) {
  printf("Executing send_propose_test to address %s with price %i\n", contract.address, (int)contract.price);
}

void send_accept_test(char *interface_id) {
  printf("Executing send_accept_test\n");
}

void send_reject_test(char *interface_id) {
  printf("Executing send_reject_test\n");
}

void send_begin_test(char *interface_id) {
  printf("Executing send_begin_test\n");
}

void link_test_destroy() {
  printf("Executing link_stop_destroy\n");
}


