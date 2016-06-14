#include <stdio.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.gate_interface = &gate_interface_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

pid_t network_ipv4_init(T_STATE states[], int *new_connection) {
  //Probably want to turn on DHCP to allow for automatic network configuration
  //Also, find a way (spawn daemon? probably) to query iptables for packets passed through the open windows
  system("iptables -P FORWARD DROP");
  //Make the interfaces that we whitelist configurable
  system("iptables -I FORWARD -i wlan0 -j ACCEPT");
}

void gate_interface_ipv4(char *src_addr, char *dst_addr, time_t time_expiration, long int bytes) {
  char buffer[CHAR_BUFFER_LEN];
  char *time_string = buffer;
  struct tm* tm_info = gmtime(&time_expiration);
  strftime(time_string, 26, "%Y-%m-%dT%H:%M:%S", tm_info);
  char buf[CHAR_BUFFER_LEN];
  char *output = buf;
  sprintf(output, "iptables -I FORWARD -i $(ip -o route get %s | awk '{ print $3 }') -d %s -m time --datestop %s -m connbytes --connbytes 0:%li --connbytes-dir both --connbytes-mode bytes -j ACCEPT\n",src_addr,dst_addr,time_string,bytes);
  system(output);
}

void network_ipv4_destroy(pid_t net_pid) {
  system("iptables -F");
  system("iptables -P FORWARD ACCEPT");
}


