#include <stdio.h>

#include "payment_test.h"

T_PAYMENT_INTERFACE payment_test_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_test_init;
  return interface;
}

void payment_test_init() {
  printf("Executing payment_test_init\n");
}

void send_payment_test() {
  printf("Executing send_payment_test\n");
}

void payment_test_destroy() {
  printf("Executing payment_test_destroy\n");
}


