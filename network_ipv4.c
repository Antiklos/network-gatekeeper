#include <stdio.h>
#include <time.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.sniff_datagram = &sniff_datagram_ipv4;
  interface.gate_interface = &gate_interface_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

pid_t network_ipv4_init(T_STATE states[], int *new_connection, char *ignore_interface) {
  //system("iptables -P FORWARD DROP");

  //char command[IFNAMSIZ + 64];
  //sprintf(command, "iptables -I FORWARD -i %s -j ACCEPT",ignore_interface);
  //system(command);
}

int sniff_datagram_ipv4(char *buffer, char *dst_address, unsigned int *packet_size) {
  struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
  struct sockaddr_in dst_addr, src_addr;

  memset(&dst_addr, 0, sizeof(struct sockaddr_in));
  dst_addr.sin_addr.s_addr = iph->daddr;


  memset(&src_addr, 0, sizeof(struct sockaddr_in));
  src_addr.sin_addr.s_addr = iph->saddr;

  strcpy(dst_address, inet_ntoa(dst_addr.sin_addr));
  *packet_size = iph->tot_len >> 8;
  return 1;
}

void gate_interface_ipv4(char *src_addr, char *dst_addr, time_t time_expiration, long int kb) {
  char buffer[CHAR_BUFFER_LEN];
  char *time_string = buffer;
  struct tm* tm_info = gmtime(&time_expiration);
  strftime(time_string, 26, "%Y-%m-%dT%H:%M:%S", tm_info);
  char buf[CHAR_BUFFER_LEN];
  char *output = buf;
  sprintf(output, "iptables -A FORWARD -i $(ip -o route get %s | awk '{ print $3 }') -d %s -m time --datestop %s -m connbytes --connbytes 0:%li --connbytes-dir both --connbytes-mode bytes -j ACCEPT\n",src_addr,dst_addr,time_string,kb*1024);
  system(output);
}

void network_ipv4_destroy(pid_t net_pid) {
  system("iptables -F");
  //system("iptables -P FORWARD ACCEPT");
}


