#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include "send_packet.h"
#include "rdp-protocol.h"

void check_error(int res, char *msg){
  if(res == -1){
    perror(msg);
    exit(EXIT_FAILURE);
  }
}


int check_address(struct sockaddr_in *target_addr, struct sockaddr_in *check_addr){
  if(target_addr->sin_port == check_addr->sin_port && target_addr->sin_addr.s_addr == check_addr->sin_addr.s_addr) return 1;
  else return 0;
}


// sender datapakke, kan gjøres enklere
int rdp_write(unsigned char pkt_num_to_send, struct packet *to_send, struct rdp_connection *con){
  int rc;
  struct sockaddr_in *check_addr = malloc(sizeof(struct sockaddr_in));
  char recv_buf[sizeof(struct packet)];

  rc = recv(con->socket, &recv_buf, sizeof(struct packet), 0);
  if(rc > 0){
    if(recv_buf[0] == 0x02){
      free(check_addr);
      return 2;
    }

    if(recv_buf[0] == 0x01){
      free(check_addr);
      return 0;
    }

    unsigned int send_to_id = htonl(con->client_id);
    unsigned int from_id = htonl(con->server_id);

    char buf[to_send->metadata];
    memset(buf, 0, to_send->metadata);
    buf[0] = 0x04;         //data_packet
    buf[1] = pkt_num_to_send;
    memcpy(&buf[4], &from_id, 4);
    memcpy(&buf[8], &send_to_id, 4);
    memcpy(&buf[12], &to_send->metadata, 4);
    memcpy(&buf[16], to_send->payload, (to_send->metadata - 16));

    printf("Sending datapacket %d to client: %d...\n", pkt_num_to_send, con->client_id);

    rc = send_packet(con->socket, buf, to_send->metadata, 0, (struct sockaddr *)&con->target_addr, sizeof(struct sockaddr_in));

    socklen_t addr_len = sizeof(struct sockaddr_in);
    rc = recvfrom(con->socket, &recv_buf, sizeof(struct packet), 0, (struct sockaddr *)check_addr, &addr_len);
    if(rc == -1 && errno!=EAGAIN){check_error(rc, "recv, make_out_file");}
    if(rc == -1){
      free(check_addr);
      return 0;
    }

    if(check_address(&con->target_addr, check_addr)){
      if(recv_buf[2] == (pkt_num_to_send+1) ){
        free(check_addr);
        check_error(rc, "send_packet");
        con->last_packet_recieved = pkt_num_to_send;
        con->ready_to_recieve = 1;
        return 1;
      }
    }
  }
  free(check_addr);
  return 0;
}






int rdp_write_no_ack(unsigned char pkt_num_to_send, struct packet *to_send, struct rdp_connection *con){
  int rc;
  struct sockaddr_in *check_addr = malloc(sizeof(struct sockaddr_in));
  char recv_buf[sizeof(struct packet)];

    unsigned int send_to_id = htonl(con->client_id);
    unsigned int from_id = htonl(con->server_id);

    char buf[to_send->metadata];
    memset(buf, 0, to_send->metadata);
    buf[0] = 0x04;         //data_packet
    buf[1] = pkt_num_to_send;
    memcpy(&buf[4], &from_id, 4);
    memcpy(&buf[8], &send_to_id, 4);
    memcpy(&buf[12], &to_send->metadata, 4);
    memcpy(&buf[16], to_send->payload, (to_send->metadata - 16));

    printf("Sending datapacket %d to client: %d...\n", pkt_num_to_send, con->client_id);

    rc = send_packet(con->socket, buf, to_send->metadata, 0, (struct sockaddr *)&con->target_addr, sizeof(struct sockaddr_in));

    socklen_t addr_len = sizeof(struct sockaddr_in);
    rc = recvfrom(con->socket, &recv_buf, sizeof(struct packet), 0, (struct sockaddr *)check_addr, &addr_len);
    if(rc == -1 && errno!=EAGAIN){check_error(rc, "recv, make_out_file");}
    if(rc == -1){
      free(check_addr);
      return 0;
    }

    if(recv_buf[0] == 0x02){
      free(check_addr);
      return 2;
    }
    if(recv_buf[0] == 0x01){
      free(check_addr);
      return 0;
    }

    if(check_address(&con->target_addr, check_addr)){
      if(recv_buf[2] == (pkt_num_to_send+1) ){
        free(check_addr);
        check_error(rc, "send_packet");
        con->last_packet_recieved = pkt_num_to_send;
        con->ready_to_recieve = 1;
        return 1;
      }
    }
  free(check_addr);
  return 0;
}







void reject_connection(struct rdp_connection *connection, int metadata){
  //Add reason for rejection
  char buf[24] = {0};
  int rc;
  unsigned int client_id = htonl(connection->client_id);
  unsigned int from = ntohl(connection->server_id);

  buf[0] = 0x20;
  memcpy(&buf[4], &from, 4);
  memcpy(&buf[8], &client_id, 4);
  memcpy(&buf[12], &metadata, 4);


  rc = send_packet(connection->socket, buf, 24, 0, (struct sockaddr *)&connection->target_addr, sizeof(struct sockaddr_in));
  check_error(rc, "send_packet, send_special_packet");
  printf("NOT CONNECTED client_id: %d server id: %d\n", connection->client_id, connection->server_id);
}


struct rdp_connection *rdp_accept(int data_fd, unsigned int server_id){

  int rc;
  unsigned int client_id;
  struct sockaddr_in target_addr;
  socklen_t addr_len;
  char recv_buf[sizeof(struct packet)];

  addr_len = sizeof(struct sockaddr_in);

  printf("Waiting for connection request...\n");
  rc = recvfrom(data_fd, &recv_buf, sizeof(struct packet), 0, (struct sockaddr *)&target_addr, &addr_len);
  check_error(rc, "recvfrom");
  if(rc > 0){
    memcpy(&client_id, &recv_buf[4], 4);
    struct rdp_connection *ret = malloc(sizeof(struct rdp_connection));
    ret->target_addr = target_addr;
    ret->server_id = server_id;
    ret->socket = data_fd;
    ret->client_id = ntohl(client_id);
    ret->ready_to_recieve = 0;
    ret->last_packet_recieved = 0;
    ret->active = 0;

    return ret;
  }
  return NULL;
}


int send_special_packet(int data_fd, struct sockaddr_in *target_addr, unsigned char flag, unsigned int from_id, unsigned int to_id){
  char buf[24] = {0};
  int rc;
  unsigned int fid = ntohl(from_id);
  unsigned int tid = ntohl(to_id);

  buf[0] = flag;
  memcpy(&buf[4], &fid, 4);
  memcpy(&buf[8], &tid, 4);


  rc = send_packet(data_fd, buf, 24, 0, (struct sockaddr *)target_addr, sizeof(struct sockaddr_in));
  check_error(rc, "send_packet, send_special_packet");
  return EXIT_SUCCESS;
}


void send_ack(int msg_fd, struct sockaddr_in *target_addr, int last_datapacket_recieved, unsigned int from_id, unsigned int to_id){
  char buf[24] = {0};
  int rc;
  unsigned int fid = ntohl(from_id);
  unsigned int tid = ntohl(to_id);

  buf[0] = 0x08;
  buf[2] = last_datapacket_recieved;
  memcpy(&buf[4], &fid, 4);
  memcpy(&buf[8], &tid, 4);


  rc = send_packet(msg_fd, buf, 24, 0, (struct sockaddr *)target_addr, sizeof(struct sockaddr_in));
  check_error(rc, "send_packet, send_ack");
  printf("Ack number '%d' sent\n", last_datapacket_recieved);
}


struct rdp_connection *connect_to_server(const char *target_ip, const char *target_port){
  int msg_fd, rc;
  struct sockaddr_in target_addr, my_addr;
  struct packet connect_req;
  struct rdp_connection *con = malloc(sizeof(struct rdp_connection));

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  msg_fd = socket(AF_INET, SOCK_DGRAM, 0);
  setsockopt(msg_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
  check_error(msg_fd, "socket");
  unsigned short my_port = generate_network_id();

  con->socket = msg_fd;


  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(my_port);
  my_addr.sin_addr.s_addr = INADDR_ANY;

  rc = bind(msg_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
  while(rc == -1 && errno == EADDRINUSE){
    my_port = generate_network_id();
    my_addr.sin_port = htons(my_port);

    rc = bind(msg_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
  }
  check_error(rc, "bind");

  unsigned int client_id = rand();
  con->client_id = client_id;

  connect_req.flags = 0x01;
  connect_req.pktseq = 0;
  connect_req.senderid = htonl(client_id);
  connect_req.recvid = 0;

  target_addr.sin_family = AF_INET;
  target_addr.sin_port = htons(atoi(target_port));
  rc = inet_pton(AF_INET, target_ip, &target_addr.sin_addr.s_addr);

  con->target_addr = target_addr;

  check_error(rc, "inet_pton");
  if(rc == 0){
    fprintf(stderr, "Ip address not valid: %s\n ", target_ip);
    free(con);
    close(msg_fd);
    exit(EXIT_FAILURE);
  }

  char send_buf[24] = {0};
  char recv_buf[sizeof(struct packet)];
  send_buf[0] = connect_req.flags;
  memcpy(&send_buf[4], &connect_req.senderid, 4);


  //Her skal du flytte løkken ut av funksjonen og returnere et rdp_connection struct istedet
  rc = send_packet(msg_fd, send_buf, 24, 0, (struct sockaddr *)&target_addr, sizeof(struct sockaddr_in));
  check_error(rc, "send_packet");

  rc = recv(msg_fd, &recv_buf, sizeof(struct packet), 0);
  if(rc == -1 && errno == EAGAIN){
    return NULL;
  }
  check_error(rc, "recv, connect_to_server");

  unsigned char connection_response = recv_buf[0];
  memcpy(&con->server_id, &recv_buf[4], 4);
  int server_id = ntohl(con->server_id);
  if(connection_response == 0x10){
    //connection accepted
    printf("CONNECTED client-id: %d server-id: %d\n", client_id, server_id);
    return con;
  }
  else if(connection_response == 0x20){
    //connection rejected
    int reason_for_rejection;
    memcpy(&reason_for_rejection, &recv_buf[12], 4);

    if(reason_for_rejection == 1){
      printf("NOT CONNECTED client id: %d server id: %d\nNo more files to deliver.\n", client_id, server_id);
      exit(EXIT_FAILURE);
    }
    else if(reason_for_rejection == 2){
      printf("NOT CONNECTED client id: %d server id: %d\nClient with id: %d is already connected to server.\n", client_id, server_id, client_id);
      exit(EXIT_FAILURE);
    }
    else exit(EXIT_FAILURE);
  }

  return NULL;
}


int generate_network_id(){return (rand() % (65535 - 49152 + 1)) + 49152;} // available ports to use


void accept_connection(struct rdp_connection *connection){
  send_special_packet(connection->socket, &connection->target_addr, 0x10, connection->server_id, connection->client_id);
}








//end
