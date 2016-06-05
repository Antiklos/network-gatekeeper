#include <stdio.h>
#include <stdlib.h>

#define CHAR_BUFFER_LEN 256

#define LINK_INTERFACES_NUMBER 2
#define NETWORK_INTERFACES_NUMBER 2
#define PAYMENT_INTERFACES_NUMBER 1

#define DEFAULT 0
#define REQUEST 1
#define PROPOSE 2
#define ACCEPT 3
#define REJECT 4
#define BEGIN 5
#define STOP 6
#define COUNT_PACKETS 7
#define PAYMENT 8

#define MAX_CONNECTIONS 4
#define MAX_CONTRACTS 10
#define MAX_IDENTIFIER_LEN 256
#define MAX_ADDRESS_LEN 256

//The following three are assumptions for testing that should be eventually calculated dynamically
#define MAX_PAYMENT 1000
#define TIME_TO_TX_CONFIRM 5
#define PACKET_THROUGHPUT 3

#define SOCK_PATH "/var/run/network_gatekeeper.sock"
#define LOG_PATH "/var/log/network_gatekeeper.log"

#ifndef MAIN_H
#define MAIN_H

struct interface_id_udp {
  char ip_addr_src[16];
  char ip_addr_dst[16];
  unsigned int incoming_port;
  unsigned int outgoing_port;
  int sockfd;
};

typedef struct S_STATE {
  struct interface_id_udp *interface_id;
  int sock_in;
  int sock_out;
  int status;
  char address[MAX_ADDRESS_LEN];
  int64_t price;
  long int payment_advance;
  time_t time_expiration;
  int64_t payment_sent;
  long int packets_delivered;
} T_STATE;

typedef struct S_CONFIG {
  int link_interface;
  int network_interface;
  int payment_interface;
} T_CONFIG;

typedef struct S_LINK_INTERFACE {
  int identifier;
  void (*link_init)();
  char* (*link_receive)(struct interface_id_udp *interface_id);
  void (*send_request)(struct interface_id_udp *interface_id, char *address);
  void (*send_propose)(struct interface_id_udp *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration);
  void (*send_accept)(struct interface_id_udp *interface_id, char *address);
  void (*send_reject)(struct interface_id_udp *interface_id, char *address, int64_t price, long int payment_advance, time_t time_expiration);
  void (*send_begin)(struct interface_id_udp *interface_id, char *address);
  void (*send_stop)();
  void (*link_destroy)();
} T_LINK_INTERFACE;

typedef struct S_NETWORK_INTERFACE {
  int identifier;
  void (*network_init)();
  void (*gate_interface)(char *interface_id, char *address, bool open);
  void (*network_destroy)();
} T_NETWORK_INTERFACE;

typedef struct S_PAYMENT_INTERFACE {
  int identifier;
  void (*payment_init)();
  void (*send_payment)(char *interface_id, char *address, int64_t price);
  void (*payment_destroy)();
} T_PAYMENT_INTERFACE;

static T_CONFIG read_config();

static int write_buffer(int sockfd, const char* message);
int send_cli_message();
static T_STATE* find_state(T_STATE states[], int *new_contract, struct interface_id_udp *interface_id, char *address);

#endif

