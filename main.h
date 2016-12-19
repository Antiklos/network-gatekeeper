#include <stdio.h>
#include <stdlib.h>

#define CHAR_BUFFER_LEN 256

#define LINK_INTERFACES_NUMBER 2
#define NETWORK_INTERFACES_NUMBER 2
#define PAYMENT_INTERFACES_NUMBER 3

#define DEFAULT 0
#define PROPOSE 1
#define ACCEPT 2
#define REJECT 3
#define BEGIN 4

//These arrays need to be allocated on the heap as linked lists
#define MAX_CONNECTIONS 4
#define MAX_CONTRACTS 10
#define MAX_ACCOUNTS 10

// This needs to be made to be available local only
#define UNIX_UDP_PORT 33546

//This needs to be configurable
#define CONTRACT_DATA_SIZE 4096

#define SOCK_PATH "/var/run/network_gatekeeper.sock"
#define LOG_PATH "/var/log/network_gatekeeper.log"

#ifndef MAIN_H
#define MAIN_H

struct route_info
{
  struct in_addr dstAddr;
  struct in_addr srcAddr;
  struct in_addr gateWay;
  char ifName[IF_NAMESIZE];
};

typedef struct S_ACCOUNT {
  char account_id[35];
  int64_t balance;
} T_ACCOUNT;

typedef struct S_INTERFACE {
  char interface_id[IFNAMSIZ];
  char net_addr_local[16];
  char net_addr_remote[16];
  bool broadcast;
  int sockfd;
  int scan_sockfd;
} T_INTERFACE;

typedef struct S_STATE {
  T_INTERFACE *interface;
  int sock_in;
  int sock_out;
  int status;
  char address[16];
  int64_t price;
  unsigned long int bytes_sent;
  time_t time_expiration;
  T_ACCOUNT *account;
} T_STATE;

typedef struct S_CONFIG {
  int link_interface;
  int network_interface;
  int payment_interface;
  char account_id[35];
  int64_t default_price;
  int contract_data;
  int contract_time;
  int64_t payment_amount;
  int data_renewal;
  int time_renewal;
  char ignore_interface[IFNAMSIZ];
} T_CONFIG;

typedef struct S_LINK_INTERFACE {
  int identifier;
  void (*link_init)(T_INTERFACE interfaces[], int *new_connection, char *ignore_interface);
  T_INTERFACE* (*link_receive)(T_INTERFACE *current_interface, char** message);
  void (*link_send)(T_INTERFACE *interface_id, char *message);
  void (*link_destroy)();
} T_LINK_INTERFACE;

typedef struct S_NETWORK_INTERFACE {
  int identifier;
  void (*network_init)();
  int (*sniff_datagram)(char *buffer, char *local_address, char *dst_address, unsigned int *packet_size);
  void (*gate_interface)(char *interface_id, char *address);
  void (*gate_address)(char *interface_id, char *dst_addr, time_t time_expiration);
  void (*network_destroy)();
} T_NETWORK_INTERFACE;

typedef struct S_PAYMENT_INTERFACE {
  int identifier;
  int (*payment_init)();
  void (*send_payment)(T_INTERFACE *interface, char *address, int64_t price);
  void (*payment_destroy)(int pid_payment);
} T_PAYMENT_INTERFACE;

static T_CONFIG read_config();
static T_STATE* find_state(T_STATE states[], int *new_contract, T_ACCOUNT accounts[], int *new_account, T_INTERFACE *interface, char *address);

#endif

