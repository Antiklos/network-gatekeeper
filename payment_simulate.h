#include <stdio.h>

#define PAYMENT_INTERFACE_SIMULATE_IDENTIFIER 1

#ifndef PAYMENT_SIMULATE_H

#define PAYMENT_SIMULATE_H

T_PAYMENT_INTERFACE payment_simulate_interface();

int payment_simulate_init();

void send_payment_simulate(struct interface_id_udp *interface, char *address, int64_t price);

void payment_simulate_destroy(int pid_payment);

#endif
