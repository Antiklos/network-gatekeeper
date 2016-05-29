Dependencies: glib-2.0
To compile: gcc main.c -o ngp -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -lglib-2.0

To start daemon: ./ngp start

To test daemon: ./ngp server test

To stop daemon: ./ngp stop

To test receipt of a certain message from the link: ./ngp server "[message]"
  This will write the message to a socket that the server is listening on.

Message formats:

send (interface_id) (port) (address) request
send (interface_id) (port) (address) stop
receive (interface_id) (port) (address) request
receive (interface_id) (port) (address) propose (price) (payment_advance) (time_expiration)
receive (interface_id) (port) (address) accept
receive (interface_id) (port) (address) reject (price) (payment_advance) (time_expiration)
receive (interface_id) (port) (address) begin

Example: "receive 1234 propose 45"

Definitions:

interface_id: This is a string that identifies the ngp link the message came in from. 
  For example, when the link is made using UDP, the interface_id will be an IP address.
address: This is a string that identifies the network level address destination that the traffic will be sent to as part of the contract with the client.
  For example, if the network interface is IPv4, an example address could be 192.168.10.44
price: This is the price per packet being negotiated

What each message means:

Each message is an interaction between what we'll call a "client" and "server". A client wants access to the rest of the network through the server, and the server is willing to route the client's packets for a price.

-->request  This is the first message that the client will send to the server asking for a price for a certain network address
<--propose  The server responds with a proposed price and terms
-->accept   The client will either accept this price and terms after which the server will respond with begin, or
-->reject   The client may reject the request, offering a different price or terms. The server must respond with another propose
<--begin    The server responds, letting the client know that they may begin sending network packets

The state of each connection is tracked in the state struct. Here is a definition of its members:

  char interface_id[MAX_IDENTIFIER_LEN]; This is the string identifier of the link level interface the connection is on.
  int status; This is an enumerated integer that maintains the state of the connection. A description of each follows.
  char address[MAX_ADDRESS_LEN]; This is the string identifier of the network level address the connection will instruct packets to deliver to.
  int64_t price; This is the price per packet that is agreed upon.
  long int payment_advance; This is the number of packets the client must prepay for before they will receive service. If the client fails to maintain this payment buffer, the server will stop delivering packets.
  int64_t payment_sent; This is the record of how much the client has sent in payment.
  long int packets_delivered; This is a record of how many packets the server has delivered.
  time_t time_expiration;

Features to add in:
- Implement the link system using inet sockets instead of dummy messages to the terminal
- Implement the auto construction and destruction of an ipv4 network in the network_ipv4 class
- Create a new payment class to implement sending and receiving bitcoin
- Smarter client behavior for making payments just in time
- Smarter default client behavior for polling all interfaces and choosing the best deal
- GUI for config and other things

