#include <stdio.h>
#include <linux/if_ether.h>
#include <linux/ip.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.gate_interface = &gate_interface_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

pid_t network_ipv4_init() {
  //Probably want to turn on DHCP to allow for automatic network configuration
  //Also, find a way (spawn daemon? probably) to query iptables for packets passed through the open windows
  system("iptables -P FORWARD DROP");
  //Make the interfaces that we whitelist configurable
  system("iptables -I FORWARD -i wlan0 -j ACCEPT");

  pid_t pid,sid;

  pid = fork();
  if (pid < 0) {
    printf("Failure to spawn network daemon\n");
    exit(EXIT_FAILURE);
  } 
  if (pid > 0) {
    return pid;
  }

  umask(0);
  sid = setsid();
  if (sid < 0) {
    printf("Failure to set session id on network daemon\n");
    exit(EXIT_FAILURE);
  }

  if ((chdir("/")) < 0) {
    exit(EXIT_FAILURE);
  }

  int saddr_size , data_size;
  struct sockaddr saddr;
  int sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL));
  setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , "eth0" , strlen("eth0")+ 1 );
  if(sock_raw < 0)
  {
    printf("Error creating socket for sniffing packets\n");
    exit(EXIT_FAILURE);
  }
  char buffer[PACKET_BUFFER_SIZE];
  
  while(1) {
    saddr_size = sizeof saddr;
    data_size = recvfrom(sock_raw , buffer , PACKET_BUFFER_SIZE , 0 , &saddr , (socklen_t*)&saddr_size);
    if(data_size <0 )
    {
      printf("Recvfrom error , failed to get packets\n");
      exit(EXIT_FAILURE);
    }

    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    struct sockaddr_in src_addr,dst_addr;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sin_addr.s_addr = iph->saddr;
     
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_addr.s_addr = iph->daddr;

    printf("Catching packet with src %s and dst %s\n",inet_ntoa(src_addr.sin_addr),inet_ntoa(dst_addr.sin_addr));
  }
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
  kill(net_pid,SIGKILL);
  system("iptables -F");
  system("iptables -P FORWARD ACCEPT");
}


