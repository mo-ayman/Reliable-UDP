<h1 align="center">Reliable UDP File Transfer</h1>
<div align="center">
    <img src="./imgs/img-intro.jpg" alt="File Transfer" width="500" />
</div>


This project implements a basic UDP-based file transfer protocol with congestion control. The implementation consists of a server and a client, allowing the transfer of files between the two over UDP.

## Table of Contents

- [Overview](#overview)
- [UDP Server](#udp-server)
- [UDP Client](#udp-client)
- [Usage](#usage)
- [Performance Metrics](#performance-metrics)
- [License](#license)

## Overview

In this project, a file is split into fixed-length chunks, and the data from each chunk is added to a UDP datagram packet. The implementation includes a simple congestion control mechanism inspired by TCP.

## UDP Server

### Pseudocode

1. Listen for requests on the predefined port.
2. Get client IP address and port number.
3. If it's a new connection, fork a new child to handle it. Otherwise, route the received packet to the appropriate child via IPC.
   - Open the file to be sent.
   - Receive ack 0 from the client, which is equivalent to ack a handshake.
   - Transmit all the window determined by CWND.
   - Wait for the window to be completely acknowledged.
   - Go to step (c) if more bytes are remaining.

## UDP Client

### Pseudocode

1. Open socket.
2. Connect with the server with an IP address taken from arguments.
3. Send a packet that initializes the new connection.
4. Receive a packet from the server that has the file size.
5. Maintain a variable last ack with the first value = 1 to request using ack.
6. Receive packets sent by the server, save them, and maintain a sent array that tells us that this packet is sent.
7. Always ack using last ack.
8. Update last ack when the sequence number received is equal to the last ack.
9. Update until unsent packets.
10. Do until the last packet.

## Usage

1. Compile the server and client programs.
```
g++ main.cpp -o server
g++ client.cpp -o client
```
2. Start the server using
```
./server <loss_rate>
```
3. Start the client using
```
./client <file_name> <server_ip>
```

## Performance Metrics

- **File Size:** 5076501 bytes
- **Time Taken:** 155.854 seconds
- **Throughput:** 32572.157275399 bytes/second
              = 65.144314551 packets/second

## License

This project is licensed under the [MIT License](LICENSE).
