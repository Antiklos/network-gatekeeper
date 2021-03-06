#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <ifaddrs.h>

#include "main.h"
#include "link_udp.c"
#include "network_ipv4.c"
#include "payment_simulate.c"
#include "payment_bitcoin.c"
#include "contract.c"

T_CONFIG config;

static T_CONFIG read_config()
{
  T_CONFIG config;
  FILE *file;
  char filebuf[1024];
  file = fopen("net.conf","r");
  char *buffer = filebuf;

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.link_interface = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.network_interface = (int)strtol(buffer,NULL,10);
  
  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.payment_interface = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  strcpy(config.account_id,buffer);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.default_price = (int64_t)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.contract_data = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.contract_time = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.payment_amount = (int64_t)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.data_renewal = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.time_renewal = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  strcpy(config.ignore_interface,buffer);

  return config;
}

static int parse_message(T_STATE *current_state, char *message, T_LINK_INTERFACE link_interface, T_PAYMENT_INTERFACE payment_interface, T_NETWORK_INTERFACE network_interface, T_CONFIG *config) {
    char *argument = strsep(&message," ");
    char buffer[CHAR_BUFFER_LEN];
    char *current_message = buffer;
    if (argument == NULL) {
      printf("No message sent to receive.\n");
    } else if (strcmp(argument,"propose") == 0) {
      char *price_arg = strsep(&message," ");
      char *time_expiration = strsep(&message," ");
      char *account_id = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for propose.\n");
      } else if (time_expiration == NULL) {
    printf("Time expiration not provided for propose.\n");
      } else if (current_state->status != DEFAULT && current_state->status != REJECT && current_state->status != ACCEPT) {
    printf("Not ready to receive propose.\n");
      } else {
    current_state->price = (int64_t)strtol(price_arg,NULL,10);
    current_state->time_expiration = (time_t)strtol(time_expiration,NULL,10);
    if (evaluate_propose(current_state, config)) {
      current_state->status = ACCEPT;
      strcpy(current_state->account->account_id, account_id);
      strcpy(current_message, current_state->address);
      strcat(current_message, " accept");
      link_interface.link_send(current_state->interface, current_message);
      current_state->account->balance += current_state->price;

      if (current_state->account->balance > 0) {
        int64_t payment = config->payment_amount;
        payment_interface.send_payment(current_state->interface, current_state->account->account_id, payment);
        current_state->bytes_sent = 0;
        current_state->account->balance -= payment;
      }
    } else {
      current_state->status = REJECT;
      sprintf(current_message, "%s propose %lli %u", current_state->address, (long long int)current_state->price, (unsigned int)current_state->time_expiration);
      link_interface.link_send(current_state->interface, current_message);
    }
      }
    } else if (strcmp(argument,"accept") == 0) {
      if (current_state->status != PROPOSE) {
        printf("Not ready to receive accept.\n");
      } else {
          network_interface.gate_address(current_state->interface->interface_id, current_state->address, current_state->time_expiration);
          current_state->account->balance -= current_state->price * config->contract_data;
          current_state->status = BEGIN;
      }
    } else if (strcmp(argument,"reject") == 0) {
      char *price_arg = strsep(&message," ");
      char *time_expiration = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for reject.\n");
      } else if (time_expiration == NULL) {
    printf("Time expiration not provided for propose.\n");
      } else if (current_state->status != PROPOSE) {
    printf("Not ready to receive reject.");
      } else {
        current_state->price = (int64_t)strtol(price_arg,NULL,10);
        current_state->time_expiration = (time_t)strtol(time_expiration,NULL,10);
        if (evaluate_request(current_state, config)) {
          sprintf(current_message, "%s propose %lli %u %s", current_state->address, (long long int)current_state->price, (unsigned int)current_state->time_expiration, config->account_id);
          link_interface.link_send(current_state->interface, current_message);
          current_state->status = PROPOSE;
        }
      }
    } else if (strcmp(argument,"payment") == 0) {
      char *price_arg = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for payment.\n");
      } else if (current_state->status != BEGIN) {
    printf("Not ready to receive payment.\n");
      } else {
      current_state->account->balance += (int64_t)strtol(price_arg,NULL,10);
      }
    } else {
      printf("Invalid message type.\n");
    }
}

static T_ACCOUNT* find_account(T_ACCOUNT accounts[], int new_account, char *account_id) {
  int i;
  for (i = 0; i < new_account; i++) {
    if (strcmp(accounts[i].account_id, account_id) == 0) {
      return &accounts[i];
    }
  }
}

static T_STATE* find_state(T_STATE states[], int *new_contract, T_ACCOUNT accounts[], int *new_account, T_INTERFACE *interface, char *address) {
  T_STATE *current_state = NULL;
  T_ACCOUNT *current_account = NULL;
  int i;
  for (i = 0; i < *new_contract; i++) {
    if (states[i].interface != NULL && strcmp(states[i].interface->net_addr_remote,interface->net_addr_remote) == 0) {
      if (strcmp(states[i].address,address) == 0) {
        current_state = &states[i];
      }
      current_account = states[i].account;
    }
  }
  if (current_state == NULL) {
    current_state = &states[*new_contract];
    *new_contract = *new_contract + 1;
    current_state->interface = interface;
    strcpy(current_state->address, address);
    current_state->status = DEFAULT;
    current_state->price = 0;
    current_state->bytes_sent = 0;
    if (current_account == NULL) {
      current_state->account = &accounts[*new_account];
      *new_account = *new_account + 1;
      current_state->account->balance = 0;
    } else {
      current_state->account = current_account;
    }
    printf("Creating new state for identifier %s and address %s\n",interface->interface_id, address);
  }

  return current_state;
}

int start(bool verbose)
{
  //Create interface arrays
  T_LINK_INTERFACE link_interfaces[LINK_INTERFACES_NUMBER];
  T_NETWORK_INTERFACE network_interfaces[NETWORK_INTERFACES_NUMBER];
  T_PAYMENT_INTERFACE payment_interfaces[PAYMENT_INTERFACES_NUMBER];

  //Populate interface arrays
  link_interfaces[LINK_INTERFACE_UDP_IDENTIFIER] = link_udp_interface();
  network_interfaces[NETWORK_INTERFACE_IPV4_IDENTIFIER] = network_ipv4_interface();
  payment_interfaces[PAYMENT_INTERFACE_SIMULATE_IDENTIFIER] = payment_simulate_interface();
  payment_interfaces[PAYMENT_INTERFACE_BITCOIN_IDENTIFIER] = payment_bitcoin_interface();

  //Read config file
  config = read_config();

  //Get interfaces from config file
  T_LINK_INTERFACE link_interface = link_interfaces[config.link_interface];
  T_NETWORK_INTERFACE network_interface = network_interfaces[config.network_interface];
  T_PAYMENT_INTERFACE payment_interface = payment_interfaces[config.payment_interface];

  /* Our process ID and Session ID */
  pid_t pid, sid;

  /* Fork off the parent process */
  pid = fork();

  if (pid < 0) {
    exit(EXIT_FAILURE);
  } 

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Change the file mode mask */
  umask(0);
    
  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  /* Change the current working directory */
  if ((chdir("/")) < 0) {
    exit(EXIT_FAILURE);
  }

  if (!verbose) {
    close(STDIN_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
      printf("failed to reopen stdin while daemonizing (errno=%d)\n",errno);
      exit(EXIT_FAILURE);
    }

    int logfile_fileno = open(LOG_PATH,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP);
    if (logfile_fileno == -1) {
      printf("failed to open logfile (errno=%d)\n",errno);
      exit(EXIT_FAILURE);
    }

    dup2(logfile_fileno,STDOUT_FILENO);
    dup2(logfile_fileno,STDERR_FILENO);
    close(logfile_fileno);
    setbuf(stdout, NULL);
  }

  //Set up socket for CLI communication
  int cli_sockfd;
  struct sockaddr_un addr_local, addr_remote;
  cli_sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  addr_local.sun_family = AF_UNIX; 
  strcpy(addr_local.sun_path, SOCK_PATH);
  unlink(addr_local.sun_path);
  int len = strlen(addr_local.sun_path) + sizeof(addr_local.sun_family);
  bind(cli_sockfd, (struct sockaddr *)&addr_local, len);
  listen(cli_sockfd, 5);

  //Set up loop for events
  T_STATE states[MAX_CONTRACTS];
  T_INTERFACE interfaces[MAX_CONNECTIONS];
  T_ACCOUNT accounts[MAX_ACCOUNTS];
  int new_contract = 0;
  int new_connection = 0;
  int new_account = 0;
  int command_result = 0;

  unsigned int i;

  link_interface.link_init(interfaces, &new_connection, config.ignore_interface);
  pid_t payment_pid = payment_interface.payment_init();

  char buffer[PACKET_BUFFER_SIZE];
  int data_size;

  struct sockaddr saddr;
  int saddr_size = sizeof(saddr);

  while(command_result > -1) {
    bzero(buffer,PACKET_BUFFER_SIZE);
    data_size = 0;

    for (i = 0; i < new_connection; i++) {
      data_size = recvfrom(interfaces[i].sockfd , buffer , PACKET_BUFFER_SIZE , MSG_DONTWAIT , (struct sockaddr *) &saddr , (socklen_t *)&saddr_size);
      if (data_size > 0) {
        char *message = buffer;
        T_INTERFACE *current_interface = link_interface.link_receive(&interfaces[i], &message);
        if (current_interface != NULL) {
          char *address = strsep(&message," ");

          if (config.payment_interface == PAYMENT_INTERFACE_SIMULATE_IDENTIFIER && strcmp(address,"payment") == 0) {
            char payment_message[256];
            sprintf(payment_message, "payment %s", message);
            send_cli_message(payment_message);
          } else {
            T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, address);
            if (current_state == NULL) {
              printf("Unable to find current state\n");
            } else {
              parse_message(current_state, message, link_interface, payment_interface, network_interface, &config);
            }
          }
        }
        break;
      }
    }

    for (i = 0; i < new_connection; i++) {
      data_size = recvfrom(interfaces[i].scan_sockfd , buffer , PACKET_BUFFER_SIZE , MSG_DONTWAIT ,(struct sockaddr *) &saddr , (socklen_t*)&saddr_size);
      if (data_size > 0) {
        char dst_address[CHAR_BUFFER_LEN];
        unsigned int packet_size;
        
        T_INTERFACE *current_interface = &interfaces[i];
        if (network_interface.sniff_datagram(buffer, current_interface->net_addr_local, dst_address, &packet_size) == 1) {
          if (strcmp(current_interface->net_addr_remote, dst_address) == 0) {
            printf("Ignoring broadcast\n");
            break;
          }
          T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, dst_address);
          current_state->bytes_sent += packet_size;

          if (current_state->status == DEFAULT)
          {
            network_interface.gate_interface(current_interface->interface_id, current_state->address);
            network_interface.gate_address(current_interface->interface_id, current_state->address, time(NULL) + 1);
            evaluate_request(current_state, &config);
            current_state->status = PROPOSE;
            char message[CHAR_BUFFER_LEN];
            sprintf(message, "%s propose %lli %u %s", current_state->address, (long long int)current_state->price, (unsigned int)current_state->time_expiration, config.account_id);
            link_interface.link_send(current_state->interface, message);
          }

          if (current_state->status == BEGIN && (current_state->bytes_sent > config.contract_data * 1024)) {
            network_interface.gate_interface(current_interface->interface_id, current_state->address);
            evaluate_request(current_state, &config);
            current_state->bytes_sent = 0;
            current_state->status = PROPOSE;
            char message[CHAR_BUFFER_LEN];
            sprintf(message, "%s propose %lli %u %s", current_state->address, (long long int)current_state->price, (unsigned int)current_state->time_expiration, config.account_id);
            link_interface.link_send(current_state->interface, message);
          }
        }
        break;
      }
    }

    int addr_size = sizeof(addr_remote);
    int cli_clientfd = accept(cli_sockfd, (struct sockaddr *)&addr_remote, &addr_size);
    data_size = recvfrom(cli_clientfd , buffer , PACKET_BUFFER_SIZE , MSG_DONTWAIT , (struct sockaddr *)&addr_remote , (socklen_t*)&addr_size);
    if (data_size > 0) {
      char *message = buffer;
      char *argument = strsep(&message," ");

      if (strcmp(argument,"test") == 0) {
        printf("Test OK\n");
      }
      else if (strcmp(argument,"stop") == 0) {
        printf("Daemon stopping.\n");
        command_result = -1;
      }
      else if (strcmp(argument,"payment") == 0) {
        char *address = strsep(&message," ");
        T_ACCOUNT *account = find_account(accounts, new_account, address);

        if (account != NULL) {
          char *price_arg = strsep(&message," ");
          int64_t payment = (int64_t)strtol(price_arg,NULL,10);
          account->balance += payment;
        } else {
          printf("No account found for account_id %s\n", address);
        }
      }
      else {
        printf("Invalid command sent to server: %s %s\n", argument, message);
      }
    }
  }

  for (i = 0; i < new_connection; i++) {
    close(interfaces[i].sockfd);
    close(interfaces[i].scan_sockfd);
  }
  close(cli_sockfd);
  network_interface.network_destroy();
  payment_interface.payment_destroy(payment_pid);
  fflush(stdout);
  exit(EXIT_SUCCESS);
  return 0;
}

int send_cli_message(char *message)
{
  struct sockaddr_un addr_remote;
  int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  addr_remote.sun_family = AF_UNIX;
  strcpy(addr_remote.sun_path, SOCK_PATH);
  int addrlen = strlen(addr_remote.sun_path) + sizeof(addr_remote.sun_family);
  int result = connect(sockfd, (struct sockaddr *)&addr_remote, addrlen);
  result = send(sockfd, message, strlen(message), 0);
  return 0;
}

main(int args, char *argv[])
{
  if (args < 2 || args > 3)
  {
    printf("You should use one or two arguments. No action taken.\n");
    return 1;
  }

  if (strcmp(argv[1],"start") == 0)
  {
    return start(args >= 3 && strcmp(argv[2],"-v") == 0);
  } 
  else if (strcmp(argv[1],"stop") == 0)
  {
    return send_cli_message("stop");
  } 
  else if (strcmp(argv[1],"server") == 0 && args == 3)
  {
    return send_cli_message(argv[2]);
  }
  else
  {
    printf("Invalid argument.\n");
    return 1;
  }
}

