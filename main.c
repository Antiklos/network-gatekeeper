#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <linux/if_ether.h>
#include <linux/ip.h>

#include "main.h"
#include "link_test.c"
#include "link_udp.c"
#include "network_test.c"
#include "network_ipv4.c"
#include "payment_test.c"
#include "payment_simulate.c"
#include "contract.c"

static T_CONFIG read_config()
{
  GKeyFile* gkf = g_key_file_new();
  GError* error = NULL;

  char* filename = (gchar*)malloc(256 * sizeof(gchar));
  strcpy(filename, "net.conf");
  const gchar* conf_file = filename;

  if (!g_key_file_load_from_file(gkf, conf_file, G_KEY_FILE_NONE, &error)){
    fprintf (stderr, "Could not read config file %s\n", conf_file);
  }

  gchar *link_value = g_key_file_get_value(gkf, "Group", "LINK_INTERFACE", NULL);
  gchar *network_value = g_key_file_get_value(gkf, "Group", "NETWORK_INTERFACE", NULL);
  gchar *payment_value = g_key_file_get_value(gkf, "Group", "PAYMENT_INTERFACE", NULL);
  gchar *user_value = g_key_file_get_value(gkf, "Group", "USER_INTERFACE", NULL);

  g_key_file_free(gkf);
  free(filename);

  T_CONFIG config;
  config.link_interface = (int)strtol(link_value,NULL,10);
  config.network_interface = (int)strtol(network_value,NULL,10);
  config.payment_interface = (int)strtol(payment_value,NULL,10);
  return config;
}

static int write_buffer(int sockfd, const char* message)
{
  return write(sockfd,message,strlen(message));
}

struct interface_id_udp* find_interface(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char *ip_addr_src, char *ip_addr_dst) {
    struct interface_id_udp *current_interface = NULL;
    int i;
    for (i = 0; i < *new_connection; i++) {
      if (interfaces[i].sockfd == sockfd) {
      current_interface = &interfaces[i];
      printf("Found previous interface for ip_addr %s outport %u inport %u and sockfd %i\n",
        current_interface->ip_addr_dst, current_interface->outgoing_port, current_interface->incoming_port, current_interface->sockfd);
      //break;
      }
    }
    if (current_interface == NULL) {
      if (ip_addr_dst == NULL || ip_addr_dst == "") {
        printf("Must provide ip_addr for new interfaces\n");
        return NULL;
      }
      current_interface = &interfaces[*new_connection];
      *new_connection = *new_connection + 1;
      if (ip_addr_src != NULL) {
        strcpy(current_interface->ip_addr_src, ip_addr_src);
      }
      strcpy(current_interface->ip_addr_dst, ip_addr_dst);
      current_interface->outgoing_port = LINK_UDP_DEFAULT_PORT;
      current_interface->sockfd = create_udp_socket(&current_interface->incoming_port);
      if (current_interface->sockfd < 0) {
        return NULL;
      }
      printf("Creating new interface for ip_addr %s outport %u inport %u and sockfd %i\n",
        current_interface->ip_addr_dst, current_interface->outgoing_port, current_interface->incoming_port, current_interface->sockfd);
    }

    return current_interface;
}

void link_send_message(struct interface_id_udp *interface_id, char *message) {
  printf("About to send message: %s\n",message);
  char buffer[CHAR_BUFFER_LEN];
  strcpy(buffer, interface_id->ip_addr_src);
  char port[8];
  sprintf(port, " %u ", interface_id->incoming_port);
  strcat(buffer, port);
  strcat(buffer, message);
  message = buffer;

  printf("About to send raw message: %s\n",message);
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(interface_id->outgoing_port);
  inet_aton(interface_id->ip_addr_dst, &serv_addr.sin_addr);
  int result = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (result < 0) {
    printf("send_request_udp failed\n");
  }
}

struct interface_id_udp* link_receive_message(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char** message) {
  printf("Received raw message: %s\n",*message);
  char *ip_addr = strsep(message," ");
  if (ip_addr == NULL) {
    printf("No ip_addr provided.\n");
    return NULL;
  }
  char *port = strsep(message," ");
  if (port == NULL) {
    printf("No port provided.\n");
    return NULL;
  }
  struct interface_id_udp *current_interface = find_interface(interfaces, new_connection, sockfd, NULL, ip_addr);
  current_interface->outgoing_port = (unsigned int)strtol(port,NULL,10);
  return current_interface;
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

int parse_message(T_STATE *current_state, char *message, T_PAYMENT_INTERFACE payment_interface, T_NETWORK_INTERFACE network_interface) {
    char *argument = strsep(&message," ");
    char buffer[CHAR_BUFFER_LEN];
    char *current_message = buffer;
    if (argument == NULL) {
      printf("No message sent to receive.\n");
    } else if (strcmp(argument,"request") == 0) {
      if (current_state->status != DEFAULT) {
    printf("Cannot process request. Contract already in progress.\n");
      } else {
    evaluate_request(current_state);
    current_state->status = PROPOSE;
    construct_message(current_message, current_state, "propose");
    link_send_message(current_state->interface_id, current_message);
    //link_interface.send_propose(current_state->interface_id->ip_addr, current_state->address,
      //current_state->price, current_state->payment_advance, current_state->time_expiration);
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
    if (evaluate_propose(current_state)) {
      current_state->status = ACCEPT;
      strcpy(current_message, current_state->address);
      strcat(current_message, " accept");
      link_send_message(current_state->interface_id, current_message);
      //link_interface.send_accept(current_state->interface_id->ip_addr, current_state->address);
      int64_t payment = MAX_PAYMENT;
      strcpy(current_message, current_state->address);
      strcat(current_message, " payment ");
      char payment_buf[CHAR_BUFFER_LEN];
      char *payment_buffer = payment_buf;
      sprintf(payment_buffer, "%lli ", (long long int)payment);
      strcat(current_message, payment_buffer);
      payment_interface.send_payment(current_state->interface_id->ip_addr_dst, current_state->address, payment);
      link_send_message(current_state->interface_id, current_message);
      current_state->payment_sent += payment;
    } else {
      current_state->status = REJECT;
      construct_message(current_message, current_state, "reject");
      link_send_message(current_state->interface_id, current_message);
      //link_interface.send_reject(current_state->interface_id->ip_addr, current_state->address,
        //current_state->price, current_state->payment_advance, current_state->time_expiration);
    }
      }
    } else if (strcmp(argument,"accept") == 0) {
      if (current_state->status != PROPOSE) {
    printf("Not ready to receive accept.\n");
      } else {
    current_state->status = PAYMENT;
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
    evaluate_reject(current_state);
    construct_message(current_message, current_state, "propose");
    link_send_message(current_state->interface_id, current_message);
    //link_interface.send_propose(current_state->interface_id->ip_addr, current_state->address,
      //current_state->price, current_state->payment_advance, current_state->time_expiration);
    current_state->status = PROPOSE;
      }
    } else if (strcmp(argument,"begin") == 0) {
      if (current_state->status != ACCEPT) {
    printf("Not ready to receive begin.\n");
      } else {
    current_state->status = COUNT_PACKETS;
    //Is this really necessary? Commenting out for now
    //network_interface.gate_interface(current_state->interface_id->ip_addr_dst, current_state->address, true);
      }
    } else if (strcmp(argument,"payment") == 0) {
      char *price_arg = strsep(&message," ");
      if (price_arg == NULL) {
    printf("Price not provided for payment.\n");
      } else if (current_state->status != PAYMENT && current_state->status != BEGIN) {
    printf("Not ready to receive payment.\n");
      } else {
    current_state->payment_sent += (int64_t)strtol(price_arg,NULL,10);
    if (deliver_service(current_state)) {
      network_interface.gate_interface(current_state->interface_id->ip_addr_dst, current_state->address, current_state->time_expiration, 4096);
      if (current_state->status != BEGIN) {
        current_state->status = BEGIN;
        strcpy(current_message, current_state->address);
        strcat(current_message, " begin");
        link_send_message(current_state->interface_id, current_message);
        //link_interface.send_begin(current_state->interface_id->ip_addr, current_state->address);
      }
    }
      }
    } else if (strcmp(argument,"count_packets") == 0) {
      char *count = strsep(&message," ");
      if (count == NULL) {
    printf("Packet count not provided.\n");
      } else if (current_state->status != BEGIN && current_state->status != COUNT_PACKETS && current_state->status != STOP) {
    printf("Not ready to count packets.\n");
      } else {
    current_state->packets_delivered = strtol(count,NULL,10);
    if (!deliver_service(current_state)) {
      //Pretty sure this is not necessary
      //network_interface.gate_interface(current_state->interface_id->ip_addr_dst, current_state->address, false);
    }

    if (current_state->status == COUNT_PACKETS && renew_service(current_state)) {
      int64_t payment = MAX_PAYMENT;
      strcpy(current_message, current_state->address);
      strcat(current_message, " payment ");
      char payment_buf[CHAR_BUFFER_LEN];
      char *payment_buffer = payment_buf;
      sprintf(payment_buffer, "%lli ", (long long int)payment);
      strcat(current_message, payment_buffer);
      payment_interface.send_payment(current_state->interface_id->ip_addr_dst, current_state->address, payment);
      link_send_message(current_state->interface_id, current_message);
      current_state->payment_sent += payment;
    }
      }
    } else {
      printf("Invalid message type.\n");
    }
}

static T_STATE* find_state(T_STATE states[], int *new_contract, struct interface_id_udp *interface_id, char *address) {
  T_STATE *current_state = NULL;
  int i;
  for (i = 0; i < *new_contract; i++) {
    if (states[i].interface_id != NULL && strcmp(states[i].interface_id->ip_addr_dst,interface_id->ip_addr_dst) == 0 && strcmp(states[i].address,address) == 0) {
      current_state = &states[i];
      printf("Found previous state for identifier %s and address %s\n",interface_id->ip_addr_dst, address);
      //break;
    }
  }
  if (current_state == NULL) {
    current_state = &states[*new_contract];
    *new_contract = *new_contract + 1;
    current_state->interface_id = interface_id;
    strcpy(current_state->address, address);
    current_state->status = DEFAULT;
    current_state->price = 0;
    current_state->payment_sent = 0;
    current_state->packets_delivered = 0;
    printf("Creating new state for identifier %s and address %s\n",interface_id->ip_addr_dst, address);
  }

  return current_state;
}

bool address_exists(T_STATE states[], int new_contract, char *address) {
  int i;
  for(i = 0; i < new_contract; i++) {
    if (strcmp(states[i].address, address) == 0) {
      return true;
    }
  }
  return false;
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
  T_CONFIG config = read_config();

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
  int new_contract = 0;
  int new_connection = 0;
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
          struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
          struct sockaddr_in src_addr,dst_addr;
     
          memset(&src_addr, 0, sizeof(src_addr));
          src_addr.sin_addr.s_addr = iph->saddr;

          memset(&dst_addr, 0, sizeof(dst_addr));
          dst_addr.sin_addr.s_addr = iph->daddr;

          char src_address[CHAR_BUFFER_LEN];
          strcpy(src_address, inet_ntoa(src_addr.sin_addr));
          char dst_address[CHAR_BUFFER_LEN];
          strcpy(dst_address, inet_ntoa(dst_addr.sin_addr));

          if (!address_exists(states, new_contract, dst_address)) {
            char msg_buffer[CHAR_BUFFER_LEN];
            char *msg_buf = msg_buffer;
            sprintf(msg_buf, "send %s 192.168.50.10 %s request",src_address,dst_address);
            send_cli_message(msg_buf);
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
            struct interface_id_udp *current_interface = find_interface(interface_ids, &new_connection, 0, ip_addr_src, ip_addr_dst);
            if (current_interface == NULL) {
              printf("Unable to find current interface for argument %s\n",argument);
            } else {
              T_STATE *current_state = find_state(states, &new_contract, current_interface, address);

              argument = strsep(&message," ");
              if (argument == NULL) {
                printf("No message sent to receive.\n");
              } else if (strcmp(argument,"request") == 0) {
                current_state->status = REQUEST;
                link_send_message(current_interface, address_with_message);
              } else if (strcmp(argument,"stop") == 0) {
                if (current_state->status != COUNT_PACKETS) {
                  printf("Not ready to stop contract.\n");
                } else {
                  current_state->status = STOP;
                }
              }
            }
          }
        }
        else if (strcmp(argument,"receive") == 0) {
          char *message = buffer;
          struct interface_id_udp *current_interface = link_receive_message(interface_ids, &new_connection, 0, &message);

          char *address = strsep(&message," ");
          T_STATE *current_state = find_state(states, &new_contract, current_interface, address);
          parse_message(current_state, message, payment_interface, network_interface);
        }
        else {
          printf("Invalid command sent to server.\n");
        }
      } else {
        char *message = buffer;
        struct interface_id_udp *current_interface = link_receive_message(interface_ids, &new_connection, fd, &message);
        //printf("Received message with address %s\n",inet_ntoa(sock_addr.sin_addr));
        if (current_interface == NULL) {
          printf("Unable to find current interface\n");
        } else {
          printf("About to parse message %s\n",message);
          char *address = strsep(&message," ");
          T_STATE *current_state = find_state(states, &new_contract, current_interface, address);
          if (current_state == NULL) {
            printf("Unable to find current state\n");
          } else {
            parse_message(current_state, message, payment_interface, network_interface);
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

