// implement the client side of the UDP protocol that will recieve file from server side and implement the congestion control algorithm over UDP protocol.

#include <bits/stdc++.h>
#include "header.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#define PORT 8080
#define MSS 500
#define SLOW_START 0
#define CONGESTION_AVOIDANCE 1
#define FAST_RECOVERY 2

using namespace std;

typedef struct packet {
	uint16_t cksum;
	uint32_t len;
	uint32_t seqno;
	char data[500];
} packet;

typedef struct ack_packet {
	uint16_t cksum;
	uint32_t len;
	uint32_t ackno;
} ack;


size_t get_file_size(string file_name) {
    struct stat stat_buf;
    int rc = stat(file_name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


int main(int argc, char const* argv[])
{

    char const * file_name = argv[1];
    char const * server_ip = argv[2];
    int port_number = PORT;
    if(argc == 4) {
        port_number = stoi(argv[3]);
    }

    
    FILE *fp = fopen(file_name, "wb");

	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_number);

	// Convert IPv4 and IPv6 addresses from text to binary
	if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		return -1;
	}
    cout << "Connecting to server : " << server_ip << " port : " << port_number << endl;
    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // sent hello packet
    packet hello;
    hello.seqno = 0;
    hello.len = 0;
    hello.cksum = 0;
    strcpy(hello.data, file_name);
    send(sock, &hello, sizeof(hello), 0);
    
    // receive dataack that contains file size
    ack file_size_ack;
    recv(sock, &file_size_ack, sizeof(file_size_ack), 0);

    size_t file_size = file_size_ack.len;
    size_t total_bytes = 0;
    size_t last_ack = 1;
    // initilize array of packets
    packet packets[file_size_ack.len / MSS + 1];

    bool sent[file_size_ack.len / MSS + 1];
    memset(sent, false, sizeof(sent));

    while(total_bytes < file_size) {
        cout << "total bytes : " << total_bytes << endl;
        cout << "file size : " << file_size << endl;
        // map last ack to index
        int last = (last_ack-1) / MSS;
        // if last ack is not received
        if(!sent[last]) {
            // send ack for last ack
            ack ack_packet;
            ack_packet.cksum = 0;
            ack_packet.len = 0;
            ack_packet.ackno = last_ack;
            send(sock, &ack_packet, sizeof(ack_packet), 0);
            cout << "sent ack : " << ack_packet.ackno << endl;
        }

        // receive packet
        packet p;
        recv(sock, &p, sizeof(p), 0);
        cout << "received packet : " << p.seqno << endl;
        
        if(p.seqno == last_ack) {
            sent[last] = true;
            packets[last] = p;

            while(sent[(last_ack-1) / MSS]) {
                last_ack += MSS;
                total_bytes += MSS;
            }

        } else {
            sent[(p.seqno-1) / MSS] = true;
            packets[(p.seqno-1) / MSS] = p;
            continue;
        }


    }

    // ack for end of file
    ack ack_packet;
    ack_packet.cksum = 0;
    ack_packet.len = 0;
    ack_packet.ackno = last_ack;
    send(sock, &ack_packet, sizeof(ack_packet), 0);

    // save file
    for(int i = 0; i < file_size_ack.len / MSS + 1; i++) {
        fwrite(packets[i].data, sizeof(char), packets[i].len, fp);
    }


    fclose(fp);
    close(sock);

	return 0;
}
