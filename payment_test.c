#include <stdio.h>

#include "payment_test.h"

T_PAYMENT_INTERFACE payment_test_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_test_init;
  interface.send_payment = &send_payment_test;
  interface.payment_destroy = &payment_test_destroy;
  return interface;
}

int payment_test_init() {
  printf("Executing payment_test_init\n");
  return 0;
}

void send_payment_test(struct interface_id_udp *interface, char *address, int64_t price) {
  printf("Executing send_payment_test on interface %s and address %s with price %i\n", interface->interface, address, (int)price);
}

void payment_test_destroy(int pid_payment) {
  printf("Executing payment_test_destroy\n");
}


