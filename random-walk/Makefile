CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -O2 -Iinclude -D_POSIX_C_SOURCE=200112L
LDFLAGS=-pthread

BIN=bin

COMMON_SRC=src/common/net.c src/common/protocol.c

SERVER_SRC=src/server/main.c src/server/server.c src/server/simulation.c \
           src/server/world.c src/server/config.c src/server/results.c

CLIENT_SRC=src/client/main.c src/client/client.c src/client/menu.c src/client/render.c

all: dirs server client

dirs:
	mkdir -p $(BIN)

server: $(COMMON_SRC) $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $(BIN)/server $(LDFLAGS)

client: $(COMMON_SRC) $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $(BIN)/client $(LDFLAGS)

clean:
	rm -rf $(BIN)

.PHONY: all dirs server client clean
