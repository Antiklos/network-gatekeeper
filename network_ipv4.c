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
  //system("iptables -P FORWARD DROP");
}

void gate_interface_ipv4(char *interface_id, char *address, bool open) {
  //Try out time and connbytes params for iptables to control the data and timestamps
}

void network_ipv4_destroy() {
  //Shut down daemon referenced above
  system("iptables -F");
}


