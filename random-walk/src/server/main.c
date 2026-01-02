#include "net.h"
#include "protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    uint16_t port = 5555;
    if (argc >= 2) port = (uint16_t)atoi(argv[1]);

    int lfd = net_listen(port, 8);
    if (lfd < 0) {
        perror("net_listen");
        return 1;
    }
    printf("[server] listening on %u...\n", (unsigned)port);

    int cfd = net_accept(lfd);
    if (cfd < 0) {
        perror("net_accept");
        close(lfd);
        return 1;
    }
    printf("[server] client connected\n");

    // receive HELLO
    msg_type_t t;
    char payload[64];
    uint32_t len = 0;

    if (proto_recv(cfd, &t, payload, sizeof(payload), &len) != 0 || t != MSG_HELLO) {
        fprintf(stderr, "[server] expected HELLO\n");
        close(cfd);
        close(lfd);
        return 1;
    }
    payload[(len < sizeof(payload)) ? len : (sizeof(payload) - 1)] = 0;
    printf("[server] HELLO payload: '%s'\n", payload);

    // send ACK
    if (proto_send(cfd, MSG_HELLO_ACK, NULL, 0) != 0) {
        fprintf(stderr, "[server] failed to send ACK\n");
        close(cfd);
        close(lfd);
        return 1;
    }

    printf("[server] handshake OK, closing.\n");
    close(cfd);
    close(lfd);
    return 0;
}
