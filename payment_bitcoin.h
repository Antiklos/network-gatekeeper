#include <stdio.h>

#define PAYMENT_INTERFACE_BITCOIN_IDENTIFIER 2

#define BITCOIN_ADDRESS_LEN 36

#ifndef PAYMENT_BITCOIN_H

#define PAYMENT_BITCOIN_H

T_PAYMENT_INTERFACE payment_bitcoin_interface();

int payment_bitcoin_init();

void send_payment_bitcoin(struct interface_id_udp *interface, char *address, int64_t price);

void payment_bitcoin_destroy(int pid_payment);

#endif
