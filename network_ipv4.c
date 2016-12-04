#include <stdio.h>
#include <time.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.sniff_datagram = &sniff_datagram_ipv4;
  interface.gate_interface = &gate_interface_ipv4;
  interface.gate_address = &gate_address_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

void network_ipv4_init() {

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

void gate_interface_ipv4(char *interface_id, char *address) {
  char output[CHAR_BUFFER_LEN];
  sprintf(output, "iptables -I FORWARD -i %s -d %s -j DROP\n",interface_id, address);
  system(output);
}

void gate_address_ipv4(char *interface_id, char *dst_addr, time_t time_expiration, long int kb) {
  char time_string[CHAR_BUFFER_LEN];
  struct tm* tm_info = gmtime(&time_expiration);
  strftime(time_string, 26, "%Y-%m-%dT%H:%M:%S", tm_info);
  char output[CHAR_BUFFER_LEN];
  sprintf(output, "iptables -I FORWARD -i %s -d %s -m time --datestop %s -j ACCEPT\n",interface_id,dst_addr,time_string);
  system(output);
}

void network_ipv4_destroy() {
  system("iptables -F");
}


