# Kompilátor (GNU C compiler)
CC=gcc

# CFLAGS = prepínače pre kompiláciu:
# -std=c11            -> používame štandard C11
# -Wall -Wextra       -> zapne veľa varovaní (lepší kód)
# -Werror             -> varovania berie ako chyby (kvalita, hodnotitelia to majú radi)
# -pedantic           -> prísnejšia kontrola štandardu
# -O2                 -> optimalizácia
# -Iinclude           -> kde hľadať naše vlastné hlavičky (.h)
# -D_POSIX_C_SOURCE=200112L -> sprístupní POSIX funkcie/typy (napr. getaddrinfo, struct addrinfo)
# -g                  -> debug symboly (pre valgrind, gdb)
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -O2 -g -Iinclude -D_POSIX_C_SOURCE=200112L

# LDFLAGS = prepínače pre linkovanie:
# -pthread -> zapne podporu pthread (vlákna) a správne nalinkuje knižnice
LDFLAGS=-pthread

# Výstupný priečinok pre binárky
BIN=bin

# Zdrojáky spoločné pre server aj klient (sockety + protokol)
COMMON_SRC=src/common/net.c src/common/protocol.c

# Zdrojáky servera
SERVER_SRC=src/server/main.c src/server/server.c src/server/results.c

# Zdrojáky klienta
CLIENT_SRC=src/client/main.c src/client/client.c src/client/menu.c

# Default target (spustí sa keď dáš len "make"):
# Najprv vytvorí priečinky, potom zbuildí server aj klienta
all: dirs server client

# Vytvor výstupný priečinok pre binárky
# POZOR: riadky s príkazmi musia začínať TABOM, nie medzerami!
dirs:
	mkdir -p $(BIN)

# Build server:
# $^ = všetky závislosti (COMMON_SRC + SERVER_SRC)
server: $(COMMON_SRC) $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $(BIN)/server $(LDFLAGS)

# Build client:
client: $(COMMON_SRC) $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $(BIN)/client $(LDFLAGS)

# Valgrind server
valgrind-server: server
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BIN)/server 5555

# Valgrind client
valgrind-client: client
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BIN)/client 127.0.0.1 5555

# Vyčistenie build artefaktov
clean:
	rm -rf $(BIN)

# Označenie "falošných" targetov (nie sú to skutočné súbory)
.PHONY: all dirs server client clean
