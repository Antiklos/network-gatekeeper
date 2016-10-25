#Network Gatekeeper

##Why?

Mesh networking offers a distributed alternative to the currently centralized method of connecting to the internet, but isn't viable as such an alternative today because of the lack of incentive for nodes to provide service. Owners of centralized backbones are able to offer services in exchange for a subscription payment, but nodes in a mesh network aren't able to easily exchange money for their services. This makes it difficult for meshet projects to build an infrastructure to provide service to entire cities, and practically impossible to provide service between cities, because most service providing nodes do so solely at the expense of their owner. If these nodes were able to exchange money for their services, it would have the potential to drive an economy for meshnet connections to the internet, up to and including parts of network infrastructure that require large economies of scale. Also, if these nodes were able to charge a certain price for a certain amount of data to a certain destination, rather than charging for a subscription, it would allow people who consume very little bandwidth to participate where they otherwise would not. It would also allow owners of network hardware to recoup the costs of their participation, which would have otherwise prohibited them from joining the network.

Network Gatekeeper gives nodes the ability to pay their neighbors in exchange for forwarding their packets. The user should be able to configure settings and then start the daemon, which allows the user to access the internet normally, while the daemon negotiates and pays for service automatically. Settings include acceptable price ranges, a grace period for payment, when to renew a contract, and more.

##How does it work?

Each message is an interaction between what we'll call a "client" and "server". A client wants access to the rest of the network through the server, and the server is willing to route the client's packets for a price.

* -->request  This is the first message that the client will send to the server asking for a price for a certain network address
* <--propose  The server responds with a proposed price and terms
* -->accept   The client will either accept this price and terms after which they will send payment, or
* -->reject   The client may reject the request, offering a different price or terms. The server must respond with another propose

After the contract is accepted, a grace period will be implemented allowing the client to send data to the destination immediately, while their payment is processed.

The parameters of the contract are:

1. Price
2. Network level source address
3. Network level destination address
4. Expiration time

Explanation of parameters in the config file:

* LINK_INTERFACE: The identifier for the module that will be used to encode and send messages to an ngp daemon on another host
* NETWORK_INTERFACE: The identifier for the module that will be used to monitor and gate network-layer traffic
* PAYMENT_INTERFACE: The identifier for the module that will be used to send and check for received payment
* NGP_INTERFACE: The network-layer address for the server
* DEFAULT_PRICE: The price per kilobyte that the client will be charged
* GRACE_PERIOD_PRICE: The worth of the data the client will be given in the grace period (for example, if the accepted price is 5, and GRACE_PERIOD_PRICE is 10, the client will be given 2kb of data)
* CONTRACT_DATA: The data limit of any contract in kilobytes
* CONTRACT_TIME: The duration of any contract
* DATA_RENEWAL: When there is less than this amount of data in bytes left on a contract, the client will initiate another contract
* TIME_RENEWAL: When there is less than this amount of time left of a contract, the client will initiate another contract

##System Requirements:

Dependencies: gcc; build-essential; jq
Tested only on Ubuntu 14.04 and Debian Jessie

##How to:

To get started right away with one machine, here's a guide on how to test it out:

1. Install dependencies: sudo apt-get install gcc build-essential jq
2. Compile the code: gcc main.c -o ngp
3. Start the server: sudo ./ngp start -v
4. Now test that the server is receiving messages on the local unix socket: sudo ./ngp server test

To test out communication between two hosts, make sure that both are running ngp successfully as above. Then:

1. Make sure that when ngp is not running on either host, the client is able to route its packets through the server.
2. Edit net.conf on the client and make sure NGP_INTERFACE is the ip address of your server.
3. Until it's made configurable, ngp assumes that the client and server communicate over the interface eth0. If that's not right for you, change it on line 30 of network_ipv4.c, and recompile.
4. It's also assumed that the interface for hosts upstream of the server will be wlan0. If that's not right for you, change it at line 17 of network_ipv4.c and recompile.
5. Run on server: sudo ./ngp start (add the flag -v if you want to see log messages printed to the command line)
6. Run on client: sudo ./ngp start (add the flag -v if you want to see log messages printed to the command line)
7. On the client, ping something or do something that will try to route data across your server. Notice that if you pinged something requiring a DNS request, it will take a few seconds to return, because the first message was actually blocked by the server before the grace period could be implemented.
8. When you're finished, run on client: sudo ./ngp stop
9. Run on server: sudo ./ngp stop


