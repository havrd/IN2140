CFLAGS = -Wall -Wextra -g
VFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --malloc-fill=0x40 --free-fill=0x23
BIN = ruterdrift

all: $(BIN)

ruterdrift: ruterdrift.c
	gcc $(CFLAGS) ruterdrift.c -o ruterdrift

check_ruterdrift: ruterdrift
	valgrind $(VFLAGS) ./ruterdrift 10_routers_15_edges/topology.dat 10_routers_15_edges/commands.txt

run: ruterdrift
	./ruterdrift 10_routers_15_edges/topology.dat 10_routers_15_edges/commands.txt


clean:
	rm -f $(BIN)

#5_routers_fully_connected/topology.dat
