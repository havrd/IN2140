CFLAGS = -Wall -Wextra -g
VFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --malloc-fill=0x40 --free-fill=0x23
BIN = client server

all: $(BIN)


client: newFSP-client.c send_packet.c rdp-protocol.c
	gcc $(CFLAGS) newFSP-client.c send_packet.c rdp-protocol.c -o client

server: newFSP-server.c send_packet.c rdp-protocol.c
	gcc $(CFLAGS) newFSP-server.c send_packet.c rdp-protocol.c -o server

clean:
	rm -f $(BIN) kernel-file-*
