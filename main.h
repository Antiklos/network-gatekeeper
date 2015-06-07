#include <stdio.h>
#include <stdlib.h>

#define LINK_INTERFACES_NUMBER 1
#define NETWORK_INTERFACES_NUMBER 1
#define PAYMENT_INTERFACES_NUMBER 1

#define REQUEST 0
#define PROPOSE 1
#define ACCEPT 2
#define REJECT 3
#define BEGIN 4
#define STOP 5
#define COUNT_PACKETS 6
#define PAYMENT 7

#define MAX_CONNECTIONS 4
#define MAX_IDENTIFIER_LEN 256
#define MAX_ADDRESS_LEN 256

#define SOCK_PATH "/var/run/network_market.sock"
#define LOG_PATH "/var/log/network_market.log"

#ifndef MAIN_H
#define MAIN_H

typedef struct S_CONTRACT {
  char address[MAX_ADDRESS_LEN];
  int64_t price;
} T_CONTRACT;

typedef struct S_STATE {
  char interface_id[MAX_IDENTIFIER_LEN];
  int status;
  T_CONTRACT contract;
} T_STATE;

typedef struct S_CONFIG {
  int link_interface;
  int network_interface;
  int payment_interface;
} T_CONFIG;

typedef struct S_LINK_INTERFACE {
  int identifier;
  void (*link_init)();
  void (*send_request)(char *interface_id);
  void (*send_propose)(char *interface_id, T_CONTRACT contract);
  void (*send_accept)(char *interface_id);
  void (*send_reject)(char *interface_id);
  void (*send_begin)(char *interface_id);
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

static T_CONFIG read_config();

static int write_buffer(int sockfd, const char* message);
int send_cli_message();

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
