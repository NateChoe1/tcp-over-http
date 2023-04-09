CFLAGS = -O2

all: client server

client: client.c common.c config.h common.h
	$(CC) $(CFLAGS) $< common.c -o $@

server: server.c common.c config.h common.h
	$(CC) $(CFLAGS) common.c $< -o $@

config.h: config_dev.h
	cp $< $@

.PHONY: all
