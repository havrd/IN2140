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

#define MAX_PACKET_LENGTH 1016


unsigned int client_id, server_id;
unsigned char last_datapacket_recieved; //recieved_first_packet;
struct rdp_connection *server_connection;
char filename[17]; //Filename is always 17 chars long, including 0-byte


int check_if_datapacket(unsigned char flags){
  if(flags == 0x04) return 1;
  else return 0;
}


void generate_filename(){
  char *tmp = "kernel-file-";
  int rd;
  char n[5];
  rd = 1000+(rand()%9000);
  snprintf(n, 5, "%d", rd);
  n[4] = 0;
  memcpy(filename, tmp, 12);
  memcpy(&filename[12], n, 5);
}


void make_out_file(int msg_fd, struct sockaddr_in *target_addr){
  int rc, data_length;
  char buf[MAX_PACKET_LENGTH];
  generate_filename();
  while( access( filename, F_OK ) == 0 ) {
    generate_filename();
  }
  FILE *outFile = fopen(filename, "w");
  if(outFile == NULL){
    perror("fopen");
  }
  while(1){
    send_ack(msg_fd, target_addr, last_datapacket_recieved, client_id, server_id);

    rc = recv(msg_fd, &buf, MAX_PACKET_LENGTH, 0);
    if(rc == -1 && errno!=EAGAIN){check_error(rc, "recv, make_out_file");}
    if(rc == -1 || rc == 0 || buf[1] < (last_datapacket_recieved) ){
      memcpy(&data_length, &buf[12], sizeof(int));
      data_length = data_length - 16;

      //File transmission over, ready to close connection
      if(data_length == -16 && buf[0] == 0x04){
        send_special_packet(msg_fd, target_addr, 0x02, client_id, server_id);
        printf("Connection closed, filename: %s\n", filename);
        fclose(outFile);
        return;
      }
      continue;
    }

    //denne kan potensielt fjernes
    //if(buf[1] == 1 && recieved_first_packet == 0){
      //continue;
    //}

    unsigned int target_id = 0;
    memcpy(&target_id, &buf[8], 4);
    if(client_id != ntohl(target_id)){
      printf("client id: %d - target id: %d\n",client_id, ntohl(target_id) );
      continue;
    }

    check_error(rc, "recv");
    memcpy(&data_length, &buf[12], sizeof(int));
    data_length = data_length - 16;

    fwrite(&buf[16], data_length, 1, outFile);

    if(ferror(outFile)){
      fprintf(stderr, "Error occured in writing out-file\n");
      fclose(outFile);
      exit(EXIT_FAILURE);
    }
    if(buf[0] == 0x04){
      last_datapacket_recieved++;
      fprintf(stderr, "Recieved packet number: %d\n", buf[1]);
      //recieved_first_packet = 1;
    }
  }
  fclose(outFile);
}


int main(int argc, char const *argv[]) {

  if(argc < 4){
    printf("Usage: %s <target ip> <target port> <loss_probability>\n", argv[0]);
    return EXIT_SUCCESS;
  }
  last_datapacket_recieved = 0;
  //recieved_first_packet = 0;
  srand(time(0));
  set_loss_probability(atof(argv[3]));

  int counter = 0;
  int connected = 0;

  struct rdp_connection *connection;

  while(!connected && counter < 11){
    connection = connect_to_server(argv[1], argv[2]);
    if(connection == NULL){
      if(counter == 10){
        printf("Connection request timed out. Try again later.\n");
        exit(EXIT_FAILURE);
      }
      counter++;
      continue;
    }
    connected = 1;
  }

  server_id = connection->server_id;
  client_id = connection->client_id;

  server_connection = connection;

  make_out_file(connection->socket, &connection->target_addr);

  return EXIT_SUCCESS;
}
