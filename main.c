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
#include "link_test.c"
#include "link_udp.c"
#include "network_test.c"
#include "network_ipv4.c"
#include "payment_test.c"
#include "payment_simulate.c"
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
  strcpy(config.ngp_interface,buffer);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.default_price = (int64_t)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.grace_period_price = (int64_t)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.grace_period_time = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.data_renewal = (int)strtol(buffer,NULL,10);

  fscanf(file,"%s\n",buffer);
  strsep(&buffer,"=");
  config.time_renewal = (int)strtol(buffer,NULL,10);

  return config;
}

int create_udp_socket(unsigned int *port) {
  struct sockaddr_in serv_in_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0) { 
    printf("ERROR opening socket\n");
  }
  serv_in_addr.sin_family = AF_INET;
  serv_in_addr.sin_port = htons(*port);
  serv_in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t sock_len = sizeof(serv_in_addr);
  int result = bind(sockfd, (struct sockaddr *) &serv_in_addr, sock_len);
  if (result < 0) {
    printf("Binding error %i getting INET socket. Are you running as root?\n",result);
    return -1;
  }
  getsockname(sockfd, (struct sockaddr *) &serv_in_addr, (socklen_t *) &sock_len);
  *port = ntohs(serv_in_addr.sin_port);
  printf("Created udp socket for port %u and sockfd %i\n",ntohs(serv_in_addr.sin_port),sockfd);
  return sockfd;
}

void construct_message(char *message, T_STATE *current_state, const char *action) {
  char buffer[16];
  char *buf = buffer;
  strcpy(message, current_state->address);
  strcat(message, " ");
  strcat(message, action);
  strcat(message, " ");
  sprintf(buf, "%lli ", (long long int)current_state->price);
  strcat(message,  buf);
  bzero(buffer, 16);
  sprintf(buf, "%li ", current_state->payment_advance);
  strcat(message,  buf);
  bzero(buffer, 16);
  sprintf(buf, "%u", (unsigned int)current_state->time_expiration);
  strcat(message,  buf);
  bzero(buffer, 16);
}

int parse_message(T_STATE *current_state, char *message, T_LINK_INTERFACE link_interface, T_PAYMENT_INTERFACE payment_interface, T_NETWORK_INTERFACE network_interface, T_CONFIG *config) {
    char *argument = strsep(&message," ");
    char buffer[CHAR_BUFFER_LEN];
    char *current_message = buffer;
    if (argument == NULL) {
      printf("No message sent to receive.\n");
    } else if (strcmp(argument,"request") == 0) {
      if (current_state->status != DEFAULT) {
    printf("Cannot process request. Contract already in progress.\n");
      } else {
    evaluate_request(current_state, config);
    current_state->status = PROPOSE;
    construct_message(current_message, current_state, "propose");
    link_interface.link_send(current_state->interface_id, current_message);
      }
    } else if (strcmp(argument,"propose") == 0) {
      char *price_arg = strsep(&message," ");
      char *payment_advance = strsep(&message," ");
      char *time_expiration = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for propose.\n");
      } else if (payment_advance == NULL) {
    printf("Payment advance not provided for propose.\n");
      } else if (time_expiration == NULL) {
    printf("Time expiration not provided for propose.\n");
      } else if (current_state->status != REQUEST && current_state->status != REJECT) {
    printf("Not ready to receive propose.\n");
      } else {
    current_state->price = (int64_t)strtol(price_arg,NULL,10);
    current_state->payment_advance = strtol(payment_advance,NULL,10);
    current_state->time_expiration = (time_t)strtol(time_expiration,NULL,10);
    if (evaluate_propose(current_state, config)) {
      current_state->status = ACCEPT;
      strcpy(current_message, current_state->address);
      strcat(current_message, " accept");
      link_interface.link_send(current_state->interface_id, current_message);
      int64_t payment = MAX_PAYMENT;
      payment_interface.send_payment(current_state->interface_id, current_state->address, payment);
      //current_state->account->balance -= payment;
    } else {
      current_state->status = REJECT;
      construct_message(current_message, current_state, "reject");
      link_interface.link_send(current_state->interface_id, current_message);
    }
      }
    } else if (strcmp(argument,"accept") == 0) {
      if (current_state->status != PROPOSE) {
        printf("Not ready to receive accept.\n");
      } else {
        long int grace_period_data = config->grace_period_price / current_state->price;
        network_interface.gate_interface(current_state->interface_id->ip_addr_dst, current_state->address, time(NULL) + config->grace_period_time, grace_period_data);
        //decrement balance here
        current_state->status = BEGIN;
      }
    } else if (strcmp(argument,"reject") == 0) {
      char *price_arg = strsep(&message," ");
      char *payment_advance = strsep(&message," ");
      char *time_expiration = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for reject.\n");
      } else if (payment_advance == NULL) {
    printf("Payment advance not provided for propose.\n");
      } else if (time_expiration == NULL) {
    printf("Time expiration not provided for propose.\n");
      } else if (current_state->status != PROPOSE) {
    printf("Not ready to receive reject.");
      } else {
    current_state->price = (int64_t)strtol(price_arg,NULL,10);
    current_state->payment_advance = strtol(payment_advance,NULL,10);
    current_state->time_expiration = (time_t)strtol(time_expiration,NULL,10);
    evaluate_request(current_state, config);
    construct_message(current_message, current_state, "propose");
    link_interface.link_send(current_state->interface_id, current_message);
    current_state->status = PROPOSE;
      }
    } else if (strcmp(argument,"payment") == 0) {
      char *price_arg = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for payment.\n");
      } else if (current_state->status != BEGIN) {
    printf("Not ready to receive payment.\n");
      } else {
    current_state->account->balance += (int64_t)strtol(price_arg,NULL,10);
    if (deliver_service(current_state)) {
      network_interface.gate_interface(current_state->interface_id->ip_addr_dst, current_state->address, current_state->time_expiration, CONTRACT_DATA_SIZE);
    }
      }
    } else {
      printf("Invalid message type.\n");
    }
}

static T_STATE* find_state(T_STATE states[], int *new_contract, T_ACCOUNT accounts[], int *new_account, struct interface_id_udp *interface_id, char *address) {
  T_STATE *current_state = NULL;
  T_ACCOUNT *current_account = NULL;
  int i;
  for (i = 0; i < *new_contract; i++) {
    if (states[i].interface_id != NULL && strcmp(states[i].interface_id->ip_addr_dst,interface_id->ip_addr_dst) == 0) {
      if (strcmp(states[i].address,address) == 0) {
        current_state = &states[i];
        printf("Found previous state for identifier %s and address %s\n",interface_id->ip_addr_dst, address);
      }
      current_account = states[i].account;
    }
  }
  if (current_state == NULL) {
    current_state = &states[*new_contract];
    *new_contract = *new_contract + 1;
    current_state->interface_id = interface_id;
    strcpy(current_state->address, address);
    current_state->status = DEFAULT;
    current_state->price = 0;
    if (current_account == NULL) {
      current_state->account = &accounts[*new_account];
      *new_account = *new_account + 1;
    } else {
      current_state->account = current_account;
    }
    printf("Creating new state for identifier %s and address %s\n",interface_id->ip_addr_dst, address);
  }

  return current_state;
}

int start(bool quiet)
{
  //Create interface arrays
  T_LINK_INTERFACE link_interfaces[LINK_INTERFACES_NUMBER];
  T_NETWORK_INTERFACE network_interfaces[NETWORK_INTERFACES_NUMBER];
  T_PAYMENT_INTERFACE payment_interfaces[PAYMENT_INTERFACES_NUMBER];

  //Populate interface arrays
  link_interfaces[LINK_INTERFACE_TEST_IDENTIFIER] = link_test_interface();
  link_interfaces[LINK_INTERFACE_UDP_IDENTIFIER] = link_udp_interface();
  network_interfaces[NETWORK_INTERFACE_TEST_IDENTIFIER] = network_test_interface();
  network_interfaces[NETWORK_INTERFACE_IPV4_IDENTIFIER] = network_ipv4_interface();
  payment_interfaces[PAYMENT_INTERFACE_TEST_IDENTIFIER] = payment_test_interface();
  payment_interfaces[PAYMENT_INTERFACE_SIMULATE_IDENTIFIER] = payment_simulate_interface();

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
    /* Log the failure */
    exit(EXIT_FAILURE);
  }

  /* Change the current working directory */
  if ((chdir("/")) < 0) {
    /* Log the failure */
    exit(EXIT_FAILURE);
  }

  if (quiet) {
    close(STDIN_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
      printf("failed to reopen stdin while daemonizing (errno=%d)",errno);
      exit(EXIT_FAILURE);
    }
    int logfile_fileno = open(LOG_PATH,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP);
    if (logfile_fileno == -1) {
      printf("failed to open logfile (errno=%d)",errno);
      exit(EXIT_FAILURE);
    }
    dup2(logfile_fileno,STDOUT_FILENO);
    dup2(logfile_fileno,STDERR_FILENO);
    close(logfile_fileno);
  }

  int cli_sockfd, link_sockfd;
  socklen_t clilen;
  int result, n;

  //Set up socket for CLI communication
  int unix_udp_port = UNIX_UDP_PORT;
  cli_sockfd = create_udp_socket(&unix_udp_port);

  //Set up socket for INET communication
  int default_port = LINK_UDP_DEFAULT_PORT;
  link_sockfd = create_udp_socket(&default_port);

  //Set up loop for events
  T_STATE states[MAX_CONTRACTS];
  struct interface_id_udp interface_ids[MAX_CONNECTIONS];
  T_ACCOUNT accounts[MAX_ACCOUNTS];
  int new_contract = 0;
  int new_connection = 0;
  int new_account = 0;
  int command_result = 0;
  fd_set readfds;
  int maxfd, fd;
  unsigned int i;
  int status;
  struct sockaddr_in sock_addr;
  socklen_t sock_len = sizeof(sock_addr);

  pid_t net_pid = network_interface.network_init(states, &new_contract);

  int saddr_size, data_size;
  struct sockaddr saddr;
  int scan_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if(scan_sockfd < 0)
  {
    printf("Error creating socket for sniffing packets\n");
    return 1;
  }
  char buffer[PACKET_BUFFER_SIZE];

  while(command_result > -1) {
    bzero(buffer,PACKET_BUFFER_SIZE);
    data_size = 0;
    int fds[new_connection + 3];
    for (i = 0; i < new_connection; i++) {
      fds[i] = interface_ids[i].sockfd;
    }
    fds[new_connection] = cli_sockfd;
    fds[new_connection + 1] = link_sockfd;
    fds[new_connection + 2] = scan_sockfd;

    for (i = 0; i < new_connection + 3; i++) {
      data_size = recvfrom(fds[i] , buffer , PACKET_BUFFER_SIZE , MSG_DONTWAIT , &saddr , (socklen_t*)&saddr_size);
      if (data_size > 0) {
        fd = fds[i];
        break;
      }
    }
    
    if (data_size > 0) {
      if (fd == scan_sockfd) {
        char src_address[CHAR_BUFFER_LEN];
        char dst_address[CHAR_BUFFER_LEN];
        char next_hop_address[CHAR_BUFFER_LEN];
        unsigned int packet_size;
         
        if (network_interface.sniff_datagram(buffer,src_address,dst_address,next_hop_address,config.ngp_interface,&packet_size) == 1) {
          struct interface_id_udp *current_interface = link_interface.link_find_interface(interface_ids, &new_connection, 0, src_address, next_hop_address);
          T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, dst_address);
          current_state->bytes_sent += packet_size;

          if (current_state->status == DEFAULT || (current_state->bytes_sent + config.data_renewal > CONTRACT_DATA_SIZE) || (current_state->time_expiration - time(NULL) < config.time_renewal)) {
            current_state->status = REQUEST;
            char message[CHAR_BUFFER_LEN];
            sprintf(&message[0],"%s request",dst_address);
            link_interface.link_send(current_interface, message);
          }
        }
      } else if (fd == cli_sockfd) {
        char *message = buffer;
        char *argument = strsep(&message," ");

        if (strcmp(argument,"test") == 0) {
          printf("Test OK\n");
        }
        else if (strcmp(argument,"stop") == 0) {
          printf("Daemon stopping.\n");
          command_result = -1;
        }
        else if (strcmp(argument,"send") == 0) {
          char *ip_addr_src = strsep(&message," ");
          char *ip_addr_dst = strsep(&message," ");
          char address_with_message[CHAR_BUFFER_LEN];
          strcpy(address_with_message, message);
          char *address = strsep(&message," ");
          if (ip_addr_src == NULL) {
            printf("No ip_addr_src id provided.\n");
          } else if (ip_addr_dst == NULL) {
            printf("No ip_addr_dst id provided.\n");
          } else if (address == NULL) {
            printf("No address provided.\n");
          } else {
            struct interface_id_udp *current_interface = link_interface.link_find_interface(interface_ids, &new_connection, 0, ip_addr_src, ip_addr_dst);
            if (current_interface == NULL) {
              printf("Unable to find current interface for argument %s\n",argument);
            } else {
              T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, address);

              argument = strsep(&message," ");
              if (argument == NULL) {
                printf("No message sent to receive.\n");
              } else if (strcmp(argument,"request") == 0) {
                current_state->status = REQUEST;
                link_interface.link_send(current_interface, address_with_message);
              }
            }
          }
        }
        else if (strcmp(argument,"receive") == 0) {
          char *message = buffer;
          struct interface_id_udp *current_interface = link_interface.link_receive(interface_ids, &new_connection, 0, &message);

          char *address = strsep(&message," ");
          T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, address);
          parse_message(current_state, message, link_interface, payment_interface, network_interface, &config);
        }
        else {
          printf("Invalid command sent to server.\n");
        }
      } else {
        char *message = buffer;
        struct interface_id_udp *current_interface = link_interface.link_receive(interface_ids, &new_connection, fd, &message);
        if (current_interface == NULL) {
          printf("Unable to find current interface\n");
        } else {
          printf("About to parse message %s\n",message);
          char *address = strsep(&message," ");
          T_STATE *current_state = find_state(states, &new_contract, accounts, &new_account, current_interface, address);
          if (current_state == NULL) {
            printf("Unable to find current state\n");
          } else {
            parse_message(current_state, message, link_interface, payment_interface, network_interface, &config);
          }
        }
      }
    }

    if (n < 0) {
      printf("ERROR writing to socket\n");
      command_result = -1;
    }
  }

  for (i = 0; i < new_connection; i++) {
    close(interface_ids[i].sockfd);
  }
  close(cli_sockfd);
  close(link_sockfd);
  close(scan_sockfd);
  network_interface.network_destroy(net_pid);
  exit(EXIT_SUCCESS);
  return 0;
}

int send_cli_message(char *message)
{
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(UNIX_UDP_PORT);
  inet_aton("127.0.0.1", &serv_addr.sin_addr);
  int result = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (result < 0) {
    printf("send cli message failed\n");
    return 1;
  }
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
    return start(args >= 3 && strcmp(argv[2],"-q") == 0);
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

