#include <stdio.h>

#include "payment_test.h"

T_PAYMENT_INTERFACE payment_test_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_test_init;
  interface.send_payment = &send_payment_test;
  interface.payment_destroy = &payment_test_destroy;
  return interface;
}

void payment_test_init() {
  printf("Executing payment_test_init\n");
}

void send_payment_test(char *interface_id, char *address, int64_t price) {
  printf("Executing send_payment_test on interface %s and address %s with price %i\n", interface_id, address, (int)price);
}

void payment_test_destroy() {
  printf("Executing payment_test_destroy\n");
}


