#include <stdio.h>
#include <time.h>

#include "network_test.h"

T_NETWORK_INTERFACE network_test_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_test_init;
  interface.sniff_datagram = &sniff_datagram_test;
  interface.gate_interface = &gate_interface_test;
  interface.network_destroy = &network_test_destroy;
  return interface;
}

pid_t network_test_init(T_STATE states[], int *new_connection) {
  printf("Executing network_test_init\n");
}

int sniff_datagram_test(char *buffer, char **src_addr, char **dst_addr, char **next_hop, char *ngp_interface) {
}

void gate_interface_test(char *src_addr, char *dst_addr, time_t time_expiration, long int bytes) {
  printf("Executing gate_interface_test on src_addr %s and dst_addr %s and time_expiration %i\n", src_addr, dst_addr, (int)time_expiration);
}

void network_test_destroy(pid_t net_pid) {
  printf("Executing network_test_destroy\n");
}


