#include <stdio.h>

#define NETWORK_INTERFACE_TEST_IDENTIFIER 0

#ifndef NETWORK_TEST_H

#define NETWORK_TEST_H

T_NETWORK_INTERFACE network_test_interface();

pid_t network_test_init();

void gate_interface_test(char *src_addr, char *dst_addr, time_t time_expiration, long int bytes);

void network_test_destroy();

#endif
