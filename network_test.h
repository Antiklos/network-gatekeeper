#include <stdio.h>

#define NETWORK_INTERFACE_TEST_IDENTIFIER 0

#ifndef NETWORK_TEST_H

#define NETWORK_TEST_H

T_NETWORK_INTERFACE network_test_interface();

void network_test_init();

void gate_interface_test(char *interface_id, char *address, bool open);

void network_test_destroy();

#endif
