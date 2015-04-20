#include <stdio.h>

#include "link_test.h"

T_LINK_INTERFACE link_test_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_test_init;
  return interface;
}

void link_test_init() {
  printf("Executing link_test_init\n");
}

void send_request_test() {
  printf("Executing send_request_test\n");
}

void send_propose_test() {
  printf("Executing send_propose_test\n");
}

void send_accept_test() {
  printf("Executing send_accept_test\n");
}

void send_reject_test() {
  printf("Executing send_reject_test\n");
}

void send_begin_test() {
  printf("Executing send_begin_test\n");
}

void link_test_destroy() {
  printf("Executing link_stop_destroy\n");
}


