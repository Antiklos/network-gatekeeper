#include <stdio.h>
#include <stdlib.h>

#define CHAR_BUFFER_LEN 256

#define LINK_INTERFACES_NUMBER 2
#define NETWORK_INTERFACES_NUMBER 2
#define PAYMENT_INTERFACES_NUMBER 3

#define DEFAULT 0
#define REQUEST 1
#define PROPOSE 2
#define ACCEPT 3
#define REJECT 4
#define BEGIN 5

//These arrays need to be allocated on the heap as linked lists
#define MAX_CONNECTIONS 4
#define MAX_CONTRACTS 10
#define MAX_ACCOUNTS 10
#define MAX_INTERFACE_LEN 256
#define MAX_ADDRESS_LEN 256

// This needs to be made to be available local only
#define UNIX_UDP_PORT 33546

//The following three are assumptions for testing that should be eventually calculated dynamically
#define MAX_PAYMENT 1000
#define TIME_TO_TX_CONFIRM 5
#define PACKET_THROUGHPUT 3

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
  unsigned int local_port;
  unsigned int remote_port;
  int sockfd;
  int scan_sockfd;
} T_INTERFACE;

typedef struct S_STATE {
  T_INTERFACE *interface;
  int sock_in;
  int sock_out;
  int status;
  char address[MAX_ADDRESS_LEN];
  int64_t price;
  unsigned long int bytes_sent;
  time_t time_expiration;
  T_ACCOUNT *account;
} T_STATE;

typedef struct S_CONFIG {
  int link_interface;
  int network_interface;
  int payment_interface;
  char ngp_interface[MAX_INTERFACE_LEN];
  char account_id[35];
  int64_t default_price;
  int64_t grace_period_price;
  int contract_data;
  int contract_time;
  int data_renewal;
  int time_renewal;
  char ignore_interface[IFNAMSIZ];
} T_CONFIG;

typedef struct S_LINK_INTERFACE {
  int identifier;
  void (*link_init)();
  T_INTERFACE* (*link_find_interface)(T_INTERFACE interfaces[], int *new_connection, int sockfd, char *interface_id, char *ip_addr_src, char *ip_addr_dst);
  T_INTERFACE* (*link_receive)(T_INTERFACE interfaces[], int *new_connection, int sockfd, char** message);
  void (*link_send)(T_INTERFACE *interface_id, char *message);
  void (*link_destroy)();
} T_LINK_INTERFACE;

typedef struct S_NETWORK_INTERFACE {
  int identifier;
  pid_t (*network_init)(T_STATE states[], int *new_connection, char *ignore_interface);
  int (*sniff_datagram)(char *buffer, char *src_addr, char *dst_addr, char *next_hop, char *ngp_interface, unsigned int *packet_size);
  void (*gate_interface)(char *src_addr, char *dst_addr, time_t time_expiration, long int bytes);
  void (*network_destroy)(pid_t net_pid);
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

