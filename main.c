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
#include <sys/un.h>

#include "main.h"
#include "link_test.c"
#include "network_test.c"
#include "payment_test.c"

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

static int write_buffer(int sockfd, const char* message)
{
  return write(sockfd,message,strlen(message));
}

int daemon_command(int sockfd, socklen_t clilen, struct sockaddr_un cli_addr, char *buffer)
{
  listen(sockfd,2);
  clilen = sizeof(cli_addr);
  int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) { 
    printf("ERROR on accept\n");
    return -1;
  }
  bzero(buffer,256);
  int n = read(newsockfd,buffer,255);
  if (n < 0) {
    printf("ERROR reading from socket\n");
    return -1;
  }
  printf("Received command %s\n",buffer);

  if (strcmp(buffer,"test") == 0) {
    n = write_buffer(newsockfd,"Test OK");
  }
  else if (strcmp(buffer,"stop") == 0) {
    n = write_buffer(newsockfd,"Daemon stopping.");
    return -1;
  }
  else {
    n = write_buffer(newsockfd,"Invalid command sent to server.");
  }
  if (n < 0) {
    printf("ERROR writing to socket\n");
    return -1;
  }
}

int start()
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

  /* Our process ID and Session ID */
  pid_t pid, sid;

  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  /* If we got a good PID, then
     we can exit the parent process. */
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

  /* Open any logs here */
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

  //Set up socket for communication
  int sockfd, newsockfd;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_un serv_addr, cli_addr;
  int n;
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) { 
    printf("ERROR opening socket\n");
  }
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCK_PATH,strlen(SOCK_PATH));
  printf("Socket path: %s\n",serv_addr.sun_path);
  int result = bind(sockfd, (struct sockaddr *) &serv_addr, strlen(SOCK_PATH) + 2);
  if (result < 0) {
    printf("Binding error %i. Are you running as root?\n",result);
  }

  int command_result = 0;
  while(command_result > -1) {
    command_result = daemon_command(sockfd,clilen,cli_addr,buffer);
  }

  close(newsockfd);
  close(sockfd);
  if (unlink(SOCK_PATH) < 0) {
    printf("ERROR deleting socket\n");
  }
  exit(EXIT_SUCCESS);
  return 0;
}

int send_cli_message(char *message)
{
  int sockfd, portno, n;
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
  bzero(buffer,256);
  n = read(sockfd,buffer,255);
  if (n < 0) {
       printf("ERROR reading from socket\n");
       exit(0);
  }
  printf("%s\n",buffer);
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
    return start();
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

