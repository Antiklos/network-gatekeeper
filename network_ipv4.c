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
  //Turn routing on but set up iptables to deny routing to all packets until gate_interface opens up specific addresses and interfaces
  //Probably want to use DHCP
  //Also, find a way (spawn daemon? probably) to query iptables for packets passed through the open windows
}

void gate_interface_ipv4(char *interface_id, char *address, bool open) {
  //Send rule to iptables to allow routing for this destination and interface
}

void network_ipv4_destroy() {
  //Shut down daemon and set iptables back to what it was before
}


