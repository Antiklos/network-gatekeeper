#include <stdio.h>
#include <stdlib.h>

#define LINK_INTERFACES_NUMBER 1
#define NETWORK_INTERFACES_NUMBER 1
#define PAYMENT_INTERFACES_NUMBER 1

#define INPUT_RECEIVE_REQUEST 0
#define INPUT_RECEIVE_PROPOSE 1
#define INPUT_RECEIVE_ACCEPT 2
#define INPUT_RECEIVE_REJECT 3
#define INPUT_RECEIVE_BEGIN 4
#define INPUT_RECEIVE_STOP 5
#define INPUT_COUNT_PACKETS 6
#define INPUT_RECEIVE_PAYMENT 7

#define SOCK_PATH "/var/run/network_market.sock"

#ifndef MAIN_H
#define MAIN_H

typedef struct S_CONFIG {
  int link_interface;
  int network_interface;
  int payment_interface;
} T_CONFIG;

typedef struct S_LINK_INTERFACE {
  int identifier;
  void (*link_init)();
  void (*send_request)();
  void (*send_propose)();
  void (*send_accept)();
  void (*send_reject)();
  void (*send_begin)();
  void (*send_stop)();
  void (*link_destroy)();
} T_LINK_INTERFACE;

typedef struct S_NETWORK_INTERFACE {
  int identifier;
  void (*network_init)();
  void (*open_interface)();
  void (*network_destroy)();
} T_NETWORK_INTERFACE;

typedef struct S_PAYMENT_INTERFACE {
  int identifier;
  void (*payment_init)();
  void (*send_payment)();
  void (*payment_destroy)();
} T_PAYMENT_INTERFACE;

static bool input_loop();

static T_CONFIG read_config();

// Link Interface

void receive_request();

void receive_propose();

void receive_accept();

void receive_reject();

void receive_begin();

void receive_stop();

// Network Interface

void count_packets(int count);

// Payment Interface

void receive_payment(int amount);

// User Interface

#endif
