#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>

#include "main.h"
#include "link_test.c"
#include "link_udp.c"
#include "network_test.c"
#include "network_ipv4.c"
#include "payment_test.c"
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

static T_STATE* find_state(T_STATE states[], int *new_connection, char *interface_id, char *address) {
  T_STATE *current_state = NULL;
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    if (strcmp(states[i].interface_id,interface_id) == 0 && strcmp(states[i].address,address) == 0) {
      current_state = &states[i];
      printf("Found previous state for identifier %s and address %s\n",interface_id, address);
      //break;
    }
  }
  if (current_state == NULL) {
    current_state = &states[*new_connection];
    *new_connection = *new_connection + 1;
    strcpy(current_state->interface_id, interface_id);
    strcpy(current_state->address, address);
    current_state->status = DEFAULT;
    current_state->price = 0;
    current_state->payment_sent = 0;
    current_state->packets_delivered = 0;
    printf("Creating new state for identifier %s and address %s\n",interface_id, address);
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
  payment_interfaces[PAYMENT_INTERFACE_TEST_IDENTIFIER] = payment_test_interface();

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
  char buffer[256];
  int result, n;

  //Set up socket for CLI communication
  struct sockaddr_un serv_un_addr, current_addr;
  cli_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (cli_sockfd < 0) { 
    printf("ERROR opening socket\n");
  }
  serv_un_addr.sun_family = AF_UNIX;
  strncpy(serv_un_addr.sun_path, SOCK_PATH,strlen(SOCK_PATH));
  result = bind(cli_sockfd, (struct sockaddr *) &serv_un_addr, strlen(SOCK_PATH) + 2);
  if (result < 0) {
    printf("Binding error %i getting CLI socket. Are you running as root?\n",result);
    return 1;
  }

  //Set up socket for INET communication
  struct sockaddr_in serv_in_addr;
  link_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (link_sockfd < 0) { 
    printf("ERROR opening socket\n");
  }
  serv_in_addr.sin_family = AF_INET;
  serv_in_addr.sin_port = htons(10325);
  serv_in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  result = bind(link_sockfd, (struct sockaddr *) &serv_in_addr, sizeof(serv_in_addr));
  if (result < 0) {
    printf("Binding error %i getting INET socket. Are you running as root?\n",result);
    return 1;
  }

  //Set up loop for events
  T_STATE states[MAX_CONNECTIONS];
  int new_connection = 0;
  int command_result = 0;
  fd_set readfds;
  int maxfd, fd;
  unsigned int i;
  int status;
  while(command_result > -1) {
    listen(cli_sockfd,2);
    clilen = sizeof(current_addr);

    //loop through all available sockets for any incoming messages
    int fds[new_connection + 2];

    for (i = 0; i < new_connection; i++) {
      fds[i] = atoi(states[i].interface_id);
    }
    fds[new_connection] = cli_sockfd;
    fds[new_connection + 1] = link_sockfd;

    FD_ZERO(&readfds);
    maxfd = -1;
    for (i = 0; i < new_connection + 2; i++) {
        FD_SET(fds[i], &readfds);
        if (fds[i] > maxfd) {
            maxfd = fds[i];
        }
    }
    status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (status < 0) {
        printf("ERROR trying to read from an invalid socket\n");
        command_result = -1;
    }
    fd = -1;
    for (i = 0; i < new_connection + 2; i++)
        if (FD_ISSET(fds[i], &readfds)) {
            fd = fds[i];
            break;
        }
    if (fd == -1) {
      printf("ERROR trying to read from an invalid socket\n");
      command_result = -1;
    }
    else {
      bzero(buffer,256);
      if (fd == cli_sockfd) {
        int current_sockfd = accept(fd, (struct sockaddr *) &current_addr, &clilen);
        if (current_sockfd < 0) {
          printf("ERROR on accept\n");
          command_result = -1;
        }
        n = read(current_sockfd,buffer,256);
        if (n < 0) {
          printf("ERROR reading from socket\n");
          command_result = -1;
        }
      } else {
        n = recvfrom(fd, buffer, 256, 0, (struct sockaddr *) &current_addr, &clilen);
        if (n < 0) {
          continue;
        }
      }
    }

    //now operate on the message that was found
    printf("Received incoming message %s\n",buffer);

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
      char *interface_id = strsep(&message," ");
      char *address = strsep(&message," ");
      if (interface_id == NULL) {
        printf("No interface id provided.\n");
      } else if (address == NULL) {
        printf("No address provided.\n");
      } else {
        T_STATE *current_state = find_state(states, &new_connection, interface_id, address);

        argument = strsep(&message," ");
        if (argument == NULL) {
          printf("No message sent to receive.\n");
        } else if (strcmp(argument,"request") == 0) {
          current_state->status = REQUEST;
          link_interface.send_request(interface_id, address);
        } else if (strcmp(argument,"stop") == 0) {
          if (current_state->status != COUNT_PACKETS) {
            printf("Not ready to stop contract.\n");
          } else {
            current_state->status = STOP;
          }
        }
      }
    }
    else if (strcmp(argument,"receive") == 0) {
      char *interface_id = strsep(&message," ");
      char *address = strsep(&message," ");
      if (interface_id == NULL) {
        printf("No interface id provided.\n");
      } else if (address == NULL) {
        printf("No address provided.\n");
      } else {
        T_STATE *current_state = find_state(states, &new_connection, interface_id, address);

        argument = strsep(&message," ");
        if (argument == NULL) {
          printf("No message sent to receive.\n");
        } else if (strcmp(argument,"request") == 0) {
          if (current_state->status != DEFAULT) {
            printf("Cannot process request. Contract already in progress.\n");
          } else {
            evaluate_request(current_state);
            current_state->status = PROPOSE;
            link_interface.send_propose(current_state->interface_id, current_state->address,
              current_state->price, current_state->payment_advance, current_state->time_expiration);
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
              link_interface.send_accept(current_state->interface_id, current_state->address);
              int64_t payment = MAX_PAYMENT;
              payment_interface.send_payment(current_state->interface_id, current_state->address, payment);
              current_state->payment_sent += payment;
            } else {
              current_state->status = REJECT;
              link_interface.send_reject(current_state->interface_id, current_state->address,
                current_state->price, current_state->payment_advance, current_state->time_expiration);
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
            link_interface.send_propose(current_state->interface_id, current_state->address,
              current_state->price, current_state->payment_advance, current_state->time_expiration);
            current_state->status = PROPOSE;
          }
        } else if (strcmp(argument,"begin") == 0) {
          if (current_state->status != ACCEPT) {
            printf("Not ready to receive begin.\n");
          } else {
            current_state->status = COUNT_PACKETS;
            network_interface.gate_interface(current_state->interface_id, current_state->address, true);
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
              network_interface.gate_interface(current_state->interface_id, current_state->address, true);
              if (current_state->status != BEGIN) {
                current_state->status = BEGIN;
                link_interface.send_begin(current_state->interface_id, current_state->address);
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
              network_interface.gate_interface(current_state->interface_id, current_state->address, false);
            }

            if (current_state->status == COUNT_PACKETS && renew_service(current_state)) {
              int64_t payment = MAX_PAYMENT;
              payment_interface.send_payment(current_state->interface_id, current_state->address, payment);
              current_state->payment_sent += payment;
            }
          }
        } else {
          printf("Invalid message type.\n");
        }
      }
    }
    else {
      printf("Invalid command sent to server.\n");
    }

    if (n < 0) {
      printf("ERROR writing to socket\n");
      command_result = -1;
    }
  }

  close(cli_sockfd);
  close(link_sockfd);
  if (unlink(SOCK_PATH) < 0) {
    printf("ERROR deleting socket\n");
  }
  exit(EXIT_SUCCESS);
  return 0;
}

int send_cli_message(char *message)
{
  int sockfd, n;
  struct sockaddr_un serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  char buffer[256];
  bzero(buffer,256);
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
      printf("ERROR opening socket\n");
      exit(0);
  }
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCK_PATH, strlen(SOCK_PATH));
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
      printf("ERROR connecting. Are you running as root?\n");
      exit(0);
  }
  strncpy(buffer, message, strlen(message));
  n = write_buffer(sockfd,buffer);
  if (n < 0) {
       printf("ERROR writing to socket\n");
       exit(0);
  }
  close(sockfd);
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

