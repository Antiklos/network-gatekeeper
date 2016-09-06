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

void send_payment_simulate(struct interface_id_udp *interface, char *address, int64_t price) {
  char current_message[CHAR_BUFFER_LEN];
  strcpy(current_message, address);
  strcat(current_message, " payment ");
  char payment_buffer[CHAR_BUFFER_LEN];
  sprintf(payment_buffer, "%lli ", (long long int)price);
  strcat(current_message, payment_buffer);

  //Wait a bit to send payment to simulate delay
  pid_t pay_pid;
  pay_pid = fork();
  if (pay_pid == 0) {
    sleep(10);
    printf("Sending message for payment now\n");
    link_send_udp(interface, current_message);
    exit(EXIT_SUCCESS);
  }
}

void payment_simulate_destroy() {
  printf("Executing payment_simulate_destroy\n");
}


