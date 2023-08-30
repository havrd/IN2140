#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include<time.h>
#include "send_packet.h"
#include "rdp-protocol.h"

int number_of_times_to_send_file, num_packets, server_port, number_of_current_connections, number_of_total_connections;
unsigned int server_id;
struct rdp_connection **connections;
struct packet **data_packets;

struct rdp_connection *find_connection(unsigned int id){
  for(int i = 0; i<number_of_total_connections; i++){
    if(id == connections[i]->client_id){
      return connections[i];
    }
  }
  return NULL;
}


void read_file_to_send(const char *filename){
  FILE *datFile = fopen(filename, "r");
  if(datFile == NULL){
    perror("fopen, read_file_to_send");
    fprintf(stderr, "No file with filname '%s' found. Shutting down server.\n", filename);
    exit(EXIT_FAILURE);
  }

  int i, size, rest;
  fseek(datFile, 0L, SEEK_END);
  size = ftell(datFile);
  rewind(datFile);

  num_packets = size/1000;
  rest = size%1000;
  if(rest != 0){num_packets++;}

  if(ferror(datFile)){
    fprintf(stderr, "Error occured in reading file\n");
    fclose(datFile);
    exit(EXIT_FAILURE);
  }

  data_packets = malloc(sizeof(struct packet *) * num_packets);
  if(data_packets == NULL){ fprintf(stderr, "%malloc error\n"); fclose(datFile);}

  for(i = 0; i<num_packets; i++){
    struct packet *tmp = malloc(sizeof(struct packet));
    data_packets[i] = tmp;
    if(tmp == NULL){ fprintf(stderr, "%malloc error\n"); fclose(datFile);}
    memset(tmp, 0, sizeof(struct packet));

    if(i == (num_packets-1) ){
      tmp->payload = malloc(rest);
      if(tmp->payload == NULL){ fprintf(stderr, "%malloc error\n"); fclose(datFile);}
      tmp->metadata = (16 + rest);
      fread((tmp->payload), rest, 1, datFile);
    }
    else{
      tmp->payload = malloc(1000);
      if(tmp->payload == NULL){ fprintf(stderr, "%malloc error\n"); fclose(datFile);}
      tmp->metadata = (16 + 1000);
      fread((tmp->payload), 1000, 1, datFile);
    }
  }

  if(ferror(datFile)){
    fprintf(stderr, "Error occured in reading .dat file\n");
    fclose(datFile);
    exit(EXIT_FAILURE);
  }
  fclose(datFile);
}


void free_packets(){
  printf("Free'ing data packets...\n");
  for(int i = 0; i<num_packets; i++){
    free(data_packets[i]->payload);
    free(data_packets[i]);
  }
  free(data_packets);
}


void free_rdp_connections(){
  printf("Free'ing rdp connections...\n");
  for(int i = 0; i<number_of_times_to_send_file; i++){
    free(connections[i]);
  }
  free(connections);
}


void shutdown_server(){
  printf("Server shutting down...\n");
  free_packets();
  free_rdp_connections();
  exit(EXIT_SUCCESS);
}


void check_active_connections(){
  if(number_of_current_connections <= 0 && number_of_times_to_send_file == number_of_total_connections){
    shutdown_server();
  }
}


void respond_to_connection_req(struct rdp_connection *connection){
  int reason_for_rejection;
  if(number_of_current_connections >= number_of_times_to_send_file){
    reason_for_rejection = 1;
    reject_connection(connection, reason_for_rejection);
    return;
  }

  for(int i = 0; i<number_of_total_connections; i++){
    if(connection->client_id == connections[i]->client_id && connections[i]->active == 1){
      reason_for_rejection = 2;
      reject_connection(connection, reason_for_rejection);
      return;
    }
  }

  accept_connection(connection);
  connection->active = 1;

  printf("CONNECTED client-id: %d server-id: %d\n", connection->client_id, connection->server_id);
  connections[number_of_total_connections] = connection;
  number_of_current_connections++;
  number_of_total_connections++;
}


void shuffle(void *array, size_t n, size_t size) {
    char tmp[size];
    char *arr = array;
    size_t stride = size * sizeof(char);

    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; ++i) {
            size_t rnd = (size_t) rand();
            size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

            memcpy(tmp, arr + j * stride, size);
            memcpy(arr + j * stride, arr + i * stride, size);
            memcpy(arr + i * stride, tmp, size);
        }
    }
}


void send_packets_to_ready_clients(){
  int i, ret;
  //shuffle connections array for even distribution of sent packets
  shuffle(connections, number_of_total_connections, sizeof(struct rdp_connection *));
  for(i = 0; i<number_of_total_connections; i++){
    if(connections[i]->ready_to_recieve == 1 && connections[i]->active == 1){
      unsigned char pktnum = connections[i]->last_packet_recieved+1;

      if(pktnum >= num_packets){
        send_special_packet(connections[i]->socket, &connections[i]->target_addr, 0x04, server_id, connections[i]->client_id);
        fprintf(stderr, "EXITING\n");
        return;
      }

      while( (ret = rdp_write_no_ack(pktnum, data_packets[pktnum], connections[i])) != 1 ){
         if( ret == 2 ){
          printf("DISCONNECTED server id: %d client id: %d\n", server_id, ntohl(connections[i]->client_id));
          connections[i]->active = 0;
          number_of_current_connections--;
        }
      }

    }
  }
}


void server_loop(fd_set fds, int server_fd){

  int rc, sel;
  struct rdp_connection *new_connection;
  unsigned char type;
  unsigned char buf[sizeof(struct packet)];
  socklen_t addr_len;
  addr_len = sizeof(struct sockaddr_in);


  while(1){

    FD_ZERO(&fds);
    FD_SET(server_fd, &fds);

    //Select blokkerer i 10ms
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    struct sockaddr_in *target_addr = malloc(sizeof(struct sockaddr_in));

    sel = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
    if(sel == -1){check_error(rc, "select, server_loop");}


    if(FD_ISSET(server_fd, &fds)){
      fprintf(stderr, "HELLO\n");
      rc = recv(server_fd, &type, 1, MSG_PEEK);
      check_error(rc, "recvfrom");
      printf("Type inkommende melding: %x\n", type);

      if(type == 0x01){
        new_connection = rdp_accept(server_fd, server_id);
        if(new_connection != NULL){
          printf("Trying to accept connection\n");
          respond_to_connection_req(new_connection);
          free(target_addr);
        }
      }

      if(type == 0x08){
        printf("Prøver å sende datapakke...\n");

        rc = recvfrom(server_fd, &buf, sizeof(struct packet), MSG_PEEK, (struct sockaddr *)target_addr, &addr_len);
        unsigned char pktnum;
        memcpy(&pktnum, &buf[2], 1);
        if(buf[2] >= num_packets){
          unsigned int to_id;
          memcpy(&to_id, &buf[4], 4);
          send_special_packet(server_fd, target_addr, 0x04, server_id, to_id);
          fprintf(stderr, "EXITING\n");
          rc = recv(server_fd, &buf, 3, 0);
          free(target_addr);
        }
        else {
          int ret;
          unsigned int client_id;
          memcpy(&client_id, &buf[4], 4);
          struct rdp_connection *con = find_connection(ntohl(client_id));
          con->active = 1;
          while( (ret = rdp_write(pktnum, data_packets[pktnum], con)) != 1 ){
            if( ret == 0 ){
              continue;
            }
            else if( ret == 2 ){
              memcpy(&client_id, &buf[4], 4);
              printf("DISCONNECTED server id: %d client id: %d\n", server_id, ntohl(client_id));
              struct rdp_connection *con = find_connection(ntohl(client_id));
              con->active = 0;
              number_of_current_connections--;
            }
          }
          free(target_addr);
          }
      }

      if(type == 0x02){
        unsigned int client_id;
        rc = recvfrom(server_fd, &buf, sizeof(struct packet), 0, (struct sockaddr *)target_addr, &addr_len);
        memcpy(&client_id, &buf[4], 4);
        printf("DISCONNECTED server id: %d client id: %d\n", server_id, ntohl(client_id));
        struct rdp_connection *con = find_connection(ntohl(client_id));
        con->active = 0;
        number_of_current_connections--;
        free(target_addr);
        check_active_connections();
      }
    }


    if(sel == 0){
      send_packets_to_ready_clients();
      free(target_addr);
      continue;
    }

  }
}


int main(int argc, char const *argv[]) {

  if(argc < 4){
    printf("Usage: %s <my port> <filename> <number of times to send file> <loss probability>\n", argv[0]);
    return EXIT_SUCCESS;
  }

  int server_fd, rc;
  fd_set fds;
  struct sockaddr_in server_addr;
  server_port = atoi(argv[1]);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;


  set_loss_probability(atof(argv[4]));
  number_of_times_to_send_file = atoi(argv[3]);
  connections = malloc(sizeof(struct rdp_connection *) * number_of_times_to_send_file);
  number_of_current_connections = 0;
  number_of_total_connections = 0;
  read_file_to_send(argv[2]);
  srand(time(0));
  server_id = rand();

  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  check_error(server_fd, "socket");

  setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  rc = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
  check_error(rc, "bind 1");

  FD_ZERO(&fds);
  FD_SET(server_fd, &fds);

  server_loop(fds, server_fd);

  return EXIT_SUCCESS;
}
