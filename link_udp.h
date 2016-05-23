#include <stdio.h>

#define LINK_INTERFACE_UDP_IDENTIFIER 1

#ifndef LINK_UDP_H

#define LINK_UDP_H

T_LINK_INTERFACE link_test_interface();

void link_test_init();

void send_request_test(char *interface_id, char *address);

void send_propose_test(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration);

void send_accept_test(char *interface_id, char *address);

void send_reject_test(char *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration);

void send_begin_test(char *interface_id, char *address);

void link_test_destroy();

#endif
