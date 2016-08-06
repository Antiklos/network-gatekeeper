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

struct interface_id_udp* link_find_interface_udp(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char *ip_addr_src, char *ip_addr_dst) {
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
      current_interface->incoming_port = 0;
      current_interface->sockfd = create_udp_socket(&current_interface->incoming_port);
      if (current_interface->sockfd < 0) {
        return NULL;
      }
      printf("Creating new interface for ip_addr %s outport %u inport %u and sockfd %i\n",
        current_interface->ip_addr_dst, current_interface->outgoing_port, current_interface->incoming_port, current_interface->sockfd);
    }

    return current_interface;
}

void link_udp_init() {
  printf("Executing link_test_init\n");
}

struct interface_id_udp* link_receive_udp(struct interface_id_udp interfaces[], int *new_connection, int sockfd, char** message) {
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
  struct interface_id_udp *current_interface = link_find_interface_udp(interfaces, new_connection, sockfd, NULL, ip_addr);
  current_interface->outgoing_port = (unsigned int)strtol(port,NULL,10);
  return current_interface;
}

void link_send_udp(struct interface_id_udp *interface_id, char *message) {
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

void link_udp_destroy() {
  printf("Executing link_stop_destroy\n");
}


