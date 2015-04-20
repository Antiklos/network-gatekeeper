#include <stdio.h>

#include "network_test.h"

T_NETWORK_INTERFACE network_test_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_test_init;
  return interface;
}

void network_test_init() {
  printf("Executing network_test_init\n");
}

void open_interface_test() {
  printf("Executing open_interface_test\n");
}

void network_test_destroy() {
  printf("Executing network_test_destroy\n");
}


