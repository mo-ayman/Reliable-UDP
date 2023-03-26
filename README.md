# Reliable-UDP
This repository contains a simple implementation of a reliable UDP protocol.

## Overview
A service that guarantees the arrival of datagrams in the correct order on top of the UDP/IP protocol, along with congestion control. It provides a way for applications to send data over the network with guaranteed delivery, without requiring the complexity of TCP.

This implementation of reliable UDP provides the following features:

- Packet retransmission: If a packet is lost or damaged in transit, it will be retransmitted until it is successfully received.
- Packet sequencing: Each packet is given a unique sequence number to ensure that they are received in the correct order.
- Acknowledgements: The receiver sends an acknowledgement message to the sender to indicate that a packet has been successfully received.

## Usage
To use the reliable UDP protocol in your application, you can follow these steps:

1. Clone the repository: git clone https://github.com/mo-ayman/Reliable-UDP.git
2. Execute the server (main.cpp) then execute the client (client.cpp)

## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/mo-ayman/Reliable-UDP/blob/main/LICENSE) file for details.

