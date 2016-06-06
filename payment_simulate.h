#include <stdio.h>

#define PAYMENT_INTERFACE_SIMULATE_IDENTIFIER 1

#ifndef PAYMENT_SIMULATE_H

#define PAYMENT_SIMULATE_H

T_PAYMENT_INTERFACE payment_simulate_interface();

void payment_simulate_init();

void send_payment_simulate(char *interface_id, char *address, int64_t price);

void payment_simulate_destroy();

#endif
