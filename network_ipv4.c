#include <stdio.h>

#include "network_ipv4.h"

T_NETWORK_INTERFACE network_ipv4_interface() {
  T_NETWORK_INTERFACE interface;
  interface.network_init = &network_ipv4_init;
  interface.sniff_datagram = &sniff_datagram_ipv4;
  interface.gate_interface = &gate_interface_ipv4;
  interface.network_destroy = &network_ipv4_destroy;
  return interface;
}

pid_t network_ipv4_init(T_STATE states[], int *new_connection) {
  system("iptables -P FORWARD DROP");
  //Make the interfaces that we whitelist configurable
  system("iptables -I FORWARD -i wlan0 -j ACCEPT");
}

static int get_local_ip_addr(char *address) {
  struct ifaddrs *addrs, *tmpaddr;
  getifaddrs(&addrs);
  tmpaddr = addrs;

  while (tmpaddr) 
  {
    if (tmpaddr->ifa_addr != NULL && tmpaddr->ifa_addr->sa_family == AF_INET)
    {
        struct sockaddr_in *pAddr = (struct sockaddr_in *)tmpaddr->ifa_addr;
        if (strcmp(tmpaddr->ifa_name,"eth0") == 0) {
          strcpy(address, inet_ntoa(pAddr->sin_addr));
          break;
        }
    }

    tmpaddr = tmpaddr->ifa_next;
  }

  freeifaddrs(addrs);
  return 0;
}

static int readNlSock(int sockFd, char *bufPtr, size_t buf_size, int seqNum, int pId)
{
  struct nlmsghdr *nlHdr;
  int readLen = 0, msgLen = 0;

  do
  {
    /* Recieve response from the kernel */
    if((readLen = recv(sockFd, bufPtr, buf_size - msgLen, 0)) < 0)
    {
      perror("SOCK READ: ");
      return -1;
    }

    nlHdr = (struct nlmsghdr *)bufPtr;

    /* Check if the header is valid */
    if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
    {
      perror("Error in recieved packet");
      return -1;
    }

    /* Check if the its the last message */
    if(nlHdr->nlmsg_type == NLMSG_DONE)
    {
      break;
    }
    else
    {
      /* Else move the pointer to buffer appropriately */
      bufPtr += readLen;
      msgLen += readLen;
    }

    /* Check if it's a multi part message */
    if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
    {
      /* return if it's not */
      break;
    }
  }
  while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

  return msgLen;
}

static int parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
  struct rtmsg *rtMsg;
  struct rtattr *rtAttr;
  int rtLen;

  rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

  /* If the route is not for AF_INET or does not belong to main routing table then return. */
  if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
    return -1;

  /* get the rtattr field */
  rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
  rtLen = RTM_PAYLOAD(nlHdr);

  for(; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen))
  {
    switch(rtAttr->rta_type)
    {
    case RTA_OIF:
      if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
      break;

    case RTA_GATEWAY:
      memcpy(&rtInfo->gateWay, RTA_DATA(rtAttr), sizeof(rtInfo->gateWay));
      break;

    case RTA_PREFSRC:
      memcpy(&rtInfo->srcAddr, RTA_DATA(rtAttr), sizeof(rtInfo->srcAddr));
      break;

    case RTA_DST:
      memcpy(&rtInfo->dstAddr, RTA_DATA(rtAttr), sizeof(rtInfo->dstAddr));
      break;
    }
  }

  return 0;
}

static int route_lookup(char *address, char *next_hop) {
  int found_gatewayip = 0;

  struct nlmsghdr *nlMsg;
  struct rtmsg *rtMsg;
  struct route_info route_info;
  char msgBuf[PACKET_BUFFER_SIZE]; // pretty large buffer

  int sock, len, msgSeq = 0;

  /* Create Socket */
  if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
  {
    perror("Socket Creation: ");
    return(-1);
  }

  /* Initialize the buffer */
  memset(msgBuf, 0, sizeof(msgBuf));

  /* point the header and the msg structure pointers into the buffer */
  nlMsg = (struct nlmsghdr *)msgBuf;
  rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

  /* Fill in the nlmsg header*/
  nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
  nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

  nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
  nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
  nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

  /* Send the request */
  if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
  {
    fprintf(stderr, "Write To Socket Failed...\n");
    return -1;
  }

  /* Read the response */
  if((len = readNlSock(sock, msgBuf, sizeof(msgBuf), msgSeq, getpid())) < 0)
  {
    fprintf(stderr, "Read From Socket Failed...\n");
    return -1;
  }

  /* Parse and print the response */
  for(; NLMSG_OK(nlMsg,len); nlMsg = NLMSG_NEXT(nlMsg,len))
  {
    memset(&route_info, 0, sizeof(route_info));
    if ( parseRoutes(nlMsg, &route_info) < 0 )
      continue;  // don't check route_info if it has not been set up

    strcpy(next_hop,inet_ntoa(route_info.gateWay));
    close(sock);
    return 1;
  }

  return found_gatewayip;
}

int sniff_datagram_ipv4(char *buffer, char *src_address, char *dst_address, char *next_hop, char *ngp_interface, unsigned int *packet_size) {
  struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
  struct sockaddr_in src_addr,dst_addr;

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.sin_addr.s_addr = iph->saddr;

  memset(&dst_addr, 0, sizeof(dst_addr));
  dst_addr.sin_addr.s_addr = iph->daddr;

  strcpy(src_address, inet_ntoa(src_addr.sin_addr));
  strcpy(dst_address, inet_ntoa(dst_addr.sin_addr));

  if (strcmp("127.0.0.1",dst_address) == 0 ||
      strcmp("255.255.255.255",dst_address) == 0 ||
      strcmp(dst_address,ngp_interface) == 0) {
    return 0;
  }

  char addr_buffer[CHAR_BUFFER_LEN];
  char *local_addr = addr_buffer;
  get_local_ip_addr(local_addr);

  if (strcmp(local_addr,dst_address) == 0) {
    return 0;
  }
  
  route_lookup(dst_address, next_hop);
  if (strcmp(ngp_interface,next_hop) == 0) {
    *packet_size = iph->tot_len >> 8;
    return 1;
  }
  return 0;
}

void gate_interface_ipv4(char *src_addr, char *dst_addr, time_t time_expiration, long int kb) {
  char buffer[CHAR_BUFFER_LEN];
  char *time_string = buffer;
  struct tm* tm_info = gmtime(&time_expiration);
  strftime(time_string, 26, "%Y-%m-%dT%H:%M:%S", tm_info);
  char buf[CHAR_BUFFER_LEN];
  char *output = buf;
  sprintf(output, "iptables -I FORWARD -i $(ip -o route get %s | awk '{ print $3 }') -d %s -m time --datestop %s -m connbytes --connbytes 0:%li --connbytes-dir both --connbytes-mode bytes -j ACCEPT\n",src_addr,dst_addr,time_string,kb*1024);
  system(output);
}

void network_ipv4_destroy(pid_t net_pid) {
  system("iptables -F");
  system("iptables -P FORWARD ACCEPT");
}


