CFLAGS = -Wall -Wextra -g
VFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --malloc-fill=0x40 --free-fill=0x23
BIN = newFSP-client newFSP-server

all: $(BIN)


newFSP-client: newFSP-client.c send_packet.c rdp-protocol.c
	gcc $(CFLAGS) newFSP-client.c send_packet.c rdp-protocol.c -o newFSP-client

newFSP-server: newFSP-server.c send_packet.c rdp-protocol.c
	gcc $(CFLAGS) newFSP-server.c send_packet.c rdp-protocol.c -o newFSP-server

clean:
	rm -f $(BIN) kernel-file-* vgcore*
