#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <time.h>
#include <unordered_map>
#include <string.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
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

// hashmap to map host_port to ipcfd
std::unordered_map<std::string, int> ipcfd_map;

int handle_request(int sockfd, struct sockaddr_in cliaddr, socklen_t len, int ipcfd, float loss_rate, string file_name) {
	int cwnd = MSS;
	int dup_ack = 0;
	int state = SLOW_START;
	int can_transmit = 0;
	int conn_ack = 0;
	int transmit_round = 0;
	int i = 0;
	size_t last_ack = 0;
	size_t ssthresh = 64 * 1024;

	srand(time(NULL));

	// create new csv file
	FILE *csv_fp = fopen("out.csv", "w");
	fprintf(csv_fp, "round,cwnd,ssthresh\n");

	// send input.txt file to client

	FILE *fp = fopen(file_name.c_str(), "rb");
	if (fp == NULL) {
		printf("File not found");
		return 0;
	}

	// get file size
	fseek(fp, 0, SEEK_END);

	size_t file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// get number of packets
	size_t num_packets = file_size / 500;
	if (file_size % 500 != 0) {
		num_packets++;
	}
	
	ack ack_packet;
	ack_packet.cksum = 0;
	ack_packet.len = file_size;
	ack_packet.ackno = 0;
	cout << ack_packet.len << endl;
	// send ack packet
	sendto(sockfd, &ack_packet, sizeof(ack_packet),
		MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
			len);


	// create packets
	packet packets[num_packets];
	for (int i = 0; i < num_packets; i++) {
		size_t bytes_read = fread(packets[i].data, 1, 500, fp);
		packets[i].len = bytes_read;
		if (i == 0) {
			packets[i].seqno = 1;
		} else {
			packets[i].seqno = packets[i - 1].seqno + packets[i - 1].len;
		}
		// cout << "seqno: " << packets[i].seqno << endl;
	}
	
	while(true) {
		if (can_transmit == 1) {
			fprintf(csv_fp, "%d,%d,%d\n", ++transmit_round, cwnd, ssthresh);
			for (int j = 0; j < cwnd / MSS; j++) {
				if (i >= num_packets) {
					break;
				}
				if (rand() % 100 >= loss_rate * 100) {
					sendto(sockfd, &packets[i], sizeof(packet),
						MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
							len);
					printf("Packet %d sent.\n", i);
				} else {
					printf("Packet %d lost.\n", i);
				}
				i++;
			}
		}

		// do action on timeout
		fd_set readfds {};
		struct timeval tv {};
		int retval;
		FD_ZERO(&readfds);
		FD_SET(ipcfd, &readfds);
		tv.tv_sec = conn_ack == 0 ? 1 : 0.2;
		tv.tv_usec = 0;
		retval = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		if (retval == -1) {
			printf("Error in select()\n");
			break;
		} else if (retval == 0) {
			printf("Timeout occurred!\n");
			if (conn_ack == 0) {
				printf("Connection failed.\n");
				return -1;
			}
			ssthresh = cwnd / 2;
			cwnd = MSS;
			dup_ack = 0;
			state = SLOW_START;
			can_transmit = 0;
			int lost_packet_idx = (last_ack - 1) / MSS;
			if (rand() % 100 >= loss_rate * 100) {
				sendto(sockfd, &packets[lost_packet_idx], sizeof(packet),
					MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
						len);
				printf("Packet %d resent.\n", lost_packet_idx);
			} else {
				printf("Packet %d retransmission failed.\n", lost_packet_idx);
			}
			continue;
		}

		ack ack_packet;

		printf("Waiting for ack...\n");
		read(ipcfd, &ack_packet, sizeof(ack_packet));
		cout << "Ack " << ack_packet.ackno << " received." << endl;

		if(ack_packet.ackno > file_size) {
			printf("File transfer complete.\n");
			fclose(csv_fp);
			return 0;
		}

		// printf("Ack %d received.\n", ack_packet.ackno);

		conn_ack = 1;

		if (ack_packet.ackno == last_ack) {
			dup_ack++;
			can_transmit = 0;
		} else {
			dup_ack = 0;
			can_transmit = 1;
		}

		last_ack = ack_packet.ackno;

		if (dup_ack == 3) {
			state = FAST_RECOVERY;
			ssthresh = cwnd / 2;
			cwnd = ssthresh + 3 * MSS;
			int lost_packet_idx = (last_ack - 1) / MSS;
			if (rand() % 100 >= loss_rate * 100) {
				sendto(sockfd, &packets[lost_packet_idx], sizeof(packet),
					MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
						len);
				printf("Packet %d resent.\n", lost_packet_idx);
			} else {
				printf("Packet %d retransmission failed.\n", lost_packet_idx);
			}
			continue;
		}

		if (state == SLOW_START) {
			if (dup_ack == 0) {
				cwnd += MSS;
				if (cwnd >= ssthresh) {
					state = CONGESTION_AVOIDANCE;
				}
			}
		} else if (state == CONGESTION_AVOIDANCE) {
			if (dup_ack == 0) {
				cwnd += MSS * MSS / cwnd;
			}
		} else if (state == FAST_RECOVERY) {
			if (dup_ack == 0) {
				state = CONGESTION_AVOIDANCE;
				cwnd = ssthresh;
				can_transmit = 0;
			} else {
				cwnd += MSS;
				can_transmit = 1;
			}
		}

		if (ack_packet.ackno < i * MSS) {
			can_transmit = 0;
		} else {
			can_transmit = 1;
		}
	}
}
	
// Driver code
int main(int argc, char const* argv[]) {
	// convert loss rate to float
	float loss_rate = atof(argv[1]);

	int sockfd;
	char buffer[MSS];
	char *hello = "Hello from server";
	struct sockaddr_in servaddr, cliaddr;
		
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
		
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	
	std::stringstream ss;
		
	while (true) {
		struct sockaddr_in cliaddr;
		socklen_t len = sizeof(cliaddr); //len is value/result
		memset(&cliaddr, 0, sizeof(cliaddr));
		cout << "Waiting for client..." << endl;

		int n = recvfrom(sockfd, buffer, MSS,
					MSG_WAITALL, ( struct sockaddr *) &cliaddr,
					&len);

		cout << "Client connected." << endl;
		ss.str(std::string());
		ss << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port);
		if (ipcfd_map.find(ss.str()) == ipcfd_map.end()) {
			int ipcfd[2];
			if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipcfd) < 0) {
				perror("socketpair");
				exit(1);
			}
			ipcfd_map[ss.str()] = ipcfd[0];
			ss.str(std::string());
			packet p;
			memcpy(&p, buffer, sizeof(packet));
			ss << p.data;
			string file_name = ss.str();
			int pid = fork();
			if (pid == 0) {
				close(ipcfd[0]);
				// calculate the time taken by handle_request
				auto start = std::chrono::high_resolution_clock::now();
				handle_request(sockfd, cliaddr, len, ipcfd[1], loss_rate, file_name);
				auto finish = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = finish - start;
				cout << "Time taken by handle_request: " << elapsed.count() << " s" << endl;
				close(ipcfd[1]);
				exit(0);
			} else {
				close(ipcfd[1]);
			}
		} else {
			int ipcfd = ipcfd_map[ss.str()];
			write(ipcfd, buffer, n);
		}
	}
	return 0;
}
