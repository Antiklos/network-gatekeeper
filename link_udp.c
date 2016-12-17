#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>

#include "link_udp.h"

T_LINK_INTERFACE link_udp_interface() {
  T_LINK_INTERFACE interface;
  interface.link_init = &link_udp_init;
  interface.link_receive = &link_receive_udp;
  interface.link_send = &link_send_udp;
  interface.link_destroy = &link_udp_destroy;
  return interface;
}

void link_udp_init(T_INTERFACE interfaces[], int *new_connection, char *ignore_interface) {
  struct ifaddrs *addrs, *tmpaddr;
  getifaddrs(&addrs);
  tmpaddr = addrs;

  while (tmpaddr)
  {
    if (tmpaddr->ifa_addr != NULL && tmpaddr->ifa_addr->sa_family == AF_INET && strcmp(tmpaddr->ifa_name, "lo") != 0 && strcmp(tmpaddr->ifa_name, ignore_interface) != 0)
    {
        struct sockaddr_in *lAddr = (struct sockaddr_in *)tmpaddr->ifa_addr;
        strcpy(interfaces[*new_connection].interface_id, tmpaddr->ifa_name);
        strcpy(interfaces[*new_connection].net_addr_local, inet_ntoa(lAddr->sin_addr));
        
        struct sockaddr_in *netmask = (struct sockaddr_in *)tmpaddr->ifa_netmask;
        unsigned long netmask_raw = netmask->sin_addr.s_addr;
        unsigned long addr_dst = lAddr->sin_addr.s_addr | ~netmask_raw;
        struct in_addr *addr_remote = (struct in_addr *)&addr_dst;
        strcpy(interfaces[*new_connection].net_addr_remote, inet_ntoa(*addr_remote));
        interfaces[*new_connection].broadcast = true;
        interfaces[*new_connection].sockfd = create_udp_socket(interfaces[*new_connection].interface_id, false);

        interfaces[*new_connection].scan_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interfaces[*new_connection].interface_id);
        int result = ioctl(interfaces[*new_connection].scan_sockfd , SIOCGIFINDEX , &ifr);
        struct sockaddr_ll addr;
        memset(&addr, 0, sizeof(addr));
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = ifr.ifr_ifindex; 
        addr.sll_protocol = htons(ETH_P_IP);
        result = bind(interfaces[*new_connection].scan_sockfd,(struct sockaddr *) &addr, sizeof(addr));
        *new_connection = *new_connection + 1;
    }

    tmpaddr = tmpaddr->ifa_next;
  }

  freeifaddrs(addrs);
}

T_INTERFACE* link_receive_udp(T_INTERFACE *current_interface, char** message) {
  char *ip_addr = strsep(message," ");
  if (strcmp(ip_addr,current_interface->net_addr_local) == 0) {
    return NULL;
  }
  printf("Received raw message: %s %s\n",ip_addr,*message);
  current_interface->broadcast = 0;
  strcpy(current_interface->net_addr_remote, ip_addr);
  return current_interface;
}

void link_send_udp(T_INTERFACE *interface, char *message) {
  char buffer[CHAR_BUFFER_LEN];
  sprintf(buffer, "%s %s", interface->net_addr_local, message);
  message = buffer;

  printf("About to send raw message: %s\n",message);
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (interface->broadcast) {
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(LINK_UDP_DEFAULT_PORT);
  inet_aton(interface->net_addr_remote, &serv_addr.sin_addr);
  int result = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (result < 0) {
    printf("send_request_udp failed with errno %i\n", errno);
  }
}

void link_udp_destroy() {
  printf("Executing link_stop_destroy\n");
}


