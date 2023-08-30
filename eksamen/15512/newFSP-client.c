#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include<time.h>
#include "send_packet.h"
#include "rdp-protocol.h"

struct rdp_connection *server_connection;
char *filename;


//Funksjon for å generere et filnavn, og legge det i en global variabel
void generate_filename(){
  filename = malloc(17);//Filename is always 17 chars long, including 0-byte
  char *tmp = "kernel-file-";
  int rd;
  char n[5];
  rd = 1000+(rand()%9000);
  snprintf(n, 5, "%d", rd);
  n[4] = 0;
  memcpy(filename, tmp, 12);
  memcpy(&filename[12], n, 5);
}


// Det opprettes først en ny fil, deretter sendes det ack-pakker til serveren helt til det mottas
// en tom datapakke. Da sendes en disconnect-pakke til serveren og klienten avsluttes
void make_out_file(){
  int data_length;
  char buf[MAX_PACKET_LENGTH] = {0};
  generate_filename();
  while( access( filename, F_OK ) == 0 ) {
    generate_filename();
  }
  FILE *outFile = fopen(filename, "w");
  if(outFile == NULL){
    perror("fopen");
  }

  //her sendes pakken på nytt hvert 100ms helt til send_ack returnerer 1 (som betyr at neste datapakke er mottatt)
  while(1){
    int ret;
    while((ret = send_ack(server_connection, buf)) != 1){
      if(ret == 2){
        send_special_packet(server_connection->socket, &server_connection->target_addr, 0x02, server_connection->client_id, server_connection->server_id);
        printf("Connection closed, filename: %s\n", filename);
        free(server_connection);
        free(filename);
        fclose(outFile);
        exit(EXIT_SUCCESS);
      }
    }

    //data fra pakken kopieres inn i et buffer og skrives til fil
    memcpy(&data_length, &buf[12], sizeof(int));
    data_length = data_length - 16;

    fwrite(&buf[16], data_length, 1, outFile);

    if(ferror(outFile)){
      fprintf(stderr, "Error occured in writing out-file\n");
      free(filename);
      free(server_connection);
      fclose(outFile);
      exit(EXIT_FAILURE);
    }
    if(buf[0] == 0x04){
      server_connection->last_packet_recieved++;
    }
  }
  fclose(outFile);
}


// Main-funksjon
int main(int argc, char const *argv[]) {

  if(argc < 4){
    printf("Usage: %s <target ip> <target port> <loss_probability>\n", argv[0]);
    return EXIT_SUCCESS;
  }
  srand(time(0));
  set_loss_probability(atof(argv[3]));

  int counter = 0;
  int connected = 0;

  struct rdp_connection *connection;

  //klienten forsøker å koble til serveren i maksimalt 1 sekund (tilsvarer at forbindelsesforespørsel sendes maks 10 ganger)
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

  connection->last_packet_recieved = 0;
  server_connection = connection;

  make_out_file();

  return EXIT_SUCCESS;
}
