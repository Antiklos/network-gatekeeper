#include <stdio.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.gate_interface = &gate_interface_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

void network_ipv4_init() {
  //Probably want to turn on DHCP to allow for automatic network configuration
  //Also, find a way (spawn daemon? probably) to query iptables for packets passed through the open windows
  system("iptables -P FORWARD DROP");
}

void gate_interface_ipv4(char *src_addr, char *dst_addr, time_t time_expiration) {
  //Try out time and connbytes params for iptables to control the data and timestamps
  printf("iptables -I FORWARD -i $(\"ip -o route get %s | awk '{ print $3 }'\") -d %s -m time --datestop $(date -d @%i +'%%Y%%m%%d%%H%%M%%S') -j ACCEPT",src_addr,dst_addr,(int)time_expiration);
}

void network_ipv4_destroy() {
  //Shut down daemon referenced above
  system("iptables -F");
  system("iptables -P FORWARD ACCEPT");
}


