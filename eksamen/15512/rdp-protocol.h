#ifndef RDP_PROTOCOL_H
#define RDP_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define MAX_PACKET_LENGTH 1016

struct packet{
  unsigned char flags;
  unsigned char pktseq;
  unsigned char ackseq;
  unsigned char unassigned;
  int senderid;
  int recvid;
  int metadata;
  char *payload;
};

struct rdp_connection{
  struct sockaddr_in target_addr;
  int socket;
  unsigned int server_id;
  unsigned int client_id;
  unsigned char active;
  unsigned char last_packet_recieved;
  unsigned char ready_to_recieve;
};

int check_address(struct sockaddr_in *target_addr, struct sockaddr_in *check_addr);

int rdp_write(unsigned char pkt_num_to_send, struct packet *to_send, struct rdp_connection *con);

int rdp_write_no_ack(unsigned char pkt_num_to_send, struct packet *to_send, struct rdp_connection *con);

void reject_connection(struct rdp_connection *connection, int metadata);

void check_error(int res, char *msg);

struct rdp_connection *rdp_accept(int data_fd, unsigned int server_port);

int send_special_packet(int data_fd, struct sockaddr_in *target_addr, unsigned char flag, unsigned int from_id, unsigned int to_id);

int send_ack(struct rdp_connection *connection, char *buffer);

struct rdp_connection *connect_to_server(const char *target_ip, const char *target_port);

int generate_network_id();

void accept_connection(struct rdp_connection *connection);

#endif /* RDP_PROTOCOL_H */
