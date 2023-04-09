CFLAGS = -O2

all: client server

client: client.c common.c config.h common.h
	$(CC) $(CFLAGS) $< common.c -o $@

server: server.c common.c config.h common.h
	$(CC) $(CFLAGS) common.c $< -o $@

.PHONY: all
