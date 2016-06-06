#include <stdio.h>

#include "payment_simulate.h"

T_PAYMENT_INTERFACE payment_simulate_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_simulate_init;
  interface.send_payment = &send_payment_simulate;
  interface.payment_destroy = &payment_simulate_destroy;
  return interface;
}

void payment_simulate_init() {
  printf("Executing payment_simulate_init\n");
}

void send_payment_simulate(char *interface_id, char *address, int64_t price) {
  printf("Executing send_payment_simulate on interface %s and address %s with price %i\n", interface_id, address, (int)price);
}

void payment_simulate_destroy() {
  printf("Executing payment_simulate_destroy\n");
}


