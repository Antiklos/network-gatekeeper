#include <stdio.h>

#include "link_udp.h"

T_LINK_INTERFACE link_udp_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_udp_init;
  interface.link_find_interface = &link_find_interface_udp;
  interface.link_receive = &link_receive_udp;
  interface.link_send = &link_send_udp;
  interface.link_destroy = &link_udp_destroy;
  return interface;
}

T_INTERFACE* link_find_interface_udp(T_INTERFACE interfaces[], int *new_connection, int sockfd, char *interface_id, char *ip_addr_src, char *ip_addr_dst) {
    T_INTERFACE *current_interface = NULL;
    int i;
    for (i = 0; i < *new_connection; i++) {
      if (interfaces[i].sockfd == sockfd || strcmp(interfaces[i].net_addr_remote, ip_addr_dst) == 0) {
        current_interface = &interfaces[i];
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
        strcpy(current_interface->net_addr_local, ip_addr_src);
      }
      strcpy(current_interface->net_addr_remote, ip_addr_dst);
      current_interface->remote_port = LINK_UDP_DEFAULT_PORT;
      current_interface->local_port = 0;
      current_interface->sockfd = create_udp_socket(&current_interface->local_port);
      if (current_interface->sockfd < 0) {
        return NULL;
      }
      printf("Creating new interface for ip_addr %s outport %u inport %u and sockfd %i\n",
        current_interface->net_addr_remote, current_interface->remote_port, current_interface->local_port, current_interface->sockfd);
    }

    return current_interface;
}

void link_udp_init() {
  printf("Executing link_test_init\n");
}

T_INTERFACE* link_receive_udp(T_INTERFACE interfaces[], int *new_connection, int sockfd, char** message) {
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
  T_INTERFACE *current_interface = link_find_interface_udp(interfaces, new_connection, sockfd, NULL, NULL, ip_addr);
  current_interface->remote_port = (unsigned int)strtol(port,NULL,10);
  return current_interface;
}

void link_send_udp(T_INTERFACE *interface, char *message) {
  char buffer[CHAR_BUFFER_LEN];
  strcpy(buffer, interface->net_addr_local);
  char port[8];
  sprintf(port, " %u ", interface->local_port);
  strcat(buffer, port);
  strcat(buffer, message);
  message = buffer;

  printf("About to send raw message: %s\n",message);
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(interface->remote_port);
  inet_aton(interface->net_addr_remote, &serv_addr.sin_addr);
  int result = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (result < 0) {
    printf("send_request_udp failed\n");
  }
}

void link_udp_destroy() {
  printf("Executing link_stop_destroy\n");
}


