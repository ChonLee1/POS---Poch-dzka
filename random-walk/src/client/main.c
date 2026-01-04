#include "net.h"
#include "protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    uint16_t port = 5555;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = (uint16_t)atoi(argv[2]);

    int fd = net_connect(host, port);
    if (fd < 0) { perror("net_connect"); return 1; }

    const char* hello = "hello-from-client";
    if (proto_send(fd, MSG_HELLO, hello, (uint32_t)strlen(hello)) != 0) {
        fprintf(stderr, "[client] failed to send HELLO\n");
        close(fd);
        return 1;
    }

    msg_type_t t;
    uint32_t len = 0;
    if (proto_recv(fd, &t, NULL, 0, &len) != 0 || t != MSG_HELLO_ACK) {
        fprintf(stderr, "[client] expected HELLO_ACK\n");
        close(fd);
        return 1;
    }
    printf("[client] handshake OK\n");


    for (int i = 0; i < 20; i++) {
    msg_type_t rt;
    uint32_t rlen = 0;
    msg_state_t st;

    if (proto_recv(fd, &rt, &st, (uint32_t)sizeof(st), &rlen) != 0) break;
    if (rt != MSG_STATE || rlen != sizeof(st)) continue;

    printf("[client] step=%u pos=(%d,%d)\n", st.step, st.x, st.y);
    }

    (void)proto_send(fd, MSG_QUIT, NULL, 0);
    close(fd);
    return 0;
}
