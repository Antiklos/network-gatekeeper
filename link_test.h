#include <stdio.h>

#define LINK_INTERFACE_TEST_IDENTIFIER 0

#ifndef LINK_TEST_H

#define LINK_TEST_H

T_LINK_INTERFACE link_test_interface();

void link_test_init();

void send_request_test(char *interface_id, char *address);

void send_propose_test(char *interface_id, int64_t price);

void send_accept_test(char *interface_id);

void send_reject_test(char *interface_id, int64_t price);

void send_begin_test(char *interface_id);

void link_test_destroy();

#endif
