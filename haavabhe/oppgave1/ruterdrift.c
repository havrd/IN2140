#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCREASE_CHANGE 16
#define MAX_LINE_LENGTH 500

struct router **router_array;
int num_routers;

struct router{
  int ID;
  unsigned char flags;
  unsigned char length_of_modelstring;
  char model[250];
  struct router *neighbours[10];
  int number_of_neighbours;
  int visited;
};

//Tar inn en char og returnerer hvordan den ser ut skrevet i bits
char *char_to_bits(char c){
  //Allokeres 9 bytes. Plass til 8 bits og en null-bit.
  char *ret = malloc(9);
  int i;
  for (i = 0; i < 8; i++) {
      ret[i] = (!!((c << i) & 0x80)) + 48; // Legger til 48 for å få bokstaven 1 og 0 istedet for tallverdien
  }
  ret[i] = 0;
  return ret;
}

//Printer ut relevant info om en router
void print_router(int rid){
  struct router *tmp = NULL;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == rid){ tmp = router_array[i]; }
  }
  if(tmp == NULL){printf("Invalid ID in function print_router.\n"); return;}
  printf("%d\n", tmp->flags);
  char *flags = char_to_bits(tmp->flags);
  printf("ID: %d\nFlags: %s\nModel name: %s\nNeighbours: ", tmp->ID, flags, tmp->model);
  for(int i = 0; i < tmp->number_of_neighbours; i++){
    printf("%d ", tmp->neighbours[i]->ID);
  }
  printf("\n\n");
  //Frigjør det som ble allokert i char_to_bits funksjonen
  free(flags);
}

//Printer ut info om alle routerne
void print_all_routers(){
  int id;
  for(int i = 0; i<num_routers; i++){
    id = router_array[i]->ID;
    print_router(id);
  }
}

//Frigjør alle router-structs
void free_routers(){
  //Dette minne ble allokert i funksjonen read_dat_file
  for(int i = 0; i<num_routers; i++){
    free(router_array[i]);
  }
  free(router_array);
}

//Endrer modell-navnet på en router, tar inn en id og en char *
void set_model_name(int id, char *name){
  struct router *tmp = NULL;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ tmp = router_array[i]; }
  }
  if(tmp == NULL){printf("Invalid ID in function set_model_name, no changes were made.\n"); return;}
  strncpy(tmp->model, name, 249);
  tmp->model[249] = 0;
}

//Setter aktiv-bit'et i en router, tar inn en id og en verdi
void set_active(int id, int value){
  struct router *tmp;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ tmp = router_array[i]; }
  }
  if(value == 0){
    tmp->flags &= ~1;
  }
  else if(value == 1){
    tmp->flags |= 1;
  }
}

//Setter wireless-bit'et i en router, tar inn en id og en verdi
void set_wireless(int id, int value){
  struct router *tmp;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ tmp = router_array[i]; }
  }
  if(value == 0){
    tmp->flags &= ~(1 << 1);
  }
  else if(value == 1){
    tmp->flags |= (1 << 1);
  }
}

//Setter 5ghz-bit'et i en router, tar inn en id og en verdi
void set_5ghz(int id, int value){
  struct router *tmp;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ tmp = router_array[i]; }
  }
  if(value == 0){
    tmp->flags &= ~(1 << 2);
  }
  else if(value == 1){
    tmp->flags |= (1 << 2);
}
}

//Setter endrings-bit'ene i en router, tar inn en id og en verdi
void set_change(int id, int value){
  struct router *tmp;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ tmp = router_array[i]; }
  }
  if(value == INCREASE_CHANGE){
    unsigned char current_value = tmp->flags;
    current_value = current_value >> 4;
    if(current_value == 15){current_value = 0;}
    else{current_value++;}

    current_value  = current_value << 4;
    tmp->flags &= 0xF;
    tmp->flags |= current_value;
  }
  else{
    if(value > 15){return;}
    unsigned char new_value = (unsigned char) value;
    tmp->flags &= 0xF;
    new_value = new_value << 4;
    tmp->flags |= new_value;
  }
}

//Samlefunksjon for å sette ett av flaggene i en router
void set_flags(int id, int flag, int value){
  if(flag == 3 || flag > 4){
    printf("Invalid flag in function set_flags, no changes were made.\n");
    return;
  }
  int valid_id = 0;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ valid_id = 1;}
  }
  if(!valid_id){ return; }

  switch (flag) {
    case 0: set_active(id, value); break;
    case 1: set_wireless(id, value); break;
    case 2: set_5ghz(id, value); break;
    case 4: set_change(id, value); break;
  }
}

//Legger til en kobling fra en router til en annen, tar inn to forskjellige id'er
int add_connection(int from, int to){
  struct router *tmp_from = NULL, *tmp_to = NULL;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == from){ tmp_from = router_array[i]; }
    if(router_array[i]->ID == to){ tmp_to = router_array[i]; }
  }
  if(tmp_to == NULL || tmp_from == NULL){ printf("Invalid ID in function add_connection, no changes were made.\n");return EXIT_FAILURE; }

  //Checks if new connection already exists,
  for(int i = 0; i<tmp_from->number_of_neighbours; i++){
    if(tmp_from->neighbours[i]->ID == tmp_to->ID){return EXIT_FAILURE;}
  }
  tmp_from->neighbours[tmp_from->number_of_neighbours] = tmp_to;
  tmp_from->number_of_neighbours++;
  set_flags(tmp_from->ID, 4, INCREASE_CHANGE);
  return EXIT_SUCCESS;
}

//Sletter en router, tar inn en router-id
int delete_router(int id){
  struct router *to_be_removed = NULL, *tmp, *tmp2;
  int index;
  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == id){ to_be_removed = router_array[i]; index = i;}
  }
  if(to_be_removed == NULL){return EXIT_FAILURE;}
  for(int i = 0; i < num_routers; i++){
    tmp = router_array[i];
    for(int j = 0; j<tmp->number_of_neighbours; j++){
      if(tmp->neighbours[j]->ID == id){
        set_flags(tmp->ID, 4, INCREASE_CHANGE);
        to_be_removed = tmp->neighbours[j];
        tmp2 = tmp->neighbours[(tmp->number_of_neighbours)-1];
        tmp->neighbours[(tmp->number_of_neighbours)-1] = to_be_removed;
        tmp->neighbours[j] = tmp2;
        tmp->number_of_neighbours--;
      }
    }
  }
  tmp2 = router_array[num_routers-1];
  router_array[num_routers-1] = to_be_removed;
  router_array[index] = tmp2;
  free(to_be_removed);
  num_routers--;
  return EXIT_SUCCESS;
}

//Finner en sti mellom to routere, og returnerer enten true eller false. Tar inn to id'er
int find_path(int from, int to){
  struct router *tmp_from = NULL, *tmp_to = NULL;

  for(int i = 0; i < num_routers; i++){
    if(router_array[i]->ID == from){ tmp_from = router_array[i]; }
    if(router_array[i]->ID == to){ tmp_to = router_array[i]; }
  }
  if(tmp_to == NULL || tmp_from == NULL){ return 0; }

  tmp_from->visited = 1;
  for(int i = 0; i<tmp_from->number_of_neighbours; i++){
    if(tmp_from->neighbours[i]->ID == to){ return 1; }
    if(tmp_from->neighbours[i]->visited != 1){
      if(find_path(tmp_from->neighbours[i]->ID, to)){ return 1;}
    }
  }

  tmp_from->visited = 0;
  return 0;
}

//Resetter alle routerne slik at man kan kjøre find_path på nytt
void reset_visited(){
  for(int i = 0; i < num_routers; i++){
    router_array[i]->visited = 0;
  }
}

//Leser inn en binærfil med informasjon om routere, tar inn et filnavn
void read_dat_file(const char *filename){
  FILE *datFile = fopen(filename, "r");
  if(datFile == NULL){
    perror("fopen, read_dat_file");
  }

  int N, i;
  fread(&N, sizeof(int), 1, datFile);
  if(ferror(datFile)){
    fprintf(stderr, "Error occured in reading .dat file\n");
    fclose(datFile);
    exit(EXIT_FAILURE);
  }

  num_routers = N;
  //Her allokeres det 8*antall routere. Hver peker er 8 bytes
  router_array = malloc(sizeof(struct router *) * N);
  if(router_array == NULL){ fprintf(stderr, "%malloc error, router array\n"); fclose(datFile);}

  for(i = 0; i<N; i++){
    //Her allokeres det 344 bytes (4+1+1+250+80+4+4)
    struct router *tmp = malloc(sizeof(struct router));
    if(tmp == NULL){ fprintf(stderr, "%malloc error, single router\n"); fclose(datFile);}
    fread(&(tmp->ID), sizeof(int), 1, datFile);
    fread(&(tmp->flags), sizeof(char), 1, datFile);
    fread(&(tmp->length_of_modelstring), 1, 1, datFile);
    fread(&(tmp->model), tmp->length_of_modelstring, 1, datFile);
    fread(&(tmp->model[tmp->length_of_modelstring]), 1, 1, datFile);
    tmp->number_of_neighbours = 0;
    router_array[i] = tmp;
  }

  int from, to;
  while(fread(&from, sizeof(int), 1, datFile) && fread(&to, sizeof(int), 1, datFile)){
    add_connection(from, to);
  }

  if(ferror(datFile)){
    fprintf(stderr, "Error occured in reading .dat file\n");
    fclose(datFile);
    exit(EXIT_FAILURE);
  }
  fclose(datFile);
}

//Leser inn en tekstfil med kommandoer, tar inn filnavn.
//Denne funksjonen tar i bruk mange av funskjonene definert over
void read_txt_file(const char *filename){
  FILE *txtFile = fopen(filename, "r");
  if(txtFile == NULL){
    perror("fopen, read_txt_file");
  }
  char line[MAX_LINE_LENGTH];
  while(fgets(line, MAX_LINE_LENGTH, txtFile) != NULL){
    char *token = strtok(line, " ");

    if(strcmp(token, "print") == 0){
      int id = atoi(strtok(NULL, ""));
      print_router(id);
    }
     else if(strcmp(token, "sett_modell") == 0){
      char name[MAX_LINE_LENGTH];
      name[0] = 0;
      int id = atoi(strtok(NULL, " "));
      char *next = strtok(NULL, " ");
      do {
        strcat(name, next);
        strcat(name, " ");
        next = strtok(NULL, " ");
      } while(next != NULL);
      name[strcspn(name, "\n")] = 0;
      set_model_name(id, name);
    }
    else if(strcmp(token, "sett_flagg") == 0){
      int id = atoi(strtok(NULL, " "));
      int flag = atoi(strtok(NULL, " "));
      int value = atoi(strtok(NULL, " "));
      set_flags(id, flag, value);
    }
    else if(strcmp(token, "finnes_rute") == 0){
      reset_visited();
      int from_id = atoi(strtok(NULL, " "));
      int to_id = atoi(strtok(NULL, " "));
      if(find_path(from_id, to_id)){
        printf("Det finnes en rute mellom %d og %d\n", from_id, to_id);
      }
      else{
        printf("Det finnes IKKE en rute mellom %d og %d\n", from_id, to_id);
      }
    }
    else if(strcmp(token, "slett_ruter") == 0){
      int id = atoi(strtok(NULL, " "));
      if(delete_router(id)){ printf("Sletter router med ID: %d\n", id); }
    }
    else if(strcmp(token, "legg_til_kobling") == 0){
      int from_id = atoi(strtok(NULL, " "));
      int to_id = atoi(strtok(NULL, " "));
      if(add_connection(from_id, to_id)){ printf("Legger til kobling fra %d til %d\n", from_id, to_id); }
    }
  }

  if(ferror(txtFile)){
    fprintf(stderr, "Error occured in reading .txt file\n");
    fclose(txtFile);
    exit(EXIT_FAILURE);
  }
  fclose(txtFile);
}

//Lager en binærfil i samme format som inn-filen.
void make_out_file(char *filename){
  FILE *outFile = fopen(filename, "w");
  if(outFile == NULL){
    perror("fopen");
  }

  int i, j;
  fwrite(&num_routers, sizeof(int), 1, outFile);

  for(i = 0; i<num_routers; i++){
    struct router *tmp = router_array[i];
    char nullbyte = 0;
    fwrite(&(tmp->ID), sizeof(int), 1, outFile);
    fwrite(&(tmp->flags), sizeof(char), 1, outFile);
    fwrite(&(tmp->length_of_modelstring), 1, 1, outFile);
    fwrite(&(tmp->model), tmp->length_of_modelstring, 1, outFile);
    fwrite(&nullbyte, 1, 1, outFile);
  }

  for(i = 0; i<num_routers; i++){
    for(j = 0; j<(router_array[i]->number_of_neighbours); j++){
      fwrite(&(router_array[i]->ID), sizeof(int), 1, outFile);
      fwrite(&(router_array[i]->neighbours[j]->ID), sizeof(int), 1, outFile);
    }
  }

  if(ferror(outFile)){
    fprintf(stderr, "Error occured in writing .dat file\n");
    fclose(outFile);
    exit(EXIT_FAILURE);
  }

  fclose(outFile);
}

//Her kjøres programmet
int main(int argc, char const *argv[]) {
  if(argc != 3){
    fprintf(stderr, "USAGE: %s <filename.dat> <filename.txt>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  read_dat_file(argv[1]);
  //read_txt_file(argv[2]);
  print_all_routers();
  make_out_file("new-topology.dat");

  free_routers();
  return EXIT_SUCCESS;
}
