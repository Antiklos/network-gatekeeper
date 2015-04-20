#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "main.h"
#include "link_test.c"
#include "network_test.c"
#include "payment_test.c"

static T_CONFIG read_config()
{
  T_CONFIG config;
  config.link_interface = LINK_INTERFACE_TEST_IDENTIFIER;
  config.network_interface = NETWORK_INTERFACE_TEST_IDENTIFIER;
  config.payment_interface = PAYMENT_INTERFACE_TEST_IDENTIFIER;
  return config;

//FILE *file;
//file = fopen("network.conf",'r');
//int filesize = 
//char* contents = malloc(filesize * sizeof(char));
//fclose(file);
}

static bool input_loop()
{
  bool loop = true;
  int input;
  scanf("%d", &input);

  switch(input) {
    case INPUT_RECEIVE_REQUEST:
      receive_request();
    break;
    case INPUT_RECEIVE_PROPOSE:
      receive_propose();
    break;
    case INPUT_RECEIVE_ACCEPT:
      receive_accept();
    break;
    case INPUT_RECEIVE_REJECT:
      receive_reject();
    break;
    case INPUT_RECEIVE_BEGIN:
      receive_begin();
    break;
    case INPUT_RECEIVE_STOP:
      receive_stop();
    break;
    case INPUT_COUNT_PACKETS:
      count_packets(5);
    break;
    case INPUT_RECEIVE_PAYMENT:
      receive_payment(10);
    break;
    default:
      loop = false;
      break;
  }

  return loop;
}

main(int args, char *argv[])
{

//Create interface arrays
T_LINK_INTERFACE link_interfaces[LINK_INTERFACES_NUMBER];
T_NETWORK_INTERFACE network_interfaces[NETWORK_INTERFACES_NUMBER];
T_PAYMENT_INTERFACE payment_interfaces[PAYMENT_INTERFACES_NUMBER];

//Populate interface arrays
link_interfaces[LINK_INTERFACE_TEST_IDENTIFIER] = link_test_interface();
network_interfaces[NETWORK_INTERFACE_TEST_IDENTIFIER] = network_test_interface();
payment_interfaces[PAYMENT_INTERFACE_TEST_IDENTIFIER] = payment_test_interface();

//Read config file
T_CONFIG config = read_config();

//Get interfaces from config file
T_LINK_INTERFACE link_interface = link_interfaces[config.link_interface];
T_NETWORK_INTERFACE network_interface = network_interfaces[config.network_interface];
T_PAYMENT_INTERFACE payment_interface = payment_interfaces[config.payment_interface];

bool loop = true;
while (loop == true) {
  loop = input_loop();
}

return 0;
}

// Link Interface

void receive_request() {
  printf("Executing receive_request\n");
}

void receive_propose() {
  printf("Executing receive_propose\n");
}

void receive_accept() {
  printf("Executing receive_accept\n");
}

void receive_reject() {
  printf("Executing receive_reject\n");
}

void receive_begin() {
  printf("Executing receive_begin\n");
}

void receive_stop() {
  printf("Executing receive_stop\n");
}

// Network Interface

void count_packets(int count) {
  printf("Executing count_packets with %d packets having been counted.\n", count);
}


// Payment Interface

void receive_payment(int amount) {
  printf("Executing receive_payment with %d having been received.\n", amount);
}

// User Interface


