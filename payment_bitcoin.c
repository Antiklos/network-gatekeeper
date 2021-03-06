#include <stdio.h>
#include <signal.h>

#include "payment_bitcoin.h"

T_PAYMENT_INTERFACE payment_bitcoin_interface() {
  T_PAYMENT_INTERFACE interface;
  interface.payment_init = &payment_bitcoin_init;
  interface.send_payment = &send_payment_bitcoin;
  interface.payment_destroy = &payment_bitcoin_destroy;
  return interface;
}

int payment_bitcoin_init() {
  pid_t pay_pid;
  pay_pid = fork();
  if (pay_pid == 0) {
    FILE *p;
    int tx_number = -1;
    char buffer[BITCOIN_ADDRESS_LEN];
    char *output = buffer;
    char cbuffer[256];
    char *command = cbuffer;
    while(true) {
      sleep(1);
      p = popen("electrum history | jq '. | length'", "r");

      bzero(buffer, BITCOIN_ADDRESS_LEN);
      fgets(output, BITCOIN_ADDRESS_LEN, p);
      if (pclose(p) == -1) {
        printf("Error closing pipe\n");
      }

      if (tx_number == -1) {
        tx_number = 0;
      }

      int tx_found = atoi(output);
      while(tx_found > tx_number) {
        bzero(cbuffer, 256);
        sprintf(command, "electrum history | jq '.[%i].value * 100000000'",tx_number);
        p = popen(command, "r");
        
        bzero(buffer, BITCOIN_ADDRESS_LEN);
        fgets(output, BITCOIN_ADDRESS_LEN, p);
        if (pclose(p) == -1) {
          printf("Error closing pipe\n");
        }

        char *mantissa = strsep(&output, "e");
        int64_t payment = (int64_t)atoi(mantissa);
        strsep(&output, "+");
        if (output != NULL) {
          int exponent = atoi(output);
          while (exponent > 0) {
            payment *= 10;
            exponent--;
          }
        }
        if (payment > 0) {
          bzero(cbuffer, 256);
          sprintf(command, "electrum deserialize $(electrum gettransaction $(electrum history | jq -r '.[%i].txid') | jq -r '.hex') | jq -r '.inputs[0].address'",tx_number);
          p = popen(command, "r");
        
          bzero(buffer, BITCOIN_ADDRESS_LEN);
          fgets(output, BITCOIN_ADDRESS_LEN, p);
          if (pclose(p) == -1) {
            printf("Error closing pipe\n");
          }

          char current_message[CHAR_BUFFER_LEN];
          sprintf(current_message, "payment %s %lli", output, (long long int)payment);
          printf("Detected payment and sending message %s\n",current_message);
          send_cli_message(current_message);
        }

        tx_number++;
      }
    }
  }

  return pay_pid;
}

void send_payment_bitcoin(T_INTERFACE *interface, char *address, int64_t price) {
  char buffer[256];
  char *command = buffer;
  double price_double = price / 100000000.0f;
  sprintf(command, "electrum broadcast $(electrum payto %s %.8f -W password | jq -r '.hex')", address, price_double);
  printf("Sending bitoin payment with command %s\n",command);
  system(command);
}

void payment_bitcoin_destroy(int pid_payment) {
  kill(pid_payment, SIGTERM);
}


