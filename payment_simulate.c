#include <stdio.h>

#include "payment_simulate.h"

T_PAYMENT_INTERFACE payment_simulate_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_simulate_init;
  interface.send_payment = &send_payment_simulate;
  interface.payment_destroy = &payment_simulate_destroy;
  return interface;
}

int payment_simulate_init() {
  printf("Executing payment_simulate_init\n");
  return 0;
}

void send_payment_simulate(struct interface_id_udp *interface, char *address, int64_t price) {
  char current_message[CHAR_BUFFER_LEN];
  sprintf(current_message, "payment %s %lli", address, (long long int)price);

  //Wait a bit to send payment to simulate delay
  pid_t pay_pid;
  pay_pid = fork();
  if (pay_pid == 0) {
    sleep(5);
    printf("Sending message for payment now\n");
    link_send_udp(interface, current_message);
    exit(EXIT_SUCCESS);
  }
}

void payment_simulate_destroy(int pid_payment) {
  printf("Executing payment_simulate_destroy\n");
}


