#include <stdio.h>

#include "network_test.h"

T_NETWORK_INTERFACE network_test_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_test_init;
  interface.gate_interface = &gate_interface_test;
  interface.network_destroy = &network_test_destroy;
  return interface;
}

void network_test_init() {
  printf("Executing network_test_init\n");
}

void gate_interface_test(char *interface_id, char *address, bool open) {
  printf("Executing gate_interface_test on interface %s and address %s and open value %i\n", interface_id, address, open);
}

void network_test_destroy() {
  printf("Executing network_test_destroy\n");
}


