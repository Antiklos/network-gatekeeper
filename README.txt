Dependencies: gcc; glib-2.0
Tested only on Ubuntu 14.04

To get started right away with one machine, here's a guide on how to test it out:

1. Install dependencies: sudo apt-get install gcc glib-2.0
2. Compile the code: gcc main.c -o ngp -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -lglib-2.0
3. Start the server: sudo ./ngp start
4. Now test that the server is receiving messages on the local unix socket: sudo ./ngp server test
5. Try to send a message over the sockets that are opened up to UDP messages on your local machine: sudo ./ngp server "send 127.0.0.1 127.0.0.1 192.168.33.77 request"

You should get the following text on your console after the last command:
Created udp socket for port 55908 and sockfd 6
Creating new interface for ip_addr 127.0.0.1 outport 10325 inport 55908 and sockfd 6
Creating new state for identifier 127.0.0.1 and address 192.168.33.77
About to send message: 192.168.33.77 request
About to send raw message: 127.0.0.1 55908 192.168.33.77 request
Received raw message: 127.0.0.1 55908 192.168.33.77 request
Created udp socket for port 54101 and sockfd 8
Creating new interface for ip_addr 127.0.0.1 outport 10325 inport 54101 and sockfd 8
About to parse message 192.168.33.77 request
About to try to find state address 192.168.33.77
Found previous state for identifier 127.0.0.1 and address 192.168.33.77
Cannot process request. Contract already in progress.

Here's what's happening: We're writing the message "send 127.0.0.1 127.0.0.1 44325 192.168.33.77 request" to the unix socket.
We (the first 127.0.0.1) are going to request that the server (the second 127.0.0.1) route packets on our behalf to the destination (192.168.33.77).
In this example, the client selects port 55908 to listen for a response.
The server (which we have specified is going to be the same daemon as the client) receives the message and looks for an existing contract for the address 192.168.33.77.
The server finds it, but bails here, because it isn't allowed to send a "propose" message to something it knows it's already sent a "request" to.
An ngp daemon will never send a message to itself, but it's useful here to be able to get a quick look into the daemon in action without having to set it up on multiple machines.

6. Stop the daemon: sudo ./ngp stop


Message formats:

send (host_ip_addr) (dest_ip_addr) (port) (address) request
send (interface_id) (port) (address) stop
receive (interface_id) (port) (address) request
receive (interface_id) (port) (address) propose (price) (payment_advance) (time_expiration)
receive (interface_id) (port) (address) accept
receive (interface_id) (port) (address) reject (price) (payment_advance) (time_expiration)
receive (interface_id) (port) (address) begin

Example: "receive 127.0.0.1 10325 192.168.22.89 accept"

Definitions:

interface_id: This is a string that identifies the ngp link the message came in from. 
  For example, when the link is made using UDP, the interface_id will be an IP address.
address: This is a string that identifies the network level address destination that the traffic will be sent to as part of the contract with the client.
  For example, if the network interface is IPv4, an example address could be 192.168.10.44
price: This is the price per packet being negotiated
payment_advance: The client will be allowed to use this much worth of network bandwidth before being required to pay for it
time_expiration: The client must use the bandwidth allocated in this contract before this timestamp

What each message means:

Each message is an interaction between what we'll call a "client" and "server". A client wants access to the rest of the network through the server, and the server is willing to route the client's packets for a price.

-->request  This is the first message that the client will send to the server asking for a price for a certain network address
<--propose  The server responds with a proposed price and terms
-->accept   The client will either accept this price and terms after which the server will respond with begin, or
-->reject   The client may reject the request, offering a different price or terms. The server must respond with another propose
<--begin    The server responds, letting the client know that they may begin sending network packets

Features to add in:
- Test it out on a real network
- Implement the gating of network traffic from the network_ipv4 class using iptables
- Create a test message framework to spoof making payments
- Create a new payment class to implement sending and receiving bitcoin
- Implement the automatic server configuration, including enabling routing and DNS, probably using DHCP?
- Find a way for the client to anticipate when traffic is about to be sent to a destination, and then send the request before allowing sending traffic
- Implement use cases for renewing contracts
- Smarter client behavior for making payments just in time
- Smarter default client behavior for polling all interfaces and choosing the best deal
- GUI for config and other things

